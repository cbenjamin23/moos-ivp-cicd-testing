#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: ufld_obstacle_sim_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output.
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=$(basename "$0")
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
TEARDOWN_HELPER="$REPO_DIR/scripts/moos_scoped_teardown.sh"
if [ ! -f "$TEARDOWN_HELPER" ]; then
    echo "launch.sh: Missing scoped teardown helper: $TEARDOWN_HELPER" >&2
    exit 1
fi
# shellcheck source=/dev/null
. "$TEARDOWN_HELPER"

stop_here() {
    moos_scoped_teardown_stop_root "$SCRIPT_DIR"
}

# shellcheck disable=SC2329  # Invoked through signal traps.
on_signal() {
    trap - INT TERM
    echo
    echo "$ME: Halting scoped mission apps"
    stop_here || true
    exit 130
}

trap on_signal INT TERM
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
LOG_MODE="minimal"
if [ "${LOG_MODE_PREPARED:-no}" = yes ] && [ -n "${LOG_MODE_PREPARED_VALUE:-}" ]; then
    LOG_MODE="$LOG_MODE_PREPARED_VALUE"
fi
MOOS_PORT="7600"
PSHARE_PORT="7610"
MMOD=""
XLAUNCHED="no"
NOGUI=""
SHORE_MOOS="meta_shoreside.moos"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [[ "${ARGI}" == "--help" || "${ARGI}" == "-h" ]]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h           Show this help message"
        echo "  --verbose, -v        Verbose, confirm launch"
        echo "  --just_make, -j      Only create targ files"
        echo "  --log_clean, -lc     Run clean.sh before launch"
        echo "  --log=<mode>         minimal (default) or full"
        echo "  --mport=N            MOOSDB port"
        echo "  --pshare=N           pShare port"
        echo "  --shore_mport=N      MOOSDB port alias for xlaunch"
        echo "  --shore_pshare=N     pShare port alias for xlaunch"
        echo "  --mmod=<mod>         Mission variation/mod"
        echo "  --xlaunched, -x      Launched by xlaunch"
        echo "  --nogui, -ng         Headless launch"
        exit 0
    elif [[ "${ARGI//[^0-9]/}" == "$ARGI" && "$TIME_WARP" == 1 ]]; then
        TIME_WARP=$ARGI
    elif [[ "${ARGI}" == "--verbose" || "${ARGI}" == "-v" ]]; then
        VERBOSE=$ARGI
    elif [[ "${ARGI}" == "--just_make" || "${ARGI}" == "-j" ]]; then
        JUST_MAKE=$ARGI
    elif [[ "${ARGI}" == "--log_clean" || "${ARGI}" == "-lc" ]]; then
        LOG_CLEAN=$ARGI
    elif [ "${ARGI:0:6}" = "--log=" ]; then
        LOG_MODE="${ARGI#--log=*}"
    elif [ "${ARGI:0:8}" = "--mport=" ]; then
        MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI:0:14}" = "--shore_mport=" ]; then
        MOOS_PORT="${ARGI#--shore_mport=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        PSHARE_PORT="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:12}" = "--veh_mport=" ]; then
        true
    elif [ "${ARGI:0:13}" = "--veh_pshare=" ]; then
        true
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD="${ARGI#--mmod=*}"
    elif [[ "${ARGI}" == "--xlaunched" || "${ARGI}" == "-x" ]]; then
        XLAUNCHED="yes"
    elif [[ "${ARGI}" == "--nogui" || "${ARGI}" == "-ng" ]]; then
        NOGUI="--nogui"
    else
        echo "$ME: Bad arg: $ARGI Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Optionally clean previous generated/run artifacts.
#------------------------------------------------------------
if [ "$LOG_CLEAN" != "" ]; then
    ./clean.sh
fi

case "$LOG_MODE" in
    minimal|full) ;;
    *) echo "$ME: --log must be minimal or full" >&2; exit 2 ;;
esac
if [ "${LOG_MODE_PREPARED:-no}" != yes ]; then
    ./prepare_logging_mode.sh "$LOG_MODE"
fi
export LOG_MODE_PREPARED=yes
export LOG_MODE_PREPARED_VALUE="$LOG_MODE"

#------------------------------------------------------------
#  Part 5: If verbose, show vars and confirm before launching.
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "============================================"
    echo "  $ME SUMMARY"
    echo "============================================"
    echo "CMD_ARGS =     [${CMD_ARGS}]"
    echo "TIME_WARP =    [${TIME_WARP}]"
    echo "JUST_MAKE =    [${JUST_MAKE}]"
    echo "LOG_MODE =     [${LOG_MODE}]"
    echo "MOOS_PORT =    [${MOOS_PORT}]"
    echo "PSHARE_PORT =  [${PSHARE_PORT}]"
    echo "MMOD =         [${MMOD}]"
    echo "XLAUNCHED =    [${XLAUNCHED}]"
    echo "NOGUI =        [${NOGUI}]"
    echo -n "Hit any key to continue launch "
    read -r -n 1 _
fi

#------------------------------------------------------------
#  Part 6: Generate target files.
#------------------------------------------------------------
NSFLAGS=(--strict --force -x)
if [ "${XLAUNCHED}" != "yes" ]; then
    NSFLAGS=(--interactive --force -x)
fi

if [ -f "meta_shoreside.moosx" ]; then
    SHORE_MOOS="meta_shoreside.moosx"
fi

nsplug "$SHORE_MOOS" targ_shoreside.moos "${NSFLAGS[@]}" \
       WARP="$TIME_WARP" MOOS_PORT="$MOOS_PORT" PSHARE_PORT="$PSHARE_PORT" \
       MMOD="$MMOD"

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 7: Launch the uFldObstacleSim community.
#------------------------------------------------------------
vecho "Launching uFldObstacleSim community"
pAntler targ_shoreside.moos >& /dev/null &

if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos || true
    trap - INT TERM
    if ! stop_here; then
        echo "$ME: Scoped mission teardown failed" >&2
        exit 1
    fi
fi

exit 0
