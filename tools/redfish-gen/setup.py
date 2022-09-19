#!/usr/bin/env python3
from setuptools import find_packages, setup

setup(
    name="redfish_gen",
    version="1.0",
    packages=find_packages(),
    install_requires=["inflection", "mako", "pyyaml"],
    scripts=["redfish-gen", "redfish-gen-meson"],
    package_data={"obmc-yadro-webapp": ["templates/*.mako"]},
    url="https://github.com/YADRO-KNS/obmc-yadro-webapp",
    classifiers=["SPDX short identifier: Apache-2.0"],
)
