#!/bin/bash 
#--------------------------------------------------------------
#   Script: clean.sh
#   Author: Michael Benjamin
#     Date: November 2019
#--------------------------------------------------------------

#--------------------------------------------------------------
#  Part 1: Initialize global variables
#--------------------------------------------------------------
VERBOSE=""
RM_SEED_FILES="no"

#-------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#-------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ] ; then
	echo "clean.sh [SWITCHES]                         "
	echo "Switches:                                   " 
	echo "  --verbose                                 " 
	echo "  --help, -h                                " 
	echo "  --all, -a     Also remove the seed files. " 
	exit 0;
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ] ; then
	VERBOSE="-v"
    elif [ "${ARGI}" = "--all" -o "${ARGI}" = "-a" ] ; then
	RM_SEED_FILES="yes"
    else
	echo "Bad argument: "$ARGI
	exit 1
    fi
done


#-------------------------------------------------------
#  Part 3: Do the cleaning!
#-------------------------------------------------------
rm -rf  $VERBOSE LOG_*  XLOG_*  *~
rm -f   $VERBOSE targ_* .LastOpenedMOOSLogDirectory
rm -rf  $VERBOSE *_res_ufld_joust* vpositions.txt

if [ "${RM_SEED_FILES}" = "yes" ] ; then
    rm -f   $VERBOSE seeds*.txt joust.txt dock_start.txt
fi

exit 0
