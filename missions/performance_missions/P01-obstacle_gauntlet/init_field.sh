#!/bin/bash
#------------------------------------------------------------
#   Script: init_field.sh
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

ME=`basename "$0"`
VERBOSE=""
SCENARIO="baseline_field"
FIXED_DIR="$(cd "$(dirname "$0")" && pwd)/obstacles"

for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --verbose, -v      Verbose, confirm values"
        echo "  --scenario=<name>  baseline_field | dense_field"
        echo "                     endurance_random"
        exit 0
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    elif [ "${ARGI:0:11}" = "--scenario=" ]; then
        SCENARIO="${ARGI#--scenario=*}"
    else
        echo "$ME: Bad Arg: $ARGI. Exit Code 1."
        exit 1
    fi
done

vecho "Preparing vehicle field files for [$SCENARIO]"

echo "x=51.8,y=3.1,heading=239" > vpositions.txt
echo "140,-60"                 > vdestpos.txt
echo "2.0"                     > vspeeds.txt
echo "abe"                     > vnames.txt
echo "yellow"                  > vcolors.txt

case "$SCENARIO" in
    baseline_field)
        cp "$FIXED_DIR/baseline_field.txt" obstacles.txt
        ;;
    dense_field)
        cp "$FIXED_DIR/dense_field.txt" obstacles.txt
        ;;
    endurance_random)
        echo "obstacle file generated" > obstacles.txt
        gen_obstacles --poly=-43.3,-37.1:-17.6,-91.3:54.7,-57.1:29,-2.8 \
                      --min_size=5 --max_size=8 \
                      --amt=7 \
                      --min_range=12 >> obstacles.txt

        if [ $? != 0 ]; then
            echo "$ME Unable to Gen Obstacles for [$SCENARIO]. Exit code 2."
            exit 2
        fi
        ;;
    *)
        echo "$ME: Unknown mission scenario: [$SCENARIO]. Exit Code 2."
        exit 2
        ;;
esac

if [ "$SCENARIO" != "endurance_random" ] && [ ! -f obstacles.txt ]; then
    echo "$ME: Unable to stage fixed obstacle file for [$SCENARIO]. Exit Code 3."
    exit 3
fi

if [ "${VERBOSE}" != "" ]; then
    echo "--------------------------------------"
    echo "vpositions.txt:"; cat vpositions.txt
    echo "vdestpos.txt:";   cat vdestpos.txt
    echo "vspeeds.txt:";    cat vspeeds.txt
    echo "vnames.txt:";     cat vnames.txt
    echo "vcolors.txt:";    cat vcolors.txt
    echo "SCENARIO = $SCENARIO"
    echo "obstacles.txt:";  cat obstacles.txt
    echo -n "Hit any key to continue"
    read ANSWER
fi

exit 0
