#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: Apr 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="80"
NOGUI="--nogui"
CASE=""

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/opregion_missions/opregion_motion"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_MOOS_STEM="$MISSION_DIR/meta_vehicle.moos"
VEHICLE_BHV_STEM="$MISSION_DIR/meta_vehicle.bhv"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_MOOS_XFILE="$MISSION_DIR/meta_vehicle.moosx"
VEHICLE_BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --case=<name>      Run one named case"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=save_recover_pass"
        echo "  ./zlaunch.sh --case=halt_breach_fail"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="yes"
    elif [ "${ARGI:0:11}" = "--max_time=" ]; then
        MAX_TIME="${ARGI#--max_time=*}"
    elif [ "${ARGI:0:7}" = "--case=" ]; then
        CASE="${ARGI#--case=*}"
    elif [ "${ARGI}" = "--gui" ]; then
        NOGUI=""
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Set convenience functions for managing x-files
#          and per-run cleanup.
#------------------------------------------------------------
clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_MOOS_XFILE" "$VEHICLE_BHV_XFILE"
}

cleanup() {
    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        ./clean.sh >/dev/null 2>&1 || true
        stop_mission_apps
    fi
    cd "$start_dir"
}

stop_mission_apps() {
    local app
    local pid
    local cwd
    local apps
    apps="MOOSDB pRealm pLogger uProcessWatch pShare pHostInfo"
    apps="$apps uFldShoreBroker uFldNodeComms uTimerScript pMissionEval"
    apps="$apps pAutoPoke pMarineViewer pMissionHash uFldNodeBroker pHelmIvP"
    apps="$apps uSimMarineV22 pMarinePIDV22"

    for app in $apps; do
        for pid in `pgrep -x "$app" 2>/dev/null`; do
            cwd=`lsof -a -p "$pid" -d cwd -Fn 2>/dev/null | sed -n 's/^n//p'`
            if [ "$cwd" = "$MISSION_DIR" ]; then
                kill "$pid" >/dev/null 2>&1 || true
            fi
        done
    done
}

#------------------------------------------------------------
#  Part 5: Determine the expected outcome and patch files
#          for one named case.
#------------------------------------------------------------
get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "inside_region_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "save_recover_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/save-recover-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/save-recover-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "save_dist_buffer_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/save-dist-buffer-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/save-dist-buffer-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "save_dist_buffer_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/save-dist-buffer-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/save-dist-buffer-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "halt_breach_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/halt-breach-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/halt-breach-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "entry_gate_start_outside_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/entry-gate-start-outside-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/entry-gate-start-outside-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "entry_gate_disabled_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/entry-gate-disabled-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/entry-gate-disabled-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "trigger_exit_debounce_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/trigger-exit-debounce-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/trigger-exit-debounce-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "trigger_exit_strict_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/trigger-exit-strict-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/trigger-exit-strict-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "trigger_entry_delay_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/trigger-entry-delay-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/trigger-entry-delay-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "reset_before_exit_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/reset-before-exit-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/reset-before-exit-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/reset-before-exit-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "max_time_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/max-time-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/max-time-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "dynamic_region_expand_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/dynamic-region-expand-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/dynamic-region-expand-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/dynamic-region-expand-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "dynamic_region_update_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/dynamic-region-update-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/dynamic-region-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/dynamic-region-update-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "dynamic_region_halt_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/dynamic-region-halt-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/dynamic-region-halt-fail-vehicle.xmoos"
    else
        echo "$ME: Unknown case: [$CASE_NAME]"
        return 1
    fi
    return 0
}

#------------------------------------------------------------
#  Part 6: Apply case-specific nspatch overlays.
#------------------------------------------------------------
apply_case_patches() {
    clear_xfiles

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    fi

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_MOOS_STEM" "$VEH_MOOS_PATCH" --targ="$VEHICLE_MOOS_XFILE"
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_BHV_STEM" "$VEH_BHV_PATCH" --targ="$VEHICLE_BHV_XFILE"
    fi
}

#------------------------------------------------------------
#  Part 7: Execute one case and append its summary line.
#------------------------------------------------------------
run_case() {
    local case_name="$1"
    local line
    local actual
    local status
    local launch_rc
    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME --mmod=$case_name $TIME_WARP"
    if [ "$NOGUI" != "" ]; then
        XARGS="$XARGS $NOGUI"
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
        XARGS="$XARGS --just_make"
    fi

    vecho "Running case [$case_name] with xlaunch args: $XARGS"
    xlaunch.sh $XARGS
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR"
        if [ "$launch_rc" = 0 ]; then
            return 0
        fi
        return 1
    fi

    sleep 1

    line=`tail -n 1 results.txt 2>/dev/null`
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$launch_rc" != 0 ]; then
        status="error"
        actual="script_error"
        ALL_OK="no"
    elif [ "$actual" = "missing" ]; then
        status="error"
        ALL_OK="no"
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  launch_rc=$launch_rc  $line" >> "$RESULTS_FILE"
    else
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" >> "$RESULTS_FILE"
    fi
    cd "$HARNESS_DIR"
}

trap cleanup EXIT

#------------------------------------------------------------
#  Part 8: Select the case set, run the matrix, and report.
#------------------------------------------------------------
if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES="inside_region_pass save_recover_pass save_dist_buffer_pass save_dist_buffer_fail halt_breach_fail entry_gate_start_outside_pass entry_gate_disabled_fail trigger_exit_debounce_pass trigger_exit_strict_fail trigger_entry_delay_pass reset_before_exit_pass max_time_fail dynamic_region_expand_pass dynamic_region_update_pass dynamic_region_halt_fail"
fi

: > "$RESULTS_FILE"

for ONE_CASE in $CASES; do
    run_case "$ONE_CASE" || {
        echo "case=$ONE_CASE  case_result=error  expected=unknown  actual=script_error" >> "$RESULTS_FILE"
        ALL_OK="no"
        if [ "$JUST_MAKE" != "yes" ]; then
            break
        fi
    }
done

if [ "$JUST_MAKE" = "yes" ]; then
    echo "$ME: Just_make complete for cases: $CASES"
    exit 0
fi

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
