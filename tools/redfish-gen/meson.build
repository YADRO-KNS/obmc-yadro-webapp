## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)

redfish_gen_oem_prog = find_program('redfish-gen-oem.sh', required: true)
meson.add_postconf_script(redfish_gen_oem_prog.path())

redfish_gen_prog = find_program('.redfish-gen', required: true)
meson.add_postconf_script(redfish_gen_prog.path())
