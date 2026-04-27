#!/bin/bash
#------------------------------------------------------------
#   Script: init_field.sh
#   Author: Charles Benjamin
#   LastEd: Apr 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
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
        echo "$ME [OPTIONS]                      "
        echo "                                   "
        echo "Options:                           "
        echo "  --help, -h       Show this help  "
        echo "  --verbose, -v    Verbose output  "
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
vecho "Setting CutRange behavior field"

echo "x=-70,y=-80,heading=90" >  vpositions.txt
echo "x=20,y=-80,heading=90"  >> vpositions.txt

echo "abe" >  vnames.txt
echo "ben" >> vnames.txt

echo "yellow" >  vcolors.txt
echo "white"  >> vcolors.txt

echo "CHASER" >  vroles.txt
echo "TARGET" >> vroles.txt

echo "3.0" >  vspeeds.txt
echo "1.2" >> vspeeds.txt

#------------------------------------------------------------
#  Part 5: If verbose, show file contents
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "--------------------------------------"
    echo "vpositions.txt:"; cat vpositions.txt
    echo "vnames.txt:";     cat vnames.txt
    echo "vcolors.txt:";    cat vcolors.txt
    echo "vroles.txt:";     cat vroles.txt
    echo "vspeeds.txt:";    cat vspeeds.txt
    echo -n "Hit any key to continue"
    read ANSWER
fi

exit 0
