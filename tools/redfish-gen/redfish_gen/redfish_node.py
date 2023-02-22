## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)

import yaml
import json
from xml.dom import minidom

from .redfish_base import RedfishBase
from .property import OemFragment, Property
from .globals import __RFG_PATH__
from .node_parameter import NodeParameter, StaticNodeParameter


class RedfishNode(RedfishBase):
    """
    Redfish node instance.
    Contains all relevant configs, provides engine to acquire
    required information to render template
    """

    # Instances factory
    @staticmethod
    def build(parent=None, **kwargs):
        if len(kwargs.keys()) != 1:
            raise Exception(
                "Invalid config definition. " +
                "Exaclty one Node should be defined at single yaml file."
            )
        key = [*kwargs.keys()][0]
        key_segments = key.split('@')
        if len(key_segments) == 1:
            return RedfishNode(parent, key, **kwargs.pop(key))
        elif len(key_segments) == 2:
            node_type = {
                "dynamic": DynamicRedfishNode,
            }.get(key_segments[1])
            return node_type(parent, key, **kwargs.pop(key))
        else:
            raise ValueError("Invalid config definition")

    def __load_openapi(self):
        data = self._load_bundle_file(self.__schema_file_openapi())
        return yaml.safe_load(data)

    def __load_schema_json(self):
        file = self.__schema_file_json()
        if file is not None:
            return json.loads(self._load_bundle_file(file))
        return file

    def __load_schema_spec_json(self):
        data = self._load_bundle_file(self.__schema_spec_file_json())
        return json.loads(data)

    def __schema_file(self):
        if self._schema_version:
            return self._schema_name + "." + self.version()
        return None

    def __schema_spec_file(self):
        return self._schema_name

    def __schema_file_json(self):
        filename = self.__schema_file()
        if filename is not None:
            return "json-schema/" + filename + ".json"
        return filename

    def __schema_spec_file_json(self):
        return "json-schema/" + self.__schema_spec_file() + ".json"

    def __schema_file_openapi(self):
        return "openapi/" + self.__schema_spec_file() + ".yaml"

    def __schema_file_csdl(self):
        return "csdl/" + self._schema_name + "_v1.xml"

    def __build_id(self):
        segment = "/%s%s" % (self.uri_prefix(), self._segment)
        parent = self._parent.id() if self._parent is not None else "/redfish"
        self._id = parent + segment

    # Constructor
    def __init__(self, parent, segment, **kwargs):
        super().__init__(standard="redfish",
                         schema_name=kwargs.get("Schema", None),
                         version=str(kwargs.get("Version", "")))
        self._parent = parent
        self._segment = kwargs.get("Segment", segment)
        self._classname = segment
        self._prefix = kwargs.get("Prefix", "").split("/")
        self.__build_id()
        self._y = kwargs
        # Load resourses
        self.openapi = self.__load_openapi()
        self.schema_spec = self.__load_schema_spec_json()
        self.schema = self.__load_schema_json()
        self.csdl = RedfishBase._load_csdl_file(self.__schema_file_csdl())

    def is_dynamic(self):
        return False

    def uri_prefix(self):
        segments = self._prefix if self.has_prefix() else ""
        if len(segments) > 0:
            return "%s/" % ("/".join(segments))
        return ""

    def uri_prefix_segments(self):
        segments = self._prefix if self.has_prefix() else list()
        if len(segments) > 0:
            return "\"%s\"" % (",".join(segments))
        return ""

    def has_prefix(self) -> bool:
        return len(self._prefix) > 0 and self._prefix[0] != ""

    # Parse config nodes
    def __actions(self):
        return (self._y["Actions"])

    def __get(self):
        return self.__actions()["Get"]

    def __modify(self):
        return self.__actions()("Modify")

    def __schema_spec_node(self):
        """
        BUG note
        Some schema declaration have wrong structure and the 'definitions' field of node
        might contains in a different places
        """
        if "definitions" in self.schema_spec:
            definitions = self.schema_spec["definitions"]
            if self._schema_name in definitions:
                return definitions[self._schema_name]
        raise Exception("Redfish JSON schema '" + self.__schema_spec_file_json()
                        + "' is corrupted")

    @staticmethod
    def __formatting_classname(classname):
        return str("Redfish" + classname[0].upper() + classname[1:])

    # Public
    def name(self):
        return self._y["Name"]

    def segment(self):
        return self._segment

    def __get_field_from_schema_spec_node(self, field):
        node = self.__schema_spec_node()
        if field in node:
            return node[field]
        elif field in node["anyOf"][1]:
            return node["anyOf"][1][field]
        raise Exception("Redfish JSON schema '"
                        + self.__schema_spec_file_json() + "' is corrupted")

    def description(self):
        return self.__get_field_from_schema_spec_node("description")

    def node_annotation(self):
        return self.__get_field_from_schema_spec_node("longDescription")

    def odata_context(self):
        if "title" in self.schema_spec:
            return "/redfish/v1/$metadata%s" % self.schema_spec["title"]
        return None

    def id(self):
        return self._id

    def odata_type(self):
        return self._available_schema()["title"]

    def classname(self):
        return RedfishNode.__formatting_classname(self._classname)

    def def_filename(self):
        return self._classname

    def insertable(self):
        return self.__schema_spec_node()["insertable"]

    def updatable(self):
        return self.__schema_spec_node()["updatable"]

    def deletable(self):
        return self.__schema_spec_node()["deletable"]

    def uris(self):
        return self.__schema_spec_node()["uris"]

    def node_specifiaction(self):
        return self.schema_spec["$id"]

    def validate(self):
        if self.id() not in self.uris():
            raise Exception("The node config '" + self.segment() +
                            "' is not valid to specified schema: " + self.classname())

    def parent_instance_definition(self) -> str:
        return ""

    def base_node_classname(self) -> str:
        childs = ",".join(self.childs())
        if len(self.childs()) > 0:
            childs = ", %s" % childs
        return "Node<%s%s> (ctx)" % (self.classname(), childs)

    def base_inherit_node_classname(self) -> str:
        childs = ",".join(self.childs())
        if len(self.childs()) > 0:
            childs = ", %s" % childs
        baseclass = "public Node<%s%s>" % (self.classname(), childs)
        if self.has_prefix():
            return baseclass + ", public IStaticSegments"
        return baseclass

    def childs(self):
        r = []
        try:
            for ref in self.__get()["Reference"]:
                if "Node" not in ref:
                    raise ValueError(
                        "Node key must be in a reference definition")
                if not isinstance(ref["Node"], list) and not isinstance(ref["Node"], str):
                    raise ValueError("Node key must be a list or a string")
                if isinstance(ref["Node"], str):
                    r.append(RedfishNode.__formatting_classname(ref["Node"]))
                elif isinstance(ref["Node"], list):
                    r += [RedfishNode.__formatting_classname(v)
                          for v in ref["Node"]]
        except Exception:
            pass
        return r

    def reference(self):
        r = list()
        try:
            if "Reference" not in self.__get():
                return r
            for ref in self.__get()["Reference"]:
                if "Hide" in ref and ref["Hide"] == True:
                    continue
                if "Field" not in ref:
                    if isinstance(ref["Node"], list):
                        raise ValueError(
                            "Node must be a string when 'Field' is not specified")
                    ref["Field"] = ref["Node"]
                if isinstance(ref["Node"], list):
                    ref_class_list = [
                        RedfishNode.__formatting_classname(n) for n in ref["Node"]]
                    ref_class = "ListReferences<%s>" % ",".join(ref_class_list)
                    ref['Type'] = 'List'
                else:
                    ref_class = "IdReference<%s>" % RedfishNode.__formatting_classname(
                        ref["Node"])
                    ref['Type'] = 'Object'
                ref["Classname"] = ref_class
                r.append(ref)
        except:
            pass
        return r

    def __check_properties_definition(self, source: str) -> bool:
        if not (isinstance(self.__get(), dict) and "Properties" in self.__get()):
            return False
        if not (isinstance(self.__get()["Properties"], dict) and source in self.__get()["Properties"]):
            return False
        return True

    def _properties_expand(self, source: str):
        if not self.__check_properties_definition(source):
            return []
        return Property.build(source, self.schema_id(), self._available_schema(), self.__get()["Properties"])

    def links(self):
        return self._properties_expand("Links")

    def links_class(self):
        links = self.links()
        if len(links) > 0:
            field_name = links[0].field_name()
            link_names = [p.classname() for p in links]
            links_class_fmt = ",".join(link_names)
            return "createAction<LinksGetter<%s>>(\"%s\")," % (links_class_fmt, field_name)
        return ""

    def related_items(self):
        return self._properties_expand("RelatedItems")

    def static_properties(self):
        return self._properties_expand("Static")

    def entities(self):
        return self._properties_expand("Entity")

    def fragments(self):
        return self._properties_expand("Fragments")

    def collections(self):
        return self._properties_expand("Collection")

    def collections_actions(self):
        collections = self._properties_expand("Collection")
        res = []
        for collection in collections:
            classname = "createAction<%s>()" % collection.classname()
            res.append(classname)
        return res

    def enums(self):
        return self._properties_expand("Enums")

    def annotations(self):
        return self._properties_expand("Annotations")

    def oem(self):
        return self._properties_expand("Oem")

    def oem_classes(self):
        instances = self._properties_expand("Oem")
        if len(instances) > 0:
            return "createAction<%s>()," % OemFragment.oem_class_getter_name(instances)
        return ""

    def fieldIdGetterDefinition(self) -> str:
        if self.schema is not None:
            return "createAction<StringGetter>(nameFieldId, fieldId),"

        return "/* The unique identifier is absent */"


class DynamicRedfishNode(RedfishNode):
    def __init__(self, parent, segment, **kwargs):
        segment = segment.split('@')[0]
        super(DynamicRedfishNode, self).__init__(parent, segment, **kwargs)
        self._parameter = NodeParameter.build(self._y["Parameter"])
        self._build_id()

    def node_parameter(self):
        return self._parameter

    def _build_id(self):
        if self._parent is not None:
            self._id = self._parent.id()
        else:
            self._id = "/redfish/"
        self._id = self._id + \
            ("/%s{%s}" % (self.uri_prefix(), self.node_parameter().name()))

    def is_dynamic(self):
        return True

    def parent_instance_definition(self) -> str:
        return "getTargetInstance()"

    def base_node_classname(self) -> str:
        base_classname = super().base_node_classname()
        return "%s, ParameterizedNode<%s>(ctx)" % (base_classname, self.classname())

    def base_inherit_node_classname(self) -> str:
        base_classname = super().base_inherit_node_classname()
        return "%s, public ParameterizedNode<%s>" % (base_classname, self.classname())

    def collections_actions(self):
        collections = self._properties_expand("Collection")
        res = []
        for collection in collections:
            classname = "createAction<%s>(getTargetInstance())" % collection.classname(
            )
            res.append(classname)
        return res

    def oem_classes(self):
        instances = self._properties_expand("Oem")
        if len(instances) > 0:
            return "createAction<%s>(getTargetInstance())," % OemFragment.oem_class_getter_name(instances)
        return ""

    def fieldIdGetterDefinition(self) -> str:
        return "createAction<CallableGetter>(nameFieldId, std::bind(&ParameterizedNode<%s>::getParameterValue, this))," % self.classname()

    def parameter_template(self):
        if isinstance(self.node_parameter(), StaticNodeParameter):
            return "/node.dynamic.static.mako"
        return "/node.dynamic.entity.mako"
