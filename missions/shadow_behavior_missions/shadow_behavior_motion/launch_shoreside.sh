#!/bin/bash
#------------------------------------------------------------
#   Script: launch_shoreside.sh
#  Mission: shadow_behavior_motion
#   Author: Charles Benjamin
#   LastEd: Apr 2026
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
JUST_MAKE="no"
VERBOSE=""
AUTO_LAUNCHED="no"
LAUNCH_GUI="yes"

IP_ADDR="localhost"
MOOS_PORT="9000"
PSHARE_PORT="9200"
MMOD=""
VNAMES="abe:ben"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]                        "
        echo "                                                 "
        echo "Options:                                         "
        echo "  --help, -h             Show this help message  "
        echo "  --just_make, -j        Only create targ files  "
        echo "  --verbose, -v          Verbose, confirm launch "
        echo "  --auto, -a             Script launch, no uMAC  "
        echo "  --nogui, -ng           Headless launch         "
        echo "                                                 "
        echo "  --ip=<localhost>       Shore default IP addr   "
        echo "  --mport=<9000>         Shore MOOSDB port       "
        echo "  --pshare=<9200>        Shore pShare listen     "
        echo "  --mmod=<mod>           Mission variation/mod   "
        echo "  --vnames=<abe:ben>     Vehicle community names "
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="yes"
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
        AUTO_LAUNCHED="yes"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-n" -o "${ARGI}" = "-ng" ]; then
        LAUNCH_GUI="no"
    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        IP_ADDR="${ARGI#--ip=*}"
    elif [ "${ARGI:0:8}" = "--mport=" ]; then
        MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD="${ARGI#--mmod=*}"
    elif [ "${ARGI:0:9}" = "--vnames=" ]; then
        VNAMES="${ARGI#--vnames=*}"
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
    echo "     launch_shoreside.sh SUMMARY            "
    echo "============================================"
    echo "$ME                               "
    echo "CMD_ARGS =      [${CMD_ARGS}]     "
    echo "TIME_WARP =     [${TIME_WARP}]    "
    echo "JUST_MAKE =     [${JUST_MAKE}]    "
    echo "AUTO_LAUNCHED = [${AUTO_LAUNCHED}]"
    echo "LAUNCH_GUI =    [${LAUNCH_GUI}]   "
    echo "----------------------------------"
    echo "IP_ADDR =       [${IP_ADDR}]      "
    echo "MOOS_PORT =     [${MOOS_PORT}]    "
    echo "PSHARE_PORT =   [${PSHARE_PORT}]  "
    echo "MMOD =          [${MMOD}]         "
    echo "VNAMES =        [${VNAMES}]       "
    echo -n "Hit any key to continue launching shoreside "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 5: Create the shoreside .moos file
#------------------------------------------------------------
NSFLAGS="--strict --force -x"
if [ "${AUTO_LAUNCHED}" = "no" ]; then
    NSFLAGS="--interactive --force -x"
fi

nsplug meta_shoreside.moos targ_shoreside.moos $NSFLAGS WARP=$TIME_WARP \
       IP_ADDR=$IP_ADDR             MOOS_PORT=$MOOS_PORT    \
       PSHARE_PORT=$PSHARE_PORT     LAUNCH_GUI=$LAUNCH_GUI  \
       MMOD=$MMOD                   VNAMES=$VNAMES          \
       AUTO_LAUNCHED=$AUTO_LAUNCHED

#------------------------------------------------------------
#  Part 6: Exit now if requested to only create targ files
#------------------------------------------------------------
if [ "${JUST_MAKE}" = "yes" ]; then
    echo "Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 7: Launch the Shoreside MOOS community
#------------------------------------------------------------
echo "Launching Shoreside MOOS Community. WARP="$TIME_WARP
pAntler targ_shoreside.moos >& /dev/null &
echo "Done Launching Shoreside Community"

#------------------------------------------------------------
#  Part 8: Unless auto-launched, launch uMAC until Ctrl-C
#------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
    exit 0
fi

uMAC targ_shoreside.moos
trap "" SIGINT
kill -- -$$
