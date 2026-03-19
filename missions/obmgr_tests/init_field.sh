#!/bin/bash 
#------------------------------------------------------------
#   Script: init_field.sh
#   Author: Charles Benjamin
#   LastEd: Mar 19th 2026
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

# custom

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
	echo "                                               "
	echo "Options (custom):                              "
       exit 0;
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
	VERBOSE=$ARGI
    else 
	echo "$ME: Bad Arg: $ARGI. Exit Code 1."
	exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Source local coordinate grid if it exits
#------------------------------------------------------------

#------------------------------------------------------------
#  Part 5: Set starting positions, speeds, vnames, colors
#------------------------------------------------------------
vecho "Setting starting position, speeds, vnames, colors"

echo "x=0,y=0,heading=90"    >  vpositions.txt
echo "0,0"                   >  vdestpos.txt
echo "0.0"                   >  vspeeds.txt
echo "abe"                   >  vnames.txt
echo "yellow"                >  vcolors.txt

#------------------------------------------------------------
#  Part 6: Set other aspects of the field, e.g., obstacles
#------------------------------------------------------------

#------------------------------------------------------------
#  Part 7: If verbose, show file contents
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
