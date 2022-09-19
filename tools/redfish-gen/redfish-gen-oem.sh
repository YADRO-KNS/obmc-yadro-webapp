#!/usr/bin/bash -eux

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

CSDL_TO_JSON_PY=${SCRIPT_DIR}/../redfish_tools/csdl-to-json-convertor/csdl-to-json.py
INPUT_PATH=${SCRIPT_DIR}/assets/oem/
OUTPUT_PATH=${SCRIPT_DIR}/assets/schemas/bundle/json-schema/

python3 "${CSDL_TO_JSON_PY}" --input "${INPUT_PATH}" --output "${OUTPUT_PATH}"
