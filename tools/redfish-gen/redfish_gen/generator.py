## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)

import os

import yaml
from datetime import date
from .redfish_node import RedfishNode
from .redfish_schema import RedfishSchema
from .globals import __BASE_PATH__


class Generator:
    nodes = []

    @staticmethod
    def process(rootdir, parent=None):
        new_parent_tmp = dict()
        for root, _, files in (next(os.walk(rootdir)),):
            for file in files:
                if file.endswith(".yaml"):
                    filename = os.path.join(root, file)
                    with open(filename) as f:
                        data = f.read()
                        y = yaml.safe_load(data)
                        node = Generator(parent, **y)
                        new_parent_tmp[node.instance.def_filename()
                                       ] = node.instance
                        Generator.nodes.append(node)

        for root, dirs, files in (next(os.walk(rootdir)),):
            for d in dirs:
                Generator.process(rootdir+"/"+d, parent=new_parent_tmp[d])

    def __init__(self, parent=None, **kwargs):
        self.instance = RedfishNode.build(parent=parent, **kwargs)

    def generate(self, loader):
        print(" - Generating Redfish node: " + self.instance.classname())
        year = date.today().year
        self.instance.validate()
        template = Generator.render(
            loader, "node.hpp.mako", instance=self.instance, year=year)
        Generator.__write_gen_file(self.instance.classname(), template)
        RedfishSchema.build("redfish", self.instance.schema_id(), self.instance.version(), resolved=True, schema_json=self.instance.schema)

    @staticmethod
    def get_node(schema_name):
        for n in Generator.nodes:
            if n.instance.schema_id() == schema_name:
                return n.instance
        return None

    @staticmethod
    def render(loader, template, **kwargs):
        t = loader.get_template(template)
        r = t.render(loader=loader, **kwargs)
        return r

    @staticmethod
    def generate_all(loader):
        for node in Generator.nodes:
            node.generate(loader=loader)
        RedfishSchema.generate_all_schema()

    @staticmethod
    def __write_gen_file(filename, content, basedir=__BASE_PATH__+"/src/redfish/generated/"):
        if not os.path.isdir(basedir):
            os.makedirs(basedir, exist_ok=True)
        filename = basedir + filename + ".hpp"
        with open(filename, "w") as file:
            file.writelines(content)
