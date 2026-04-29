#!/bin/bash 
#------------------------------------------------------------
#   Script: init_field.sh
#   Author: Charles Benjamin
#   LastEd: Apr 2026
#------------------------------------------------------------
#  Part 1: A convenience function for producing terminal
#          debugging/status output depending on verbosity.
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
VERBOSE=""

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS]                                "
        echo "                                               "
        echo "Options:                                       "
        echo "  --verbose, -v      Verbose, confirm values   "
        exit 0
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    else 
        echo "$ME: Bad Arg: $ARGI. Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Set starting positions, speeds, vnames, colors
#------------------------------------------------------------
vecho "Setting starting position, speeds, vnames, colors"

echo "x=0,y=-60,heading=90" > vpositions.txt
echo "70,-60"               > vdestpos.txt
echo "2.0"                  > vspeeds.txt
echo "abe"                  > vnames.txt
echo "yellow"               > vcolors.txt

#------------------------------------------------------------
#  Part 5: If verbose, show file contents
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "--------------------------------------"
    echo "--------------------------------------(pos/spd)"
    echo "vpositions.txt:"; cat  vpositions.txt
    echo "vspeeds.txt:";    cat  vspeeds.txt
    echo "--------------------------------------(vprops)"
    echo "vnames.txt:";     cat  vnames.txt
    echo "vcolors.txt:";    cat  vcolors.txt
    echo "--------------------------------------(custom)"
    echo "vdestpos.txt:";   cat  vdestpos.txt
    echo -n "Hit any key to continue"
    read ANSWER
fi

exit 0
