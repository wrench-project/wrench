#!/bin/bash

CMD=""
GH_DOCS=""

while test $# -gt 0
do
    case "$1" in
        --USER) CMD="$CMD USER"
            ;;
        --DEVELOPER) CMD="$CMD DEVELOPER"
            ;;
        --INTERNAL) CMD="$CMD INTERNAL"
            ;;
        --GH) GH_DOCS="GH_DOCS"
            ;;
        *) echo "Usage: $0 [--USER] [--DEVELOPER] [--INTERNAL]"
	   exit 1
            ;;
    esac
    shift
done

if [ -z "$CMD" ] ; then
	echo "Usage: $0 [--USER] [--DEVELOPER] [--INTERNAL]"
	exit 1
fi

# WRENCH version
VERSION=$(<./.version)
mkdir -p docs/$VERSION/logs

for ITEM in $CMD ; do
	echo "Generating $ITEM doc..."
	OUTPUT_DIRECTORY=$(echo $ITEM | tr '[:upper:]' '[:lower:]')
	DOXYFILE="docs/$VERSION/logs/Doxyfile_$ITEM"
	SECTIONS=""
	if [ $ITEM == DEVELOPER ] ; then
	  SECTIONS="DEVELOPER"
	fi
	if [ $ITEM == INTERNAL ]; then
	  SECTIONS="DEVELOPER INTERNAL"
	fi
	< ./Doxyfile.in sed "s/WRENCH_SECTIONS/$SECTIONS/" | sed "s/WRENCH_OUTPUT_DIRECTORY/\docs\/$VERSION\/$OUTPUT_DIRECTORY/"> $DOXYFILE
	doxygen $DOXYFILE &> $DOXYFILE.log
	if [[ ! -z "$GH_DOCS" ]] ; then
	  mkdir -p ./docs/gh-pages/$VERSION/$OUTPUT_DIRECTORY
	  cp -R ./docs/$VERSION/$OUTPUT_DIRECTORY/html/*  ./docs/gh-pages/$VERSION/$OUTPUT_DIRECTORY
	fi
	echo "$ITEM doc generated at docs/$VERSION/$OUTPUT_DIRECTORY/html/index.html"
done
