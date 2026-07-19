#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: ufield_comms_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

ME=`basename "$0"`
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
LOG_MODE="minimal"
if [ "${LOG_MODE_PREPARED:-no}" = yes ] && [ -n "${LOG_MODE_PREPARED_VALUE:-}" ]; then
    LOG_MODE="$LOG_MODE_PREPARED_VALUE"
fi
MMOD=""
SHORE_MPORT="4000"
VEH1_MPORT="4001"
VEH2_MPORT="4002"
SHORE_PSHARE="4010"
VEH1_PSHARE="4011"
VEH2_PSHARE="4012"
XLAUNCHED="no"
NOGUI=""

for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]                      "
        echo "  --help, -h         Show this help message    "
        echo "  --verbose, -v      Verbose, confirm launch   "
        echo "  --just_make, -j    Only create targ files    "
        echo "  --log_clean, -lc   Run clean.sh bef launch   "
        echo "  --log=<mode>       minimal (default) or full "
        echo "  --mmod=<mod>       Mission variation/mod     "
        echo "  --shore_mport=N    Shoreside MOOSDB port     "
        echo "  --veh1_mport=N     Vehicle 1 MOOSDB port     "
        echo "  --veh2_mport=N     Vehicle 2 MOOSDB port     "
        echo "  --shore_pshare=N   Shoreside pShare port     "
        echo "  --veh1_pshare=N    Vehicle 1 pShare port     "
        echo "  --veh2_pshare=N    Vehicle 2 pShare port     "
        echo "  --xlaunched, -x    Launched by xlaunch       "
        echo "  --nogui, -ng       Headless launch, no GUI   "
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
        LOG_CLEAN=$ARGI
    elif [ "${ARGI:0:6}" = "--log=" ]; then
        LOG_MODE="${ARGI#--log=*}"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD=$ARGI
    elif [ "${ARGI:0:14}" = "--shore_mport=" ]; then
        SHORE_MPORT="${ARGI#--shore_mport=*}"
    elif [ "${ARGI:0:13}" = "--veh1_mport=" ]; then
        VEH1_MPORT="${ARGI#--veh1_mport=*}"
    elif [ "${ARGI:0:13}" = "--veh2_mport=" ]; then
        VEH2_MPORT="${ARGI#--veh2_mport=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:14}" = "--veh1_pshare=" ]; then
        VEH1_PSHARE="${ARGI#--veh1_pshare=*}"
    elif [ "${ARGI:0:14}" = "--veh2_pshare=" ]; then
        VEH2_PSHARE="${ARGI#--veh2_pshare=*}"
    elif [ "${ARGI}" = "--xlaunched" -o "${ARGI}" = "-x" ]; then
        XLAUNCHED="yes"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
        NOGUI="--nogui"
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

case "$LOG_MODE" in
    minimal|full) ;;
    *) echo "$ME: --log must be minimal or full" >&2; exit 2 ;;
esac
if [ "${LOG_MODE_PREPARED:-no}" != yes ]; then
    ./prepare_logging_mode.sh "$LOG_MODE"
fi
export LOG_MODE_PREPARED=yes
export LOG_MODE_PREPARED_VALUE="$LOG_MODE"

./init_field.sh $VERBOSE

VEHPOS=(`cat vpositions.txt`)
VNAMES=(`cat vnames.txt`)
VCOLORS=(`cat vcolors.txt`)
VGROUPS=(`cat vgroups.txt`)

if [ "${VERBOSE}" != "" ]; then
    echo "============================================"
    echo "  $ME SUMMARY                   (ALL)       "
    echo "============================================"
    echo "CMD_ARGS =      [${CMD_ARGS}]               "
    echo "TIME_WARP =     [${TIME_WARP}]              "
    echo "JUST_MAKE =     [${JUST_MAKE}]              "
    echo "LOG_MODE =      [${LOG_MODE}]               "
    echo "MMOD =          [${MMOD}]                   "
    echo "SHORE_MPORT =   [${SHORE_MPORT}]            "
    echo "VEH1_MPORT =    [${VEH1_MPORT}]             "
    echo "VEH2_MPORT =    [${VEH2_MPORT}]             "
    echo "SHORE_PSHARE =  [${SHORE_PSHARE}]           "
    echo "VEH1_PSHARE =   [${VEH1_PSHARE}]            "
    echo "VEH2_PSHARE =   [${VEH2_PSHARE}]            "
    echo "VNAMES =        [${VNAMES[*]}]              "
    echo "START_POS =     [${VEHPOS[*]}]              "
    echo "VGROUPS =       [${VGROUPS[*]}]             "
    echo "XLAUNCHED =     [${XLAUNCHED}]              "
    echo "NOGUI =         [${NOGUI}]                  "
    echo -n "Hit any key to continue launch           "
    read ANSWER
fi

parse_pos() {
    local pos="$1"
    local key="$2"
    echo "$pos" | tr ',' '\n' | sed -n "s/^$key=//p"
}

SARGS=" --auto --mport=$SHORE_MPORT --pshare=$SHORE_PSHARE $NOGUI "
SARGS+=" --vnames=${VNAMES[0]}:${VNAMES[1]} $TIME_WARP $JUST_MAKE $VERBOSE $MMOD "
vecho "Launching shoreside: $SARGS"
./launch_shoreside.sh $SARGS
sleep 1

VARGS=" --auto $MMOD $TIME_WARP $JUST_MAKE $VERBOSE "

vecho "Launching vehicle: ${VNAMES[0]}"
./launch_vehicle.sh $VARGS --mport=$VEH1_MPORT --pshare=$VEH1_PSHARE \
    --shore_pshare=$SHORE_PSHARE --vname=${VNAMES[0]} \
    --color=${VCOLORS[0]} --vgroup=${VGROUPS[0]} \
    --xpos=`parse_pos ${VEHPOS[0]} x` --ypos=`parse_pos ${VEHPOS[0]} y` \
    --hdg=`parse_pos ${VEHPOS[0]} heading`

vecho "Launching vehicle: ${VNAMES[1]}"
./launch_vehicle.sh $VARGS --mport=$VEH2_MPORT --pshare=$VEH2_PSHARE \
    --shore_pshare=$SHORE_PSHARE --vname=${VNAMES[1]} \
    --color=${VCOLORS[1]} --vgroup=${VGROUPS[1]} \
    --xpos=`parse_pos ${VEHPOS[1]} x` --ypos=`parse_pos ${VEHPOS[1]} y` \
    --hdg=`parse_pos ${VEHPOS[1]} heading`
sleep 0.5

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos || true
    trap "" SIGINT
    kill -- -$$
fi

exit 0
