# script to generate wrench-init script. Takes one command-line argument, which is
# the CMAKE_CURRENT_SOURCE_DIR
if [ $# -ne 2 ]; then
        echo "Usage: $0 <src dir> <build dir>"
        exit 0
fi

#mkdir -p $2"/tools/" || true
#mkdir -p $2"/tools/wrench/" || true
mkdir -p $2"/tools/wrench/wrench-init/" 

# Redirect output
exec 1> $2"/tools/wrench/wrench-init/wrench-init"

echo "#!/usr/bin/env python3
#
# Copyright (c) 2019-2021. The WRENCH Team.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#"

echo "FILE_CONTENT_FINDSIMGRID_CMAKE = r\"\"\""
cat $1"/conf/cmake/FindSimGrid.cmake"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_FINDWRENCH_CMAKE = r\"\"\""
cat $1"/FindWRENCH.cmake"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_CMAKELISTS_TXT = r\"\"\""
cat $1"/tools/wrench/wrench-init/base_code/CMakeLists.txt"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_SIMULATOR_WORKFLOW_CPP = r\"\"\""
cat $1"/tools/wrench/wrench-init/base_code/Simulator_WORKFLOW.cpp"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_CONTROLLER_WORKFLOW_CPP = r\"\"\""
cat $1"/tools/wrench/wrench-init/base_code/Controller_WORKFLOW.cpp"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_CONTROLLER_WORKFLOW_H = r\"\"\""
cat $1"/tools/wrench/wrench-init/base_code/Controller_WORKFLOW.h"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_SIMULATOR_ACTION_CPP = r\"\"\""
cat $1"/tools/wrench/wrench-init/base_code/Simulator_ACTION.cpp"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_CONTROLLER_ACTION_CPP = r\"\"\""
cat $1"/tools/wrench/wrench-init/base_code/Controller_ACTION.cpp"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_CONTROLLER_ACTION_H = r\"\"\""
cat $1"/tools/wrench/wrench-init/base_code/Controller_ACTION.h"
echo "\"\"\""
echo ""

echo "FILE_CONTENT_PLATFORM_XML = r\"\"\""
cat $1"/tools/wrench/wrench-init/base_code/platform.xml"
echo "\"\"\""
echo ""

cat $1"/tools/wrench/wrench-init/wrench-init.in"



