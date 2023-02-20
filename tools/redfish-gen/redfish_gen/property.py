## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)

from tarfile import ExtractError
from typing import Iterable
from urllib.parse import urlparse
from .globals import __RFG_PATH__

import redfish_gen.generator as generator
from .redfish_schema import RedfishSchema
import json


class Property():
    @staticmethod
    def build(source, name, schema, definition: dict, global_spec=None):
        kind_property = {
            "Static": StaticProperty,
            "Entity": EntitySource,
            "Fragments": Fragments,
            "Enums": EnumDefinition,
            "Oem": OemFragment,
            "Collection": Collection,
            "Links": Links,
            "Annotations": Annotations,
            "RelatedItems": RelatedItems,
        }.get(source)
        if source not in definition:
            return []
        source_def = definition[source]
        return [kind_property(field, name, schema, global_spec) for field in source_def]

    def __init__(self, definition, schema_name, spec, global_spec=None, field_name=None):
        self._def = definition
        self._schema_name = schema_name
        self._spec = spec
        self._global_spec = global_spec
        self._additional_args = ""
        if field_name is not None:
            self._field_name = field_name
        else:
            self._field_name = definition['Name'] if 'Name' in self._def else None
        if schema_name not in spec:
            if "definitions" in spec:
                # Two cases:
                # * Single property from $ref
                # * Clean node difinition
                target_spec = spec["definitions"][schema_name]
            elif "properties" in spec:
                # Redfish schema object expanding
                target_spec = spec
            else:
                raise ExtractError(
                    "Unkown type schema specified: %s" % schema_name)

        else:
            target_spec = spec
        if "anyOf" in target_spec:
            for i in target_spec["anyOf"]:
                if "properties" in i or self.field() in i:
                    target_spec = i
        if self.field() in target_spec:
            # Scalar field expanding
            self._prop_spec = target_spec[self.field()]
        elif "properties" in target_spec and self.field() in target_spec["properties"]:
            # Redfish schema object expanding
            self._prop_spec = target_spec["properties"][self.field()]
        else:
            raise ValueError(
                "Invalid property specification: %s" % self.field())
        self._node_spec = target_spec

    @staticmethod
    def __retrieve_ref(property_spec):
        if property_spec is None:
            return None
        if "anyOf" in property_spec:
            # TODO: add condition(by version) to select relevant object from the set
            lastSpec = property_spec["anyOf"][0]["$ref"]
            for anyOf in property_spec["anyOf"]:
                if "$ref" in anyOf:
                    lastSpec = anyOf["$ref"]
            return lastSpec
        elif "$ref" in property_spec:
            return property_spec["$ref"]
        elif "items" in property_spec:
            return Property.__retrieve_ref(property_spec["items"])
        return None

    @staticmethod
    def __reference_path_to_resolve(spec_ref, spec_loop_by):
        ref = Property.__retrieve_ref(spec_ref)
        if ref is None or spec_loop_by is None:
            return None
        return urlparse(ref)

    @staticmethod
    def __resolve_spec_by_ref(ref_path: str):
        if len(ref_path.path) > 0:
            schema = ref_path.path.replace(
                "/schemas/v1/", "").replace(".json", "")
            node = generator.Generator.get_node(schema)
            schema_path = __RFG_PATH__ + "/assets/schemas/bundle/json-schema/"
            if node is not None:
                return node.schema_spec
            else:
                filename = ref_path.path.replace("/schemas/v1/", schema_path)
                with open(filename) as f:
                    data = f.read()
                    return json.loads(data)
        return None

    @staticmethod
    def resolve_global_spec(prop_spec, global_spec, defaultGlobal=True):
        ref_path = Property.__reference_path_to_resolve(prop_spec, global_spec)
        if ref_path is None:
            return prop_spec
        spec = Property.__resolve_spec_by_ref(ref_path)
        if spec is not None:
            spec_ref_chk = spec
            prop_path = ref_path.fragment.split("/")
            for segment in prop_path:
                if len(segment) > 0:
                    if segment == "definitions" and segment not in spec_ref_chk:
                        segment = "properties"
                    if segment in spec_ref_chk:
                        spec_ref_chk = spec_ref_chk[segment]
            ref = Property.__retrieve_ref(spec_ref_chk)
            if ref is not None and not ref.endswith("/idRef"):
                # Checking recursive references again
                ref_spec = Property.__resolve_spec_by_ref(urlparse(ref))
                return Property.resolve_global_spec(ref_spec, spec, defaultGlobal)
            return spec
        return global_spec if global_spec is not None and defaultGlobal else prop_spec

    @staticmethod
    def resolve_property_spec(prop_spec, global_spec, recursive=True):
        ref_path = Property.__reference_path_to_resolve(prop_spec, global_spec)
        definition = Property.resolve_global_spec(prop_spec, global_spec)
        if ref_path is None:
            return prop_spec
        prop_path = ref_path.fragment.split("/")
        properties = definition["definitions"] if "definitions" in definition else None
        for segment in prop_path:
            if len(segment) > 0:
                if segment == "definitions" and segment not in definition:
                    segment = "properties"
                if segment in definition:
                    definition = definition[segment]
                else:
                    raise ValueError("Invalid specification of %s" % segment)
        if recursive:
            ref = Property.__retrieve_ref(definition)
            if ref is not None and not ref.endswith("/idRef"):
                ref_spec = Property.__resolve_spec_by_ref(urlparse(ref))
                recursive_spec = ref_spec if ref_spec is not None else global_spec
                return Property.resolve_property_spec(definition, recursive_spec)
        RedfishSchema.extract_schemas(properties)
        return definition

    def value(self):
        raise NotImplementedError()

    def field(self):
        return self._field_name

    @staticmethod
    def resolve_type(property_spec):
        if "type" in property_spec:
            if property_spec["type"] == "string" and "enum" in property_spec:
                return "enum"
            if isinstance(property_spec["type"], list):
                return property_spec["type"][0]
            return property_spec["type"]
        return None

    def type(self):
        deep_recursive_guard = 10
        counter = 0
        assumin_type = Property.resolve_type(self._prop_spec)
        while assumin_type is None:
            spec = self._global_spec if self._global_spec is not None else self._spec
            ref_prop_spec = self.resolve_property_spec(self._prop_spec, spec)
            assumin_type = Property.resolve_type(ref_prop_spec)
            if counter > deep_recursive_guard:
                return "unrecognized"
            counter += 1
        return assumin_type

    def is_reference(self):
        return "$ref" in self._prop_spec or "anyOf" in self._prop_spec and "$ref" in self._prop_spec["anyOf"][0]

    def is_nullable(self):
        return (isinstance(self._prop_spec["type"], list) and "null" in self._prop_spec["type"]) or ("anyOf" in self._prop_spec)

    def pattern(self):
        if not self.is_reference():
            return self._prop_spec["pattern"]
        return self.__resolve_ref(self.__retrieve_ref(self._prop_spec)).pattern()

    def readonly(self):
        return self._prop_spec["readonly"]

    def description(self, value=None):
        if "description" in self._prop_spec:
            return self._prop_spec["description"]
        spec = self.resolve_property_spec(self._prop_spec, self._global_spec)
        if "description" in spec:
            return spec["description"]
        return "Descriotion is not defined"

    def long_description(self):
        if "longDescription" in self._prop_spec:
            return self._prop_spec["longDescription"]
        spec = self.resolve_property_spec(self._prop_spec, self._global_spec)
        if "longDescription" in spec:
            return spec["longDescription"]
        return "Long descriotion is not defined"

    def getter_name(self, ptype=None):
        ptype = ptype if ptype is not None else self.type()
        prefix = {
            "boolean": "Bool",
            "integer": "Decimal",
            "number": "Float",
            "string": "String",
            "array": "Array",
        }.get(ptype, "String")
        if ptype == 'enum':
            return "%sEnum" % self._resolve_enum_type_name(self._prop_spec)
        return prefix + "Getter"

    def is_enum_type(self):
        self_type = self.type()
        return self_type == "enum"

    def _resolve_enum_type_name(self, spec):
        ref = self.__retrieve_ref(spec)
        ref_url = urlparse(ref)
        prop_path = ref_url.fragment.split("/")
        return prop_path.pop(len(prop_path)-1)

    def get_enum_type_name(self):
        if not self.is_enum_type():
            raise ValueError("The property is not a enum type")
        return self._resolve_enum_type_name(self._prop_spec)

    def get_additional_getter_args(self) -> str:
        return self._additional_args

    def add_additional_arg(self, definition):
        self._additional_args = "%s, %s" % (self._additional_args, definition)


class StaticProperty(Property):
    def __init__(self, definition, schema_name, spec, global_spec=None):
        super(StaticProperty, self).__init__(
            definition, schema_name, spec, global_spec)

    def value(self):
        if self.type() == "string":
            if self._def['Value'] is None:
                return "nullptr"
            return "\"%s\"" % self._def['Value']
        if self.type() == "boolean":
            return "true" if self._def['Value'] else "false"
        return self._def['Value']

    def getter_name(self, ptype=None):
        if self._def['Value'] is None:
            return "NullGetter"
        return super(StaticProperty, self).getter_name(ptype)

    def type(self):
        if super().type() == "enum":
            return "string"
        return super().type()


class EntityProperty(Property):
    def __resolve_getter_name(self, ptype, pspec):
        if ptype == "enum":
            return "EntityEnumFieldGetter<%sEnum>" % self._resolve_enum_type_name(pspec)
        elif ptype == "array":
            subtype = self._resolve_array_subtype()
            return "ListFieldGetter<%s>" % subtype
        return "ScalarFieldGetter"

    def getter_name(self, ptype=None):
        ptype = ptype if ptype is not None else self.type()
        return self.__resolve_getter_name(ptype, self._prop_spec)

    def __init__(self, definition, schema_name, spec, global_spec=None):
        super(EntityProperty, self).__init__(
            definition, schema_name, spec, global_spec)

    def source_field(self):
        return self._def['SourceField']

    def _resolve_array_subtype(self, global_spec=None):
        if global_spec is None:
            global_spec = self._global_spec
        array_spec = Property.resolve_property_spec(
            self._prop_spec, global_spec)
        array_type = Property.resolve_type(array_spec)
        if array_type == 'array':
            items = self._prop_spec["items"]
            global_spec = self._global_spec
            if global_spec is None and ('anyOf' in items or '$ref' in items):
                global_spec = Property.resolve_global_spec(
                    self._prop_spec, self._global_spec)
                return self._resolve_array_subtype(global_spec)
            array_type = items[0]['type'] if isinstance(
                items, list) else items['type']
        if isinstance(array_type, list):
            array_type = array_type[0]
        return Property.getter_name(self, array_type)


class EntityCondition:
    def __init__(self, definition) -> None:
        self._preset = None
        self._field = None
        self._value = None
        if "Preset" in definition:
            self._preset = definition['Preset']
            self._args = definition['Args'] if 'Args' in definition else []
        else:
            self._field = definition['Field']
            self._value = definition['Value']

    def __value(self):
        if self._value is None:
            return None
        if isinstance(self._value, str) and self._value.find("::") == -1:
            return "std::string(\"%s\")" % self._value
        return self._value

    def __field(self) -> str:
        if self._field is None:
            return None
        return self._field

    def __preset(self):
        if self._preset is None:
            return None
        return self._preset

    def __args(self) -> str:

        if self._args is None or not isinstance(self._args, list):
            return ""
        return ",".join(self._args)

    def definition(self):
        if self.__preset() is not None:
            return "%s(%s)" % (self.__preset(), self.__args())
        return " BaseEntity::Condition::buildEqual(%s, %s)" % (self.__field(), self.__value())


class EntitySource():
    def __init__(self, entity, schema_name, spec, global_spec=None):
        self._source = entity["Source"]
        self._name = entity["Name"] if "Name" in entity else self._source
        self._conditions = entity["Conditions"] if "Conditions" in entity else [
        ]
        fields_to_init = entity["Fields"] if "Fields" in entity else []
        self._fields = [EntityProperty(
            field, schema_name,  spec, global_spec) for field in fields_to_init]

    def source(self):
        return self._source

    def name(self):
        return self._name

    def fields(self):
        return self._fields

    def conditions(self):
        return [EntityCondition(condtition) for condtition in self._conditions]

    def base_class(self, is_parameterized, base_entity=None):
        additional_vtparams = ""
        if base_entity is not None:
            additional_vtparams = ", %s" % base_entity
        elif is_parameterized:
            additional_vtparams = ", TParameterEntity"
        return " EntityGetter<%s%s>" % (self.source(), additional_vtparams)

    def classname(self):
        return "%sEntityGetter" % self.name()


class Fragments:
    def __init__(self, fragment, parent_id, spec=None, global_spec=None, field_name=None):
        self._name = field_name if field_name is not None else fragment["Name"]
        self._parent_id = parent_id
        global_spec = global_spec if global_spec is not None else spec
        if spec is not None:
            property_spec = self._resolve_property_spec(spec, global_spec)
            if self.name() not in global_spec["definitions"]:
                global_spec = self._resolve_global_spec(spec, global_spec)
            self.init_sources(property_spec, fragment, global_spec)

    def init_sources(self, property_spec, fragment, global_spec):
        self._sources = {
            "static": Property.build("Static", self.name(), property_spec, fragment, global_spec),
            "entity": Property.build("Entity", self.name(), property_spec, fragment, global_spec),
            "collection": Property.build("Collection", self.name(), property_spec, fragment, global_spec),
            "annotations": Property.build("Annotations", self.name(), property_spec, fragment, global_spec),
            "fragments": Property.build("Fragments", self.name(), property_spec, fragment, global_spec),
            "enums": Property.build("Enums", self.name(), property_spec, fragment, global_spec),
            "related_items": Property.build("RelatedItems", self.name(), property_spec, fragment, global_spec),
            "oem": Property.build("Oem", self.name(), property_spec, fragment, global_spec)
        }

    def _resolve_property_spec(self, spec, global_spec):
        try:
            property_spec = spec
            if "definitions" in property_spec:
                property_spec = spec["definitions"][self._parent_id]
            if "anyOf" in property_spec:
                # Take last revision from anyOf
                property_spec = property_spec["anyOf"][len(
                    property_spec["anyOf"]) - 1]
            property_spec = property_spec["properties"][self.name()]
            return Property.resolve_property_spec(property_spec, global_spec)
        except:
            raise ExtractError(
                "Could not resolve fragment of %s" % self.name())

    def _resolve_global_spec(self, ref_spec, global_spec):
        try:
            defaultGlobalSpec = True
            if "definitions" in ref_spec:
                ref_spec = ref_spec["definitions"][self._parent_id]
            if "anyOf" in ref_spec:
                # Take last revision from anyOf
                ref_spec = ref_spec["anyOf"][len(ref_spec["anyOf"]) - 1]
                defaultGlobalSpec = False
            if "properties" in ref_spec:
                ref_spec = ref_spec["properties"][self.name()]
            return Property.resolve_global_spec(ref_spec, global_spec, defaultGlobalSpec)
        except:
            raise ExtractError(
                "Could not resolve fragment of %s" % self.name())

    def parent(self):
        return self._parent_id

    def static_properties(self):
        return self._sources["static"]

    def entity_properties(self):
        return self._sources["entity"]

    def fragment_properties(self):
        return self._sources["fragments"]

    def enum_definitions(self):
        return self._sources["enums"]

    def collection_properties(self):
        return self._sources["collection"]

    def annotations(self):
        return self._sources["annotations"]

    def oem_properties(self):
        return self._sources["oem"]

    def related_items(self):
        return self._sources["related_items"]

    def oem_classes(self):
        instances = self.oem_properties()
        if len(instances) > 0:
            return "createAction<%s>(instance)," % OemFragment.oem_class_getter_name(instances)
        return ""

    def name(self):
        return self._name


class Collection(Fragments, EntitySource):
    def __init__(self, definition, parent_id, spec=None, global_spec=None):
        EntitySource.__init__(self, definition, parent_id, spec, global_spec)
        Fragments.__init__(self, definition, parent_id, spec, global_spec)

    def classname(self):
        return "%sCollectionGetter" % self.name()

    def action_classname(self):
        return "createAction<%s>(instance)" % self.classname()

    def base_class(self, is_parameterized, base_entity=None):
        if base_entity is None:
            base_entity = self.source()
        if is_parameterized:
            base_entity = "TParameterEntity"
        return "CollectionGetter<%sFragmentGetter, %s, %s>" % (self.name(), self.source(), base_entity)


class OemFragment(Fragments):
    def __init__(self, definition, parent_id, _, global_spec=None):
        self._oem_name = definition["Name"]
        super(OemFragment, self).__init__(definition,
                                          parent_id, None, global_spec, parent_id)
        self._version = definition["Version"]
        self._oem_spec = self.__load_spec()
        property_spec = self.__resolve_property_spec(self._oem_spec, parent_id)
        if self._oem_spec is None:
            raise Exception("Schema not found: %s" % self.__schema_name())
        self.init_sources(property_spec, definition, self._oem_spec)
        self._parent_id = parent_id

    def schema_type(self):
        return "#%s.v%s.%s" % (self.oem_name(), self.version().replace(".", "_"), self._parent_id )

    def owning_entity(self):
        return self._oem_spec["owningEntity"]

    def oem_name(self):
        return self._oem_name

    def __load_spec(self):
        filename = "%s.v%s.json" % (
            self.oem_name(), self.version().replace(".", "_"))
        schema_path = "%s/assets/schemas/bundle/json-schema/%s" % (
            __RFG_PATH__, filename)
        try:
            with open(schema_path) as f:
                data = f.read()
                return json.loads(data)
        except Exception:
            return None

    def version(self):
        return self._version

    def __resolve_property_spec(self, spec, parent_id):
        try:
            property_spec = spec["definitions"][parent_id]
            return Property.resolve_property_spec(property_spec, spec)
        except Exception:
            raise ExtractError(
                "Could not resolve fragment of %s" % self.oem_name())

    @staticmethod
    def oem_class_getter_name(instances: list):
        names = [i.oem_name() for i in instances]
        return "OemGetter<%s>" % ",".join(names)


class EnumDefinition(Property):
    def __init__(self, definition, schema_name, spec, global_spec=None):
        self._def = definition
        self._enum_for = self._enum_name = definition["Name"]
        self._prop_spec = spec
        if "For" in definition:
            self._enum_for = definition["For"]
        super(EnumDefinition, self).__init__(definition,
                                             schema_name, spec, global_spec, self.target())
        if global_spec is None:
            global_spec = spec
        self._prop_spec = Property.resolve_property_spec(
            self._prop_spec, global_spec)

    def name(self):
        return self._enum_name

    def target(self):
        return self._enum_for

    def source(self):
        return self._def["Source"]

    def values(self):
        predefined = list(self._def["Mapping"].keys())
        return list(set(predefined) & set(self._prop_spec["enum"]))

    def description(self, value):
        if value not in self.values():
            raise LookupError(
                "Value '%r' is not a valid enum of type '%s'" % (value, self.name()))
        return self._prop_spec["enumDescriptions"][value]

    def mapping(self):
        return [{"source": source, "dest": dest} for source, dest in self._def["Mapping"].items()]


class Annotations():
    @staticmethod
    def __build(definition):
        atype = definition['Type']
        ckind = {
            "ODataId": ODataIdAnnotations,
            "ODataCount": ODataCountAnnotations
        }.get(atype)
        return ckind(definition)

    def __init__(self, definition, _, _1, _2=None):
        self._instance = Annotations.__build(definition)

    def instance(self):
        return self._instance


class Annotation():
    def __init__(self, definition):
        self._def = definition

    def type(self):
        return self._def['Type']


class ODataIdAnnotations(Annotation):
    def __init__(self, definition):
        super(ODataIdAnnotations, self).__init__(definition)

    @staticmethod
    def getter_definition():
        return "createAction<CallableGetter>(nameFieldODataID, std::bind(&RedfishContext::getAnchorPath, *ctx))"


class ODataCountAnnotations(Annotation):
    def __init__(self, definition):
        super(ODataCountAnnotations, self).__init__(definition)
        self._field = definition["Field"]

    def getter_definition(self):
        return "createAction<CollectionSizeAnnotation>(\"%s\")," % self._field


class LinkParameter:
    def __init__(self, param_def) -> None:
        self._id = param_def["Id"]
        self._field = param_def["Field"]
        self._entity = param_def["Entity"] if "Entity" in param_def else None
        self._from_context = param_def["FromContext"] if "FromContext" in param_def else False
        conds = param_def["Conditions"] if "Conditions" in param_def else []
        self._conditions = [EntityCondition(c) for c in conds]

    def is_from_context(self) -> bool:
        return self._from_context

    def conditions(self) -> list:
        return self._conditions

    def field(self):
        return self._field

    def entity(self) -> str:
        return self._entity

    def id(self) -> str:
        return self._id


class Links(Property):
    def __init__(self, definition, _, spec, global_spec=None) -> None:
        self._name = definition["Name"]
        self._prefix = definition["Classname"] if "Classname" in definition else ""
        if "Field" in definition:
            self._field = definition["Field"]
        elif not hasattr(self, "_field"):
            self._field = "Links"
        self._template = definition["Template"]
        params_def = definition["Parameters"] if "Parameters" in definition else [
        ]
        self._ref = self.__resolve_type_ref(spec)
        self._parameters = [LinkParameter(param_def)
                            for param_def in params_def]
        self._verify_template(spec)

    def is_parameterized(self) -> bool:
        return len(self._parameters) > 0

    def template(self) -> str:
        return self._template

    def parameters(self) -> Iterable[LinkParameter]:
        return self._parameters

    def name(self) -> str:
        return self._name

    def classname(self):
        return "%s%s%s" % (self._prefix, self.name(), "Links")

    def get_type(self) -> str:
        return self._type

    def field_name(self) -> str:
        return self._field

    def _resolve_link_spec(self, spec):
        return spec["definitions"][self._field]["properties"][self._name]

    def __resolve_type_ref(self, spec):
        spec_node = self._resolve_link_spec(spec)
        if "items" in spec_node:
            spec_node = spec_node["items"]
            self._type = "LinkCollectionGetter"
        else:
            self._type = "LinkObjectGetter"
        return spec_node["$ref"]

    def _verify_template(self, spec):
        spec_node = self.__resolve_reference_spec(spec)
        if not self.template() in spec_node["uris"]:
            raise ValueError("Invalid template(%s) of link(%s) specified" % (
                self.template(), self.name()))

    def __resolve_reference_spec(self, spec):
        link_ref_spec = self._resolve_link_spec(spec)
        return Property.resolve_property_spec(link_ref_spec, spec, False)


class RelatedItems(Links):
    def __init__(self, definition, schema_name, spec, global_spec=None) -> None:
        self._schema_name = schema_name
        self._field = "RelatedItem"
        super(RelatedItems, self).__init__(
            definition, schema_name, spec, global_spec)

    def _resolve_link_spec(self, spec):
        if "definitions" in spec:
            return spec["definitions"][self._schema_name]["properties"][self.field_name()]
        return spec["properties"][self.field_name()]

    def classname(self):
        return "%sReference" % (self.name())

    def _verify_template(self, _):
        pass
