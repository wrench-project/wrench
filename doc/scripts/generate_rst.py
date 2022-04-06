#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (c) 2022 The WRENCH Team.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

import pathlib
import sys
import xml.etree.ElementTree as ET

docs_path = pathlib.Path(sys.argv[1])
src_path = pathlib.Path(sys.argv[2])
release_version = sys.argv[3]

with open(src_path.joinpath("conf.py"), "a") as f:
    f.write(f"version = '{release_version}'\n" \
            f"release = '{release_version}'\n")

for section in ["user", "developer", "internal"]:
    section_path = docs_path.joinpath(f"{section}/xml/index.xml")
    tree = ET.parse(section_path)
    root = tree.getroot()

    rst_folder = src_path.joinpath(f"api_{section}")
    rst_folder.mkdir(exist_ok=True)

    for child in root:
        if child.attrib["kind"] == "class":
            title = child.find('name').text
            ref = title.replace("wrench::", "")

            # create an .rst file for each class
            with open(rst_folder.joinpath(f"{child.attrib['refid']}.rst"), "w") as c:
                c.write(f".. _{ref}:\n{title}\n")
                line = ["*" for i in range(0, len(title))]
                c.write(f"{''.join(line)}\n\n")
                c.write(f".. doxygenclass:: {child.find('name').text}\n"
                        f"   :project: {section}\n"
                        "   :members:\n\n")
