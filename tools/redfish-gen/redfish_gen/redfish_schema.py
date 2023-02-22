## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2023, KNS Group LLC (YADRO)

import json
from shutil import copy
from os import path, makedirs, walk
from xml.dom import minidom

from .redfish_base import RedfishBase
from .globals import __RFG_PATH__
from .globals import __WWW_PATH__

class RedfishSchema(RedfishBase):
    """
    Redfish schema instance.
    Contains all relevant schemes and versions, provides engine to acquire
    required information to prepare schema file
    """

    schemas = {}

    # Instances factory
    @staticmethod
    def build(standard: str, schemaName, version, resolved, schema_json):
        if schemaName in RedfishSchema.schemas and RedfishSchema.schemas[schemaName].version() is not None:
            return RedfishSchema.schemas[schemaName]
        schema = RedfishSchema(standard, schemaName, version, resolved, schema_json)
        RedfishSchema.schemas[schemaName] = schema
        return schema

    def __init__(self, standard: str, schema_name, version, resolved, schema_json) -> None:
        super().__init__(standard=standard,
                         schema_name=schema_name,
                         version=version)
        self._dependencies = set()
        self._resolved = resolved
        if schema_json is not None:
            self.schema = schema_json
        else:
            filename = self.__json_full_filename()
            self.schema = RedfishSchema.load_json(filename)

    def schema_and_version(self):
        if self._schema_version:
            return self._schema_name + ".v" + self._schema_version
        else:
            return self._schema_name

    def resolved(self):
        return self._resolved

    @staticmethod
    def redfish_v1_relative_path(standard: str):
        return path.join(standard, RedfishBase.redfish_version())

    @staticmethod
    def extract_schemas(properties: dict, deep = 0):
        """
        Interates properties recusively to find all inner schema references.
        """
        if deep == 10:
            raise Exception("Unadle to find all inner schemas in properties, max deep recusion reached: %s" % str(deep))
        if properties is not None:
            for prop in properties.values():
                if "anyOf" in prop:
                    for ref in prop["anyOf"]:
                        if "$ref" in ref:
                            RedfishSchema.__add_schema_by_ref_json(ref["$ref"])
                if "items" in prop:
                    prop = prop["items"]
                if "$ref" in prop:
                    RedfishSchema.__add_schema_by_ref_json(prop["$ref"])
                if "properties" in prop:
                    RedfishSchema.extract_schemas(prop["properties"], deep + 1)

    @staticmethod
    def __add_schema_by_ref_json(schema_ref):
        """
        Adds schema from "$ref" of property.
        (like "http://redfish.dmtf.org/schemas/v1/LogEntry.json#/definitions/LogEntry")
        """
        if schema_ref is None:
            return None
        end_schema = schema_ref.rfind(".json")
        if end_schema != -1:
            standard = "redfish/swordfish" if "swordfish" in schema_ref else "redfish"
            start_schema = schema_ref.rfind("/", None, end_schema)
            if start_schema == -1:
                start_schema = 0
            schema_name = schema_ref[start_schema + 1: end_schema]
            return RedfishSchema.__add_schema_by_schema_id(standard, schema_name)
        return None

    @staticmethod
    def __add_schema_by_schema_id(standard: str, schema_title: str):
        """
        Creates and adds schema by schema name with or without version.

        Args:
            standard (str): One of "redfish" or "redfish/swordfish" string,
            schema_title (str): Name of the schema with or without version.

        Returns:
            Created RedfishSchema object.
        """
        if schema_title.startswith("odata"):
            return None
        if schema_title.startswith("#"):
            schema_title = schema_title[1:]
        parts = schema_title.split(".")
        if len(parts) > 1:
            return RedfishSchema.__add_schema(standard, parts[0], parts[1])
        else:
            return RedfishSchema.__add_schema(standard, parts[0], None)

    @staticmethod
    def __add_schema(standard: str, schema_name: str, schema_version: str):
        if schema_name in RedfishSchema.schemas and RedfishSchema.schemas[schema_name].version() is not None:
            return RedfishSchema.schemas[schema_name]
        if schema_version is not None :
            if not schema_version.startswith("v"):
                schema_version = "v" + schema_version
            if not "_" in schema_version:
                schema_version = schema_version.replace(".", "_")
        else:
            schema_version = RedfishSchema.__find_latest_version_schema_in_json(standard, schema_name)
        schema = RedfishSchema.build(standard, schema_name, schema_version, resolved=False, schema_json=None)
        RedfishSchema.schemas[schema_name] = schema
        return schema

    @staticmethod
    def generate_all_schema():
        schemaIndexDoc, rootElement = RedfishSchema.__create_csdl_scheme_index()
        jsonSchemas = []
        RedfishSchema.__prepare_additional_schemas()
        RedfishSchema.__prepare_oem_schemas()
        RedfishSchema.__resolve_remaining_schemas()
        RedfishSchema.__build_result_schemas(schemaIndexDoc, rootElement, jsonSchemas)
        RedfishSchema.__build_indexes(schemaIndexDoc, rootElement, jsonSchemas)

    @staticmethod
    def __prepare_additional_schemas():
        # add requred redfish schemas
        additionalRequiredSchemas = {
            "RedfishExtensions": "redfish" # or, maybe, "redfish/swordfish"
            }
        for schemaName in sorted(additionalRequiredSchemas):
            if schemaName not in RedfishSchema.schemas:
                print("Generate additional schema: %s" % schemaName)
                standard = additionalRequiredSchemas[schemaName]
                RedfishSchema.__add_schema_by_name(standard, schemaName)
            else:
                print("Warning: additional scheme already added: \"%s\"" % schemaName)

    @staticmethod
    def __prepare_oem_schemas(jsondir=path.join(__RFG_PATH__, "assets", "schemas", "bundle", "json-schema")):
        for root, _, files in (next(walk(jsondir)),):
            for file in files:
                if file.startswith("Oem") and file.endswith(".json"):
                    filename = path.join(root, file)
                    _, schemaVersion = RedfishBase.schema_and_version_from(file)
                    # Add Oem*.json schemas only with version
                    if path.isfile(filename) and schemaVersion is not None:
                        json_spec = RedfishSchema.load_json(filename)
                        RedfishSchema.extract_schemas(json_spec)
                        RedfishSchema.__add_schema_by_ref_json(filename)

    @staticmethod
    def __resolve_remaining_schemas():
        resolve_iterations = 10
        while resolve_iterations > 0:
            resolve_iterations -= 1
            all_resolved = True
            keys = sorted(RedfishSchema.schemas.keys())
            for key in keys:
                schema = RedfishSchema.schemas[key]
                if not schema.resolved():
                    RedfishSchema.__resolve(schema)
                    all_resolved = False
            if all_resolved:
                break

    @staticmethod
    def __resolve(schema):
        json = schema.get_json_schema()
        if "definitions" in json:
            RedfishSchema.extract_schemas(json["definitions"])
        schema._resolved = True

    @staticmethod
    def __build_result_schemas(schemaIndexDoc, rootElement, jsonSchemas):
        keys = sorted(RedfishSchema.schemas)
        for key in keys:
            schema = RedfishSchema.schemas[key]
            if not schema.resolved():
                RedfishSchema.__resolve(schema)
            print(" - Generating Redfish scheme file: " + schema.schema_and_version())
            RedfishSchema.__copy_schema_scdl_from_assets(schema)
            RedfishSchema.__add_index_refs_part(schema.standard(), schema.schema_id(), schemaIndexDoc, rootElement)
            # JSON schema part
            jsonSchemaIndexPath = RedfishSchema.__copy_schema_json_file_from_assets(schema)
            jsonSchemas.append(jsonSchemaIndexPath)

    @staticmethod
    def __create_csdl_scheme_index():
        domImpl = minidom.getDOMImplementation()
        csdlDocument = domImpl.createDocument("http://docs.oasis-open.org/odata/ns/edmx", "edmx:Edmx", None) # TODO replace Url with variable
        rootElement = csdlDocument.documentElement
        rootElement.setAttributeNS("xmls", "xmlns:edmx", "http://docs.oasis-open.org/odata/ns/edmx") # TODO replace Url with variable
        rootElement.setAttribute("Version", "4.0")
        return csdlDocument, rootElement

    @staticmethod
    def __build_indexes(schemaIndexDoc, rootElement, jsonSchemas):
        # Generate "/redfish/v1/$metadata/index.xml"
        csdlSchemeIndex = schemaIndexDoc.toprettyxml(encoding="utf-8")
        csdlOutDir = path.join(__WWW_PATH__, RedfishSchema.redfish_v1_relative_path("redfish"), "$metadata")
        RedfishBase.write_file(csdlOutDir, "index.xml", csdlSchemeIndex)
        # Generate "/redfish/v1/JsonSchemas/index.json"
        RedfishSchema.__generate_json_index("redfish", jsonSchemas)

    @staticmethod
    def __add_schema_by_name(standard: str, schemaName):
        version = RedfishSchema.__find_latest_version_schema_in_json(standard, schemaName)
        RedfishSchema.build(standard, schemaName, version, resolved=False, schema_json=None)

    @staticmethod
    def __add_index_refs_part(standard: str, schema_name: str, csdlIndexDoc, csdlIndexRoot, scdlOemDir = ""):
        referenceElement = csdlIndexDoc.createElement('edmx:Reference')
        uri = path.join(RedfishSchema.redfish_v1_relative_path(standard), "schema", RedfishSchema.__schema_file_csdl_by_name(schema_name)) \
            if len(scdlOemDir) == 0 else path.join(scdlOemDir, schema_name + "_" + RedfishBase.redfish_version() + ".xml")
        csdlPath = path.join(__WWW_PATH__, uri)
        csdl = RedfishBase._load_csdl_file(csdlPath)
        referenceElement.setAttribute("Uri", "/" + uri)
        for schema in csdl.getElementsByTagName("Schema"):
            schemaNamespace = schema.getAttribute("Namespace")
            includeElement = csdlIndexDoc.createElement('edmx:Include')
            includeElement.setAttribute("Namespace", schemaNamespace)
            if schemaNamespace == "RedfishExtensions.v1_0_0": # The Alias="Redfish" is required attribute for the ReadfishServiceValidator project
                includeElement.setAttribute("Alias", "Redfish")
            referenceElement.appendChild(includeElement)
        csdlIndexRoot.appendChild(referenceElement)

    @staticmethod
    def __copy_schema_scdl_from_assets(schema, srcdir=__RFG_PATH__, destdir=__WWW_PATH__):
        """ Copying scheme file prevents re-formatting XML """
        if schema.is_oem():
            fromfile = path.join(
                srcdir,
                RedfishSchema.__oem_relative_path(),
                RedfishSchema.__schema_file_csdl_by_name(schema.schema_id()))
        else:
            fromfile = path.join(
                srcdir,
                RedfishSchema.__bundle_csdl_relative_path(schema.standard()),
                RedfishSchema.__schema_file_csdl_by_name(schema.schema_id()))
        todir = path.join(
            destdir,
            schema.standard(),
            RedfishBase.redfish_version(),
            "schema")
        if not path.isdir(todir):
            makedirs(todir, exist_ok=True)
        tofile = path.join(
            todir,
            RedfishSchema.__schema_file_csdl_by_name(schema.schema_id()))
        copy(fromfile, tofile)

    @staticmethod
    def __copy_schema_json_file_from_assets(schema, srcdir=__RFG_PATH__, destdir=__WWW_PATH__):
        """
        Copy json schema file from the "redfish" and "redfish/swordfish" bundles
        to the destination diretory.

        Args:
            srcdir (str): A directory with bundle source schema files,
            destdir (str): Target directory for schema files.

        Returns:
            Relative schemas directory.
        """
        schemaNameVer = schema.schema_and_version()

        fromfile = schema.__json_full_filename()
        toname = schema.schema_id()
        redfishdir = path.join(
            schema.standard(),
            RedfishBase.redfish_version(),
            RedfishBase._json_schemas_relative_dir(),
            toname)
        todir = path.join(
            destdir,
            redfishdir)
        if not path.isdir(todir):
            makedirs(todir, exist_ok=True)
        if not schemaNameVer.startswith("Oem"):
            tofile = path.join(
                todir,
                toname + ".json")
        else:
            tofile = path.join(
                todir,
                "index.json")
        if not path.isfile(fromfile):
            # Fix "*v1_0_0.json" to "1.0.0.json" for "old" version schemas in the DMTF bundle
            fromfile = RedfishSchema.__fix_to_dot_version(fromfile)
        copy(fromfile, tofile)
        if not schemaNameVer.startswith("Oem"):
            RedfishSchema.__write_json_subindex(schema.standard(), redfishdir, schema.schema_id())
        return redfishdir

    def __json_full_filename(self, srcdir=__RFG_PATH__):
        fromname = RedfishSchema.__map_json_schema_file(self.schema_id())
        from_name_version = fromname
        if self._schema_version is not None:
            from_name_version += ".v" + self._schema_version
        return path.join(
            srcdir,
            RedfishSchema.bundle_relative_path(self._standard),
            "json-schema",
            from_name_version + ".json")

    @staticmethod
    def __generate_json_index(standard: str, json_schemas):
        json_schema_index = RedfishSchema.__create_json_schema_index(standard)
        # Generate "/redfish/v1/JsonSchemas/index.json"
        for schema_id in json_schemas:
            RedfishSchema.__add_json_scheme_index_member(json_schema_index, schema_id)
        json_schema_index["Members@odata.count"] = len(json_schema_index["Members"])
        content = json.dumps(json_schema_index, indent=2)
        json_out_dir = path.join(__WWW_PATH__,
            standard,
            RedfishBase.redfish_version(),
            "JsonSchemas"
            )
        RedfishBase.write_file(json_out_dir, "index.json", content)

    @staticmethod
    def __write_json_subindex(standard: str, relative_schema_path, schema_spec_name, destdir = __WWW_PATH__):
        content = RedfishSchema.__generate_json_schema_index(standard, schema_spec_name, relative_schema_path)
        fullpath = path.join(destdir, relative_schema_path)
        RedfishBase.write_file(fullpath, "index.json", content)

    @staticmethod
    def __create_json_schema_index(standard: str):
        return {
            "@odata.id": "/" + standard + "/" + RedfishBase.redfish_version() + "/" + RedfishBase._json_schemas_relative_dir(),
            # TODO why the next line contains "$metadata" subdir?
            "@odata.context": "/" + standard +"/" + RedfishBase.redfish_version() + "/$metadata#JsonSchemaFileCollection.JsonSchemaFileCollection",
            "@odata.type": "#JsonSchemaFileCollection.JsonSchemaFileCollection",
            "Name": "JsonSchemaFile Collection",
            "Description": "Collection of JsonSchemaFiles",
            "Members@odata.count": 0,
            "Members": []
        }

    @staticmethod
    def __add_json_scheme_index_member(index_dict, schema_path):
        index_dict["Members"].append(
            {
                "@odata.id": "/" + schema_path
            })

    @staticmethod
    def __generate_json_schema_index(standard: str, schema_spec_name, relative_schema_path):
        template = RedfishSchema.__generate_json_schema_index_template()
        template = template.replace("{name}", schema_spec_name)
        template = template.replace("{relativeSchemaPath}", relative_schema_path)
        template = template.replace("{JsonSchemas}", RedfishBase._json_schemas_relative_dir())
        template = template.replace("{standard}",standard)
        template = template.replace("{v}", RedfishBase.redfish_version())
        return template

    @staticmethod
    def __generate_json_schema_index_template():
        return '''{
    "@odata.context": "/{standard}/{v}/$metadata#JsonSchemaFile.JsonSchemaFile",
    "@odata.id": "/{standard}/{v}/{JsonSchemas}/{name}",
    "@odata.type": "#JsonSchemaFile.v1_0_2.JsonSchemaFile",
    "Name": "{name} Schema File",
    "Schema": "#{name}.{name}",
    "Description": "{name} Schema File Location",
    "Id": "{name}",
    "Languages": [
        "en"
    ],
    "Languages@odata.count": 1,
    "Location": [
        {
            "Language": "en",
            "PublicationUri": "http://redfish.dmtf.org/schemas/{standard}/{v}/{name}.json",
            "Uri": "/{relativeSchemaPath}/{name}.json"
        }
    ],
    "Location@odata.count": 1
}'''

    @staticmethod
    def __find_latest_version_schema_in_json(standartd: str, schema_id, srcdir=__RFG_PATH__):
        json_bundle_path = path.join(
            srcdir,
            RedfishSchema.bundle_relative_path(standartd),
            "json-schema")
        schema_id = RedfishSchema.__map_json_schema_file(schema_id)
        newest_name = schema_id
        newest_double_ver = 0
        for _, _, files in (next(walk(json_bundle_path)),):
            for file in files:
                if file.startswith(schema_id + ".") and file.endswith(".json"):
                    current_double_ver = RedfishSchema.__version_to_double(file)
                    if newest_double_ver < RedfishSchema.__version_to_double(file):
                        newest_name = file
                        newest_double_ver = current_double_ver
        return RedfishBase._version_from_filename(newest_name)

    @staticmethod
    def __schema_file_csdl_by_name(schema_id):
        return schema_id + "_" + RedfishBase.redfish_version() + ".xml"

    @staticmethod
    def __bundle_csdl_relative_path(standard: str):
        if "swordfish" in standard:
            # bundle/scdl-schema for SNI Swordfish bundle zip
            return path.join(RedfishSchema.bundle_relative_path(standard), "csdl-schema")
        else:
            # bundle/scdl for DMTF bundle zip
            return path.join(RedfishSchema.bundle_relative_path(standard), "csdl")

    @staticmethod
    def bundle_relative_path(standard: str):
        if "swordfish" in standard:
            return path.join("assets", "schemas", "swordfish", "bundle")
        else:
            return path.join("assets", "schemas", "bundle")

    @staticmethod
    def __oem_relative_path():
        return path.join("assets", "oem")

    @staticmethod
    def load_json(fullname):
        data = RedfishSchema._load_file_fullpath(fullname)
        return json.loads(data)

    @staticmethod
    def __version_to_double(schema_spec):
        parts = schema_spec.split(".")
        if len(parts) > 1:
            ver_parts = parts[1].split("_")
            if len(ver_parts) >= 3:
                return int(ver_parts[1]) + int(ver_parts[2]) / 100000 + 0.00000001
            elif len(ver_parts) >= 2:
                return int(ver_parts[1]) + 0.00000001
        return 0

    @staticmethod
    def __map_json_schema_file(basename):
        schemas_map = RedfishSchema.__json_schema_file_map()
        if basename in schemas_map:
            return schemas_map[basename]
        else:
            return basename

    @staticmethod
    def __json_schema_file_map():
        return { "RedfishExtensions": "redfish-schema" }

    @staticmethod
    def __fix_to_dot_version(filename):
        """
        Fix "*v1_0_0.json" to "1.0.0.json" for "old" version schemas in the DMTF bundle.
        """
        ver_index = filename.rfind(RedfishBase.redfish_version())
        last_dot_ndex = filename.rfind(".", ver_index)
        if ver_index != -1 and last_dot_ndex != -1:
            return filename[0: ver_index] \
                + filename[ver_index + 1: last_dot_ndex].replace("_", ".") \
                + filename[last_dot_ndex:]
        return filename
