#!/bin/bash
#----------------------------------------------------------
#  Script: zlaunch.sh
#  Mission: first_draft
#----------------------------------------------------------
#  Part 1: Declare global var defaults
#----------------------------------------------------------
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP=10
MAX_TIME=180
VERBOSE=""
JUST_MAKE=""
FLOW_DOWN_ARGS=""

#----------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#----------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]                      "
        echo "  --help, -h      Show this help message       "
        echo "  --verbose, -v   Enable verbose mode          "
        echo "  --just_make, -j Only create targ files       "
        echo "  --max_time=N    Max DB time for uMayFinish   "
        echo "  --nogui         Run headless                 "
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="--verbose"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="--just_make"
    elif [ "${ARGI:0:11}" = "--max_time=" ]; then
        MAX_TIME="${ARGI#--max_time=*}"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
        FLOW_DOWN_ARGS+="--nogui "
    else
        echo "$ME Bad arg:" $ARGI "Exiting with code: 1"
        exit 1
    fi
done

#----------------------------------------------------------
#  Part 3: Clean logs and generated files
#----------------------------------------------------------
./clean.sh

#----------------------------------------------------------
#  Part 4: Build args, run xlaunch, then ensure clean exit
#----------------------------------------------------------
FLOW_DOWN_ARGS+="--max_time=$MAX_TIME $JUST_MAKE $VERBOSE $TIME_WARP "
echo "$ME FLOW_DOWN_ARGS:[$FLOW_DOWN_ARGS]"

if [ "$JUST_MAKE" = "" ]; then
    : > results.txt
fi

/Users/charlesbenjamin/moos-ivp/scripts/xlaunch.sh $FLOW_DOWN_ARGS

if [ "$JUST_MAKE" = "" ]; then
    for I in $(seq 1 10); do
        if grep -q "grade=" results.txt 2>/dev/null; then
            break
        fi
        sleep 1
    done
    if [ -f results.txt ]; then
        grep -v '^[[:space:]]*$' results.txt > results.txt.tmp || true
        mv results.txt.tmp results.txt
    fi
    repo_dir=`git -C "$PWD" rev-parse --show-toplevel 2>/dev/null`
    teardown_helper="$repo_dir/scripts/moos_scoped_teardown.sh"
    if [ "$repo_dir" = "" ] || [ ! -f "$teardown_helper" ]; then
        echo "$ME: Missing scoped teardown helper: $teardown_helper"
        exit 1
    fi
    # shellcheck source=/dev/null
    source "$teardown_helper"
    if ! moos_scoped_teardown_stop_root "$PWD"; then
        echo "$ME: Scoped teardown failed for $PWD" >&2
        exit 1
    fi
    sleep 2
fi
