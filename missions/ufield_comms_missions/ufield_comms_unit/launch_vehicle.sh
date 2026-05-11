#!/bin/bash
#------------------------------------------------------------
#   Script: launch_vehicle.sh
#  Mission: ufield_comms_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

#------------------------------------------------------------
#  Part 2: Declare global var defaults
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN="no"
AUTO_LAUNCHED="no"

IP_ADDR="localhost"
MOOS_PORT="4001"
PSHARE_PORT="4011"
SHORE_IP="localhost"
SHORE_PSHARE="4010"
MMOD=""

VNAME="abe"
COLOR="yellow"
XPOS="0"
YPOS="0"
HDG="90"
VGROUP="red"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]                        "
        echo "                                                 "
        echo "Options:                                         "
        echo "  --help, -h             Show this help message  "
        echo "  --just_make, -j        Only create targ files  "
        echo "  --verbose, -v          Verbose, confirm launch "
        echo "  --log_clean, -lc       Run clean.sh bef launch "
        echo "  --auto, -a             Script launch, no uMAC  "
        echo "  --ip=<localhost>       Veh default IP addr     "
        echo "  --mport=<4001>         Veh MOOSDB port         "
        echo "  --pshare=<4011>        Veh pShare listen port  "
        echo "  --shore=<localhost>    Shoreside IP to try     "
        echo "  --shore_pshare=<4010>  Shoreside pShare port   "
        echo "  --mmod=<mod>           Mission variation/mod   "
        echo "  --vname=<abe>          Vehicle name            "
        echo "  --color=<yellow>       Vehicle color           "
        echo "  --xpos=<0>             Node report X position  "
        echo "  --ypos=<0>             Node report Y position  "
        echo "  --hdg=<90>             Node report heading     "
        echo "  --vgroup=<red>         Node report group       "
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="yes"
    elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
        LOG_CLEAN="yes"
    elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
        AUTO_LAUNCHED="yes"
    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        IP_ADDR="${ARGI#--ip=*}"
    elif [ "${ARGI:0:8}" = "--mport=" ]; then
        MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI:0:8}" = "--shore=" ]; then
        SHORE_IP="${ARGI#--shore=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD="${ARGI#--mmod=*}"
    elif [ "${ARGI:0:8}" = "--vname=" ]; then
        VNAME="${ARGI#--vname=*}"
    elif [ "${ARGI:0:8}" = "--color=" ]; then
        COLOR="${ARGI#--color=*}"
    elif [ "${ARGI:0:7}" = "--xpos=" ]; then
        XPOS="${ARGI#--xpos=*}"
    elif [ "${ARGI:0:7}" = "--ypos=" ]; then
        YPOS="${ARGI#--ypos=*}"
    elif [ "${ARGI:0:6}" = "--hdg=" ]; then
        HDG="${ARGI#--hdg=*}"
    elif [ "${ARGI:0:9}" = "--vgroup=" ]; then
        VGROUP="${ARGI#--vgroup=*}"
    else
        echo "$ME: Bad Arg:[$ARGI]. Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" = "yes" ]; then
    echo "============================================"
    echo "     launch_vehicle.sh SUMMARY        $VNAME"
    echo "============================================"
    echo "CMD_ARGS =      [${CMD_ARGS}]     "
    echo "TIME_WARP =     [${TIME_WARP}]    "
    echo "JUST_MAKE =     [${JUST_MAKE}]    "
    echo "AUTO_LAUNCHED = [${AUTO_LAUNCHED}]"
    echo "IP_ADDR =       [${IP_ADDR}]      "
    echo "MOOS_PORT =     [${MOOS_PORT}]    "
    echo "PSHARE_PORT =   [${PSHARE_PORT}]  "
    echo "SHORE_IP =      [${SHORE_IP}]     "
    echo "SHORE_PSHARE =  [${SHORE_PSHARE}] "
    echo "MMOD =          [${MMOD}]         "
    echo "VNAME =         [${VNAME}]        "
    echo "XPOS =          [${XPOS}]         "
    echo "YPOS =          [${YPOS}]         "
    echo "HDG =           [${HDG}]          "
    echo "VGROUP =        [${VGROUP}]       "
    echo -n "Hit any key to continue launching $VNAME "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 5: If Log clean before launch, do it now.
#------------------------------------------------------------
if [ "$LOG_CLEAN" = "yes" -a -f "clean.sh" ]; then
    vecho "Cleaning local Log Files"
    ./clean.sh
fi

#------------------------------------------------------------
#  Part 6: Create the .moos file.
#------------------------------------------------------------
NSFLAGS="--strict --force -x"
if [ "${AUTO_LAUNCHED}" = "no" ]; then
    NSFLAGS="--interactive --force -x"
fi

nsplug meta_vehicle.moos targ_$VNAME.moos $NSFLAGS WARP=$TIME_WARP \
       IP_ADDR=$IP_ADDR             MOOS_PORT=$MOOS_PORT \
       PSHARE_PORT=$PSHARE_PORT     SHORE_IP=$SHORE_IP   \
       SHORE_PSHARE=$SHORE_PSHARE   VNAME=$VNAME         \
       COLOR=$COLOR                 XPOS=$XPOS           \
       YPOS=$YPOS                   HDG=$HDG             \
       VGROUP=$VGROUP               MMOD=$MMOD

if [ "${JUST_MAKE}" = "yes" ]; then
    echo "Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 7: Launch the vehicle mission
#------------------------------------------------------------
echo "Launching $VNAME MOOS Community. WARP="$TIME_WARP
pAntler targ_$VNAME.moos >& /dev/null &
echo "Done Launching $VNAME MOOS Community"

#------------------------------------------------------------
#  Part 8: Unless auto-launched, launch uMAC until Ctrl-C
#------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
    exit 0
fi

uMAC targ_$VNAME.moos
trap "" SIGINT
kill -- -$$
