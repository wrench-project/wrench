#!/bin/bash

TODO=""

while test $# -gt 0
do
    case "$1" in
        --USER) TODO="$TODO USER"
            ;;
        --DEVELOPER) TODO="$TODO DEVELOPER"
            ;;
        --INTERNAL) TODO="$TODO INTERNAL"
            ;;
        *) echo "Usage: $0 [--USER] [--DEVELOPER] [--INTERNAL]"
	   exit 1
            ;;
    esac
    shift
done

if [ -z "$TODO" ] ; then
	echo "Usage: $0 [--USER] [--DEVELOPER] [--INTERNAL]"
	exit 1
fi

for ITEM in $TODO ; do
	echo "Generating $ITEM doc..."
	DOXYFILE="/tmp/Doxyfile_$ITEM"
	SECTIONS=""
	if [ $ITEM == DEVELOPER ] ; then
	  SECTIONS="DEVELOPER"
	fi
	if [ $ITEM == INTERNAL ]; then
	  SECTIONS="DEVELOPER INTERNAL"
	fi
	OUTPUT_DIRECTORY="wrenchdoc_$ITEM"
	cat ./Doxyfile.in | sed "s/WRENCH_SECTIONS/$SECTIONS/" | sed "s/WRENCH_OUTPUT_DIRECTORY/\/tmp\/$OUTPUT_DIRECTORY/"> $DOXYFILE
	doxygen $DOXYFILE 1> /dev/null 2> /dev/null
	echo "$ITEM doc generated at /tmp/$OUTPUT_DIRECTORY/html/index.html"
done
