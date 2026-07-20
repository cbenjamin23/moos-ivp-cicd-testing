#!/bin/bash
#------------------------------------------------------------
#   Script: launch_shoreside.sh
#  Mission: obstacle_gauntlet
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
JUST_MAKE="no"
VERBOSE=""
AUTO_LAUNCHED="no"
LAUNCH_GUI="yes"

IP_ADDR="localhost"
MOOS_PORT="9000"
PSHARE_PORT="9200"
SCENARIO="baseline_field"
VNAMES="abe"
ENDURANCE_MODE="false"
DRESET="false"
ENDURANCE_TIMEOUT_SECS="1800"
ENDURANCE_TARGET_CYCLES="10"
FIELD_OBSTACLE_COUNT="0"
FIELD_FINGERPRINT="unknown"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME: [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --just_make, -j    Only create targ files"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo ""
        echo "  --auto, -a         Auto-launched by a script"
        echo "  --nogui, -n        Headless mode"
        echo ""
        echo "  --ip=<localhost>   Force pHostInfo IP address"
        echo "  --mport=<9000>     Shoreside MOOSDB port"
        echo "  --pshare=<9200>    Shoreside pShare port"
        echo "  --scenario=<name>  Mission scenario"
        echo "  --field_count=<n>  Staged obstacle polygon count"
        echo "  --field_fingerprint=<id>  Staged obstacle file fingerprint"
        echo "  --vnames=<vnames>  Colon-separated vehicle names"
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
    elif [ "${ARGI:0:11}" = "--scenario=" ]; then
        SCENARIO="${ARGI#--scenario=*}"
    elif [ "${ARGI:0:14}" = "--field_count=" ]; then
        FIELD_OBSTACLE_COUNT="${ARGI#--field_count=*}"
    elif [ "${ARGI:0:20}" = "--field_fingerprint=" ]; then
        FIELD_FINGERPRINT="${ARGI#--field_fingerprint=*}"
    elif [ "${ARGI:0:9}" = "--vnames=" ]; then
        VNAMES="${ARGI#--vnames=*}"
    else
        echo "$ME: Bad Arg:[$ARGI]. Exit Code 1."
        exit 1
    fi
done

case "$SCENARIO" in
    baseline_field|dense_field) ;;
    endurance_random)
        ENDURANCE_MODE="true"
        DRESET="true"
        ;;
    *)
        echo "$ME: Unknown scenario [$SCENARIO]"
        exit 1
        ;;
esac

case "$FIELD_OBSTACLE_COUNT" in
    ''|*[!0-9]*)
        echo "$ME: Invalid field obstacle count [$FIELD_OBSTACLE_COUNT]"
        exit 1
        ;;
esac
case "$FIELD_FINGERPRINT" in
    unknown) ;;
    sha256_????????????)
        FIELD_FINGERPRINT_HEX="${FIELD_FINGERPRINT#sha256_}"
        case "$FIELD_FINGERPRINT_HEX" in
            *[!0-9a-f]*)
                echo "$ME: Invalid field fingerprint [$FIELD_FINGERPRINT]"
                exit 1
                ;;
        esac
        ;;
    *)
        echo "$ME: Invalid field fingerprint [$FIELD_FINGERPRINT]"
        exit 1
        ;;
esac

#------------------------------------------------------------
#  Part 4: If not auto_launched and IP not explicitly set,
#          try to get it using ipaddrs.sh.
#------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "no" -a "${IP_ADDR}" = "localhost" ]; then
    MAYBE_IP_ADDR=`ipaddrs.sh --blunt`
    if [ $? = 0 ]; then
        IP_ADDR=$MAYBE_IP_ADDR
    fi
fi

#------------------------------------------------------------
#  Part 5: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" = "yes" ]; then
    echo "=================================="
    echo "  launch_shoreside.sh SUMMARY"
    echo "=================================="
    echo "$ME"
    echo "CMD_ARGS =      [${CMD_ARGS}]"
    echo "TIME_WARP =     [${TIME_WARP}]"
    echo "JUST_MAKE =     [${JUST_MAKE}]"
    echo "AUTO_LAUNCHED = [${AUTO_LAUNCHED}]"
    echo "----------------------------------"
    echo "IP_ADDR =       [${IP_ADDR}]"
    echo "MOOS_PORT =     [${MOOS_PORT}]"
    echo "PSHARE_PORT =   [${PSHARE_PORT}]"
    echo "LAUNCH_GUI =    [${LAUNCH_GUI}]"
    echo "SCENARIO =      [${SCENARIO}]"
    echo "ENDURANCE_MODE= [${ENDURANCE_MODE}]"
    echo "DRESET =        [${DRESET}]"
    echo "END_TIMEOUT =   [${ENDURANCE_TIMEOUT_SECS}]"
    echo "END_TGT_CYC =   [${ENDURANCE_TARGET_CYCLES}]"
    echo "FIELD_COUNT =   [${FIELD_OBSTACLE_COUNT}]"
    echo "FIELD_ID =      [${FIELD_FINGERPRINT}]"
    echo "----------------------------------"
    echo "VNAMES =        [${VNAMES}]"
    echo -n "Hit any key to continue launch "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 6: Create the shoreside mission file
#------------------------------------------------------------
NSFLAGS="--strict --force -x"
if [ "${AUTO_LAUNCHED}" = "no" ]; then
    NSFLAGS="--interactive --force -x"
fi

nsplug meta_shoreside.moos targ_shoreside.moos $NSFLAGS WARP=$TIME_WARP \
       IP_ADDR=$IP_ADDR             MOOS_PORT=$MOOS_PORT    \
       PSHARE_PORT=$PSHARE_PORT     LAUNCH_GUI=$LAUNCH_GUI  \
       MMOD=$SCENARIO               VNAMES=$VNAMES          \
       ENDURANCE_MODE=$ENDURANCE_MODE DRESET=$DRESET        \
       ENDURANCE_TIMEOUT_SECS=$ENDURANCE_TIMEOUT_SECS       \
       ENDURANCE_TARGET_CYCLES=$ENDURANCE_TARGET_CYCLES     \
       FIELD_OBSTACLE_COUNT=$FIELD_OBSTACLE_COUNT           \
       FIELD_FINGERPRINT=$FIELD_FINGERPRINT

if [ "${JUST_MAKE}" = "yes" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 7: Launch the shoreside MOOS community
#------------------------------------------------------------
echo "Launching Shoreside MOOS Community. WARP=$TIME_WARP"
pAntler targ_shoreside.moos >& /dev/null &
echo "Done Launching Shoreside Community"

#------------------------------------------------------------
#  Part 8: If launched from script, we're done, exit now
#------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
    exit 0
fi

#------------------------------------------------------------
#  Part 9: Launch uMAC until the mission is quit
#------------------------------------------------------------
uMAC targ_shoreside.moos

trap "" SIGINT
kill -- -$$
