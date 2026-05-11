#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

#------------------------------------------------------------
#  Part 1: Set global variable default values.
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="35"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="15000"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"
MAIN_BASHPID="$BASHPID"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/timer_behavior_missions/timer_behavior_motion"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_RESULT_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_BHV_STEM="$MISSION_DIR/meta_vehicle.bhv"
VEHICLE_MOOS_STEM="$MISSION_DIR/meta_vehicle.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"
VEHICLE_MOOS_XFILE="$MISSION_DIR/meta_vehicle.moosx"

if [ -f "$TEARDOWN_HELPER" ]; then
    . "$TEARDOWN_HELPER"
else
    echo "$ME: Missing teardown helper: $TEARDOWN_HELPER"
    exit 1
fi

#------------------------------------------------------------
#  Part 2: Check for and handle command-line arguments.
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
        echo "  --jobs=<n>         Run up to n cases per wave"
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=custom_status_vars_pass"
        echo "  ./zlaunch.sh --jobs=4 --port_base=15000"
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
    elif [ "${ARGI:0:7}" = "--jobs=" ]; then
        JOBS="${ARGI#--jobs=*}"
    elif [ "${ARGI:0:12}" = "--port_base=" ]; then
        PORT_BASE="${ARGI#--port_base=*}"
    elif [ "${ARGI}" = "--keep_workdirs" ]; then
        KEEP_WORKDIRS="yes"
    elif [ "${ARGI}" = "--gui" ]; then
        NOGUI=""
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

if ! echo "$JOBS" | grep -Eq '^[0-9]+$' || [ "$JOBS" -lt 1 ]; then
    echo "$ME: Bad value for --jobs: [$JOBS]"
    exit 1
fi

if ! echo "$PORT_BASE" | grep -Eq '^[0-9]+$'; then
    echo "$ME: Bad value for --port_base: [$PORT_BASE]"
    exit 1
fi

#------------------------------------------------------------
#  Part 3: Utility functions.
#------------------------------------------------------------
clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_BHV_XFILE" "$VEHICLE_MOOS_XFILE"
}

remove_tree() {
    local targ="$1"
    if [ "$targ" != "" ] && [ -d "$targ" ]; then
        rm -rf "$targ"
    fi
}

stop_mission_apps() {
    local mission_root="${1:-$MISSION_DIR}"
    harness_teardown_stop_root "$mission_root"
}

cleanup() {
    if [ "$BASHPID" != "$MAIN_BASHPID" ]; then
        return
    fi

    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        ./clean.sh >/dev/null 2>&1 || true
        stop_mission_apps "$MISSION_DIR"
    fi
    cd "$start_dir"
    if [ "$RUN_ROOT" != "" ]; then
        stop_mission_apps "$RUN_ROOT"
    fi
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
        remove_tree "$RUN_ROOT"
    fi
}

wait_for_result_line() {
    local file="$1"
    local tries="$2"
    local line
    local i=0
    while [ "$i" -lt "$tries" ]; do
        if [ -f "$file" ]; then
            line=`tail -n 1 "$file" 2>/dev/null`
            if echo "$line" | grep -q "grade="; then
                echo "$line"
                return 0
            fi
        fi
        sleep 0.5
        i=$((i + 1))
    done
    echo ""
    return 1
}

#------------------------------------------------------------
#  Part 4: Case mapping.
#------------------------------------------------------------
get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "baseline_idle_running_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "custom_status_vars_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/custom-status-vars-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/custom-status-vars-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "default_suffix_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/default-suffix-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/default-suffix-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "custom_no_suffix_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/custom-no-suffix-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/custom-no-suffix-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "default_no_suffix_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/default-no-suffix-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/default-no-suffix-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "active_at_start_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/active-at-start-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/active-at-start-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "delayed_activation_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/delayed-activation-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/delayed-activation-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "pause_resume_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/pause-resume-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/pause-resume-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "repeated_status_param_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/repeated-status-param-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "post_mapping_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/post-mapping-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/post-mapping-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "duration_complete_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/duration-complete-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/duration-complete-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "runtime_update_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-update-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/runtime-update-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "never_active_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/never-active-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/never-active-fail-vehicle.xmoos"
    elif [ "$CASE_NAME" = "bad_idle_var_fail" ]; then
        EXPECTED="fail"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-idle-var-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_running_var_fail" ]; then
        EXPECTED="fail"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-running-var-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_suffix_fail" ]; then
        EXPECTED="fail"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-suffix-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "post_mapping_silent_fail" ]; then
        EXPECTED="fail"
        VEH_BHV_PATCH="$HARNESS_DIR/post-mapping-silent-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "unknown_param_fail" ]; then
        EXPECTED="fail"
        VEH_BHV_PATCH="$HARNESS_DIR/unknown-param-fail-vehicle.xbhv"
    else
        echo "$ME: Unknown case: [$CASE_NAME]"
        return 1
    fi
    return 0
}

apply_case_patches() {
    clear_xfiles

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_BHV_STEM" "$VEH_BHV_PATCH" --targ="$VEHICLE_BHV_XFILE"
    fi

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_MOOS_STEM" "$VEH_MOOS_PATCH" --targ="$VEHICLE_MOOS_XFILE"
    fi
}

#------------------------------------------------------------
#  Part 5: Case runners.
#------------------------------------------------------------
run_case() {
    local case_idx="$1"
    local case_name="$2"
    local line
    local actual
    local status
    local launch_rc
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local case_base

    get_case_config "$case_name" || return 1

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    veh_mport=$((case_base + 1))
    shore_pshare=$((case_base + 10))
    veh_pshare=$((case_base + 11))

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME --mmod=$case_name --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare $TIME_WARP"
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
    line=$(wait_for_result_line results.txt 60)
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

prepare_case_dir() {
    local case_dir="$1"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )

    local shore_stem="$case_dir/meta_shoreside.moos"
    local veh_bhv_stem="$case_dir/meta_vehicle.bhv"
    local veh_moos_stem="$case_dir/meta_vehicle.moos"
    local shore_xfile="$case_dir/meta_shoreside.moosx"
    local veh_bhv_xfile="$case_dir/meta_vehicle.bhvx"
    local veh_moos_xfile="$case_dir/meta_vehicle.moosx"

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$veh_bhv_stem" "$VEH_BHV_PATCH" --targ="$veh_bhv_xfile"
    fi

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$veh_moos_stem" "$VEH_MOOS_PATCH" --targ="$veh_moos_xfile"
    fi
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_result_file
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local case_base
    local line
    local actual
    local status
    local xargs
    local launch_rc

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_result_file="$CASE_RESULT_DIR/${case_tag}.txt"

    get_case_config "$case_name" || {
        echo "case=$case_name  case_result=error  expected=unknown  actual=script_error" > "$case_result_file"
        return 1
    }

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  case_result=error  expected=$EXPECTED  actual=script_error" > "$case_result_file"
        return 1
    }

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    veh_mport=$((case_base + 1))
    shore_pshare=$((case_base + 10))
    veh_pshare=$((case_base + 11))

    (
        cd "$case_dir"
        : > results.txt
        xargs="--max_time=$MAX_TIME --mmod=$case_name --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare $TIME_WARP"
        if [ "$NOGUI" != "" ]; then
            xargs="$xargs $NOGUI"
        fi
        if [ "$JUST_MAKE" = "yes" ]; then
            xargs="$xargs --just_make"
        fi
        xlaunch.sh $xargs
    )
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  case_result=success  expected=just_make  actual=just_make" > "$case_result_file"
            return 0
        fi
        echo "case=$case_name  case_result=error  expected=just_make  actual=script_error" > "$case_result_file"
        return 1
    fi

    line=$(wait_for_result_line "$case_dir/results.txt" 60)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$launch_rc" != 0 ]; then
        status="error"
        actual="script_error"
    elif [ "$actual" = "missing" ]; then
        status="error"
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  launch_rc=$launch_rc  $line" > "$case_result_file"
    else
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" > "$case_result_file"
    fi

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

trap cleanup EXIT

#------------------------------------------------------------
#  Part 6: Select the cases, run, and report.
#------------------------------------------------------------
ALL_CASES=(
    baseline_idle_running_pass
    custom_status_vars_pass
    default_suffix_pass
    custom_no_suffix_pass
    default_no_suffix_pass
    active_at_start_pass
    delayed_activation_pass
    pause_resume_pass
    repeated_status_param_pass
    post_mapping_pass
    duration_complete_pass
    runtime_update_pass
    never_active_fail
    bad_idle_var_fail
    bad_running_var_fail
    bad_suffix_fail
    post_mapping_silent_fail
    unknown_param_fail
)

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

cd "$HARNESS_DIR"
: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ]; then
    idx=0
    for case_name in "${RUN_CASES[@]}"; do
        run_case "$idx" "$case_name" || ALL_OK="no"
        idx=$((idx + 1))
    done
else
    RUN_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/timer_harness.XXXXXX")"
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"
    idx=0
    active_pids=""

    for case_name in "${RUN_CASES[@]}"; do
        run_case_isolated "$idx" "$case_name" &
        active_pids="$active_pids $!"
        idx=$((idx + 1))

        if [ $((idx % JOBS)) -eq 0 ]; then
            for pid in $active_pids; do
                wait "$pid" || ALL_OK="no"
            done
            active_pids=""
            stop_mission_apps "$RUN_ROOT"
        fi
    done

    for pid in $active_pids; do
        wait "$pid" || ALL_OK="no"
    done

    for result in "$CASE_RESULT_DIR"/*.txt; do
        [ -f "$result" ] && cat "$result" >> "$RESULTS_FILE"
    done
fi

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
