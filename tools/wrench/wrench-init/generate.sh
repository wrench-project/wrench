# Script to replace the string TO_REPLACE_FIND_SIMGRID_CMAKE_END_TO_REPLACE by the content of
# a file (which is typically the FindSimGrid.cmake file)

if [ -z $1 ]; then
        echo "Missing command-line argument to specify file path"
        exit 0
fi

# Replacing the required string by the file content, and deal with annoying backslashes
cat ./wrench-init.in | sed -e "/TO_REPLACE_FIND_SIMGRID_CMAKE_END_TO_REPLACE/r $1" -e "s///"  | sed -e "s/\\\\\\\\/\\\\\\\\\\\\\\\/" > ./wrench-init


