## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)

import os

import mako.lookup
import redfish_gen.generator as gen

def main():
    module_path = os.path.dirname(gen.__file__)
    directories=[os.path.join(module_path, "templates")]
    lookup = mako.lookup.TemplateLookup(directories=directories)
    gen.Generator.process(os.path.dirname(module_path) + "/redfish")
    gen.Generator.generate_all(lookup)
