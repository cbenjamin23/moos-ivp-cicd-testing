#!/bin/bash
#--------------------------------------------------------------
#   Script: clean.sh
#   Author: Charles Benjamin
#     Date: May 2026
#----------------------------------------------------------
VERBOSE=""

for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ] ; then
        echo "clean.sh [SWITCHES]        "
        echo "  --verbose                "
        echo "  --help, -h               "
        exit 0
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ] ; then
        VERBOSE="-v"
    else
        echo "clean.sh: Bad Arg:[$ARGI]. Exit Code 1."
        exit 1
    fi
done

if [ "${VERBOSE}" = "-v" ]; then
    echo "Cleaning: $PWD"
fi

rm -rf  $VERBOSE   MOOSLog_* XLOG_* LOG_*
rm -f   $VERBOSE   *~ *.moos++ targ_* tmp_*
rm -f   $VERBOSE   *.moosx *.bhvx
rm -f   $VERBOSE   .LastOpenedMOOSLogDirectory
