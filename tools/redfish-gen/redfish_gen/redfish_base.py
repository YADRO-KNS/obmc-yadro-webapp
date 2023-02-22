## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2023, KNS Group LLC (YADRO)

import json
from shutil import copy
from os import path, makedirs, walk
from xml.dom import minidom

from .globals import __RFG_PATH__
from .globals import __WWW_PATH__

class RedfishBase:
    """
    Redfish base class.
    Contains all common methods for RedfishNode and RedfishSchema classes.
    """

    @staticmethod
    def redfish_version():
        return "v1"

    @staticmethod
    def _json_schemas_relative_dir():
        return "JsonSchemas"

    @staticmethod
    def _load_file_fullpath(fullname):
        with open(fullname) as f:
            data = f.read()
            return data

    @staticmethod
    def _load_file(fullname, schema_path=__RFG_PATH__):
        file_path = path.join(schema_path, fullname)
        with open(file_path) as f:
            data = f.read()
            return data

    @staticmethod
    def _load_bundle_file(fullname):
        file_path = path.join("assets", "schemas", "bundle", fullname)
        return RedfishBase._load_file(file_path)

    @staticmethod
    def _load_csdl_file(fullname):
        data = RedfishBase._load_bundle_file(fullname)
        return minidom.parseString(data)


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
    def write_file(destdir, filename, content):
        if not path.isdir(destdir):
            makedirs(destdir, exist_ok=True)
        filename = path.join(destdir, filename)
        if isinstance(content, str):
            with open(filename, "w") as file:
                file.writelines(content)
        else:
            with open(filename, "wb") as file:
                file.write(content)

    def __init__(self, standard: str, schema_name, version) -> None:
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

    def _available_schema(self):
        if self.schema is not None:
            return self.schema
        return self.schema_spec

    def is_oem(self):
        return self._schema_name.startswith("Oem")

    def get_json_schema(self):
        return self.schema
