## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2023, KNS Group LLC (YADRO)

import yaml
import json
from os import path, makedirs
from xml.dom import minidom

from .globals import __RFG_PATH__

class RedfishBase:
    """
    Redfish base class.
    Contains all common methods for RedfishNode and RedfishSchema classes.
    """

    def __init__(self, standard: str, schema_name, version) -> None:
        """
        Construct base object.
        """
        if standard != "redfish" and standard != "redfish/swordfish":
            raise Exception("API Standard \"%s\" in not supported" % standard)
        self._standard = standard
        self._schema_name = schema_name
        if self._schema_name is None:
            raise ValueError("Node config: require field 'Schema'")
        self._schema_version = version[1:] if version is not None and version.startswith("v") else version

    def standard(self):
        return self._standard

    def schema_id(self):
        return self._schema_name

    def version(self):
        if self._schema_version:
            return "v" + self._schema_version.replace(".", "_")
        return None

    def is_oem(self):
        return self._schema_name.startswith("Oem")

    def _load_openapi(self):
        data = RedfishBase._load_bundle_file(self.standard(), self._schema_file_openapi())
        return yaml.safe_load(data)

    def _load_schema_json(self):
        """
        Loads json schema from file with version, like "Power.v1_7_6.json".
        """
        filename = self._schema_file_json_with_ver()
        if filename is not None:
            data = RedfishBase._load_bundle_file(self.standard(), filename)
            return json.loads(data)
        return None

    def _load_schema_spec_json(self):
        """
        Loads json schema spec from file without version, like "Power.json".
        """
        data = RedfishBase._load_bundle_file(self.standard(), self._schema_spec_file_json())
        return json.loads(data)

    def _schema_file_json_with_ver(self):
        filename = self._schema_file_with_ver()
        if filename is not None:
            return "json-schema/" + filename + ".json"
        return filename

    def _schema_file_with_ver(self):
        if self._schema_version:
            return self.schema_id() + "." + self.version()
        return None

    def _schema_spec_file_json(self):
        return "json-schema/" + self.schema_id() + ".json"

    def _schema_file_openapi(self):
        return "openapi/" + self.schema_id() + ".yaml"

    def _schema_file_csdl(self):
        return "csdl/" + self.schema_id() + "_v1.xml"

    @staticmethod
    def _load_csdl_file(standard: str, relativename):
        """
        Load CSDL schema from file from schemas budle subdir.
        """
        data = RedfishBase._load_bundle_file(standard, relativename)
        return minidom.parseString(data)

    @staticmethod
    def _load_bundle_file(standard: str, filename: str, schema_path=__RFG_PATH__):
        """
        Helper method to load file content from schemas bundle subdirs.
        """
        fullpath = path.join(schema_path, RedfishBase._bundle_relative_path(standard), filename)
        return RedfishBase._load_file_fullpath(fullname=fullpath)

    @staticmethod
    def _load_json_file(fullname):
        """
        Separate method to load json schema from any file.
        """
        data = RedfishBase._load_file_fullpath(fullname)
        return json.loads(data)

    @staticmethod
    def _load_file_fullpath(fullname):
        """
        Reads any file content.
        """
        with open(fullname) as f:
            data = f.read()
            return data

    @staticmethod
    def _write_file(destdir, filename, content):
        """
        Create dirs if not exist and writes file content.
        """
        if not path.isdir(destdir):
            makedirs(destdir, exist_ok=True)
        filename = path.join(destdir, filename)
        if isinstance(content, str):
            with open(filename, "w") as file:
                file.writelines(content)
        else:
            with open(filename, "wb") as file:
                file.write(content)

    @staticmethod
    def _version_from_filename(filename):
        parts = filename.split(".")
        if len(parts) > 1:
            return parts[1]
        return None

    @staticmethod
    def schema_and_version_from(filename):
        filename = path.basename(filename)
        filename = path.splitext(filename)[0]
        parts = filename.split(".")
        if len(parts) > 1:
            return parts[0], parts[1]
        return parts[0], None

    @staticmethod
    def _json_full_filename(schema, srcdir=__RFG_PATH__):
        fromname = RedfishBase._map_json_schema_file(schema.schema_id())
        from_name_version = fromname
        if schema._schema_version is not None:
            from_name_version += ".v" + schema._schema_version
        return path.join(
            srcdir,
            RedfishBase._bundle_relative_path(schema._standard),
            "json-schema",
            from_name_version + ".json")

    @staticmethod
    def _schema_file_csdl_by_name(schema_id):
        return schema_id + "_" + RedfishBase.redfish_version() + ".xml"

    @staticmethod
    def _map_json_schema_file(basename):
        schemas_map = RedfishBase._json_schema_file_map()
        if basename in schemas_map:
            return schemas_map[basename]
        else:
            return basename

    @staticmethod
    def _json_schema_file_map():
        return { "RedfishExtensions": "redfish-schema" }


    @staticmethod
    def _json_full_path(standard: str):
        return path.join(RedfishBase._bundle_full_path(standard), "json-schema")

    @staticmethod
    def _bundle_full_path(standard: str, srcdir=__RFG_PATH__):
        return path.join(srcdir, RedfishBase._bundle_relative_path(standard))

    @staticmethod
    def _bundle_csdl_full_path(standard: str, srcdir=__RFG_PATH__):
        if "swordfish" in standard:
            # bundle/scdl-schema for SNI Swordfish bundle zip
            return path.join(srcdir, RedfishBase._bundle_relative_path(standard), "csdl-schema")
        else:
            # bundle/scdl for DMTF bundle zip
            return path.join(srcdir, RedfishBase._bundle_relative_path(standard), "csdl")

    @staticmethod
    def _bundle_relative_path(standard: str):
        if "swordfish" in standard:
            return path.join("assets", "schemas", "swordfish", "bundle")
        else:
            return path.join("assets", "schemas", "bundle")

    @staticmethod
    def _oem_full_path(srcdir=__RFG_PATH__):
        return path.join(srcdir, "assets", "oem")

    @staticmethod
    def redfish_version():
        return "v1"

    @staticmethod
    def _json_schemas_relative_dir():
        return "JsonSchemas"

    @staticmethod
    def _formatting_classname(classname):
        return str("Redfish" + classname[0].upper() + classname[1:])
