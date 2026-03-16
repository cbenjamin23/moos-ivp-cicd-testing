#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: cmgr_smoke
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
MAX_SPD="3"
MMOD=""

# Monte
XLAUNCHED="no"
NOGUI=""

# Custom

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "$ME [OPTIONS] [time_warp]                      "
	echo "                                               "
	echo "Options:                                       "
	echo "  --help, -h         Show this help message    " 
	echo "  --verbose, -v      Verbose, confirm launch   "
	echo "  --just_make, -j    Only create targ files    " 
	echo "  --log_clean, -lc   Run clean.sh bef launch   " 
	echo "  --max_spd=N        Max helm/sim speed        "
        echo "  --mmod=<mod>       Mission variation/mod     "
	echo "                                               "
	echo "Options (monte):                               "
	echo "  --xlaunched, -x    Launched by xlaunch       "
	echo "  --nogui, -ng       Headless launch, no gui   "
	exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then 
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
	VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
	JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
	LOG_CLEAN=$ARGI
    elif [ "${ARGI:0:10}" = "--max_spd=" ]; then
        MAX_SPD="${ARGI#--max_spd=*}"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD=$ARGI
    elif [ "${ARGI}" = "--xlaunched" -o "${ARGI}" = "-x" ]; then
	XLAUNCHED="yes"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
	NOGUI="--nogui"
    else 
	echo "$ME: Bad arg:" $ARGI "Exit Code 1."
	exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Set starting positions, speeds, vnames, colors
#------------------------------------------------------------
INIT_VARS=" $VERBOSE "
./init_field.sh $INIT_VARS

VEHPOS=(`cat vpositions.txt`)
SPEEDS=(`cat vspeeds.txt`)
VNAMES=(`cat vnames.txt`)
VCOLOR=(`cat vcolors.txt`)
DESTPOS=(`cat vdestpos.txt`)

#------------------------------------------------------------
#  Part 5: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "============================================"
    echo "  $ME SUMMARY                   (ALL)       "
    echo "============================================"
    echo "CMD_ARGS =      [${CMD_ARGS}]               "
    echo "TIME_WARP =     [${TIME_WARP}]              "
    echo "JUST_MAKE =     [${JUST_MAKE}]              "
    echo "LOG_CLEAN =     [${LOG_CLEAN}]              "
    echo "MAX_SPD =       [${MAX_SPD}]                "
    echo "MMOD =          [${MMOD}]                   "
    echo "--------------------------------(VProps)----"
    echo "VNAMES =        [${VNAMES[*]}]              "
    echo "VCOLORS =       [${VCOLOR[*]}]              "
    echo "START_POS =     [${VEHPOS[*]}]              "
    echo "DEST_POS  =     [${DESTPOS[*]}]             "
    echo "--------------------------------(Monte)-----"
    echo "XLAUNCHED =     [${XLAUNCHED}]              "
    echo "NOGUI =         [${NOGUI}]                  "
    echo "                                            "
    echo -n "Hit any key to continue launch           "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 6: Launch the Vehicle
#------------------------------------------------------------
VARGS=" --sim --auto --max_spd=$MAX_SPD $MMOD "
VARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE "
VARGS+=" --mport=9001 --pshare=9201 "
VARGS+=" --start_pos=${VEHPOS[0]} "
VARGS+=" --stock_spd=${SPEEDS[0]} "
VARGS+=" --vname=${VNAMES[0]} "
VARGS+=" --color=${VCOLOR[0]} "
VARGS+=" --destpos=${DESTPOS[0]}"
vecho "Launching vehicle: $VARGS"
./launch_vehicle.sh $VARGS
sleep 0.5

#------------------------------------------------------------
#  Part 7: Launch the Shoreside mission file
#------------------------------------------------------------
SARGS=" --auto --mport=9000 --pshare=9200 $NOGUI --vnames=${VNAMES[0]} "
SARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE "
SARGS+=" $MMOD "
vecho "Launching shoreside: $SARGS"
./launch_shoreside.sh $SARGS

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 8: Unless auto-launched, launch uMAC until mission quit
#------------------------------------------------------------
if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos || true
    trap "" SIGINT
    kill -- -$$
fi

exit 0
