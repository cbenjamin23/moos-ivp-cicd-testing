#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: first_draft
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

# Monte
XLAUNCHED="no"
NOGUI=""

# Vehicle defaults
START_POS="x=-80,y=-20,heading=90"
VDEST_POS="90,-20"
VNAME="abe"
VCOLOR="yellow"
STOCK_SPD="2.0"
MAX_SPD="2.5"
OBSTACLE_SPEC="pts={-5,-12:8,-12:8,-28:-5,-28},label=ob_0"

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
        echo "                                               "
        echo "Options (monte):                               "
        echo "  --xlaunched, -x    Launched by xlaunch       "
        echo "  --nogui, -ng       Headless launch, no gui   "
        echo "                                               "
        echo "Options (custom):                              "
        echo "  --start_pos=<X,Y,Hdg>  Sim start pos/hdg     "
        echo "  --vdest=<X,Y>          Destination x/y       "
        echo "  --stock_spd=<m/s>      Default vehicle speed "
        echo "  --max_spd=<m/s>        Max helm/sim speed    "
        echo "  --obstacle=<spec>      Static obstacle spec  "
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
        LOG_CLEAN=$ARGI
    elif [ "${ARGI}" = "--xlaunched" -o "${ARGI}" = "-x" ]; then
        XLAUNCHED="yes"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
        NOGUI="--nogui"
    elif [ "${ARGI:0:12}" = "--start_pos=" ]; then
        START_POS="${ARGI#--start_pos=*}"
    elif [ "${ARGI:0:8}" = "--vdest=" ]; then
        VDEST_POS="${ARGI#--vdest=*}"
    elif [ "${ARGI:0:12}" = "--stock_spd=" ]; then
        STOCK_SPD="${ARGI#--stock_spd=*}"
    elif [ "${ARGI:0:10}" = "--max_spd=" ]; then
        MAX_SPD="${ARGI#--max_spd=*}"
    elif [ "${ARGI:0:11}" = "--obstacle=" ]; then
        OBSTACLE_SPEC="${ARGI#--obstacle=*}"
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: If requested, clean logs and generated files first
#------------------------------------------------------------
if [ "${LOG_CLEAN}" != "" -a -f "clean.sh" ]; then
    ./clean.sh
fi

#------------------------------------------------------------
#  Part 5: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "============================================"
    echo "  $ME SUMMARY                               "
    echo "============================================"
    echo "CMD_ARGS =      [${CMD_ARGS}]               "
    echo "TIME_WARP =     [${TIME_WARP}]              "
    echo "JUST_MAKE =     [${JUST_MAKE}]              "
    echo "LOG_CLEAN =     [${LOG_CLEAN}]              "
    echo "XLAUNCHED =     [${XLAUNCHED}]              "
    echo "NOGUI =         [${NOGUI}]                  "
    echo "--------------------------------------------"
    echo "VNAME =         [${VNAME}]                  "
    echo "VCOLOR =        [${VCOLOR}]                 "
    echo "START_POS =     [${START_POS}]              "
    echo "VDEST_POS =     [${VDEST_POS}]              "
    echo "STOCK_SPD =     [${STOCK_SPD}]              "
    echo "MAX_SPD =       [${MAX_SPD}]                "
    echo "OBSTACLE_SPEC = [${OBSTACLE_SPEC}]          "
    echo "                                            "
    echo -n "Hit any key to continue launch           "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 6: Launch the vehicle
#------------------------------------------------------------
VARGS=" --sim --auto --max_spd=$MAX_SPD "
VARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE "
VARGS+=" --mport=9001 --pshare=9201 "
VARGS+=" --start_pos=$START_POS "
VARGS+=" --stock_spd=$STOCK_SPD "
VARGS+=" --vname=$VNAME "
VARGS+=" --vdest=$VDEST_POS "
VARGS+=" --color=$VCOLOR "
VARGS+=" --obstacle=$OBSTACLE_SPEC "
vecho "Launching vehicle: $VARGS"
./launch_vehicle.sh $VARGS

#------------------------------------------------------------
#  Part 7: Launch the shoreside mission file
#------------------------------------------------------------
SARGS=" --auto --mport=9000 --pshare=9200 $NOGUI --vnames=$VNAME "
SARGS+=" --obstacle=$OBSTACLE_SPEC "
SARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE "
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
    uMAC --paused targ_shoreside.moos
    trap "" SIGINT
    echo
    echo "$ME: Halting all apps"
    kill -- -$$
fi

exit 0
