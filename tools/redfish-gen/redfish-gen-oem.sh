#!/usr/bin/bash -eux

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

python3 ${SCRIPT_DIR}/../redfish_tools/csdl-to-json-convertor/csdl-to-json.py --input ${SCRIPT_DIR}/assets/oem/ --output ${SCRIPT_DIR}/assets/schemas/bundle/json-schema/