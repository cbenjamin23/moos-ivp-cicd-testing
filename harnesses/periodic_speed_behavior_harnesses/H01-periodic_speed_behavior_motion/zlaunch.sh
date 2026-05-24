#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

#------------------------------------------------------------
#  Part 2: Set global variable default values.
#------------------------------------------------------------
ME=$(basename "$0")
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="55"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="15000"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/periodic_speed_behavior_missions/periodic_speed_behavior_motion"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_ROW_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_MOOS_STEM="$MISSION_DIR/meta_vehicle.moos"
VEHICLE_BHV_STEM="$MISSION_DIR/meta_vehicle.bhv"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_MOOS_XFILE="$MISSION_DIR/meta_vehicle.moosx"
VEHICLE_BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"

if [ -f "$TEARDOWN_HELPER" ]; then
    . "$TEARDOWN_HELPER"
else
    echo "$ME: Missing teardown helper: $TEARDOWN_HELPER"
    exit 1
fi

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments.
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
        echo "  --port_stride=<n>  Port block stride between cases"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=baseline_cycle_pass --port_base=15000 10"
        echo "  ./zlaunch.sh --jobs=4 --port_base=34000 --port_stride=12 10"
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
    elif [ "${ARGI:0:14}" = "--port_stride=" ]; then
        PORT_STRIDE="${ARGI#--port_stride=*}"
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

if ! echo "$PORT_STRIDE" | grep -Eq '^[0-9]+$' || [ "$PORT_STRIDE" -lt 12 ]; then
    echo "$ME: Bad value for --port_stride: [$PORT_STRIDE]"
    exit 1
fi

clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_MOOS_XFILE" "$VEHICLE_BHV_XFILE"
}

#------------------------------------------------------------
#  Part 4: Define mission patch, run, and cleanup helpers.
#------------------------------------------------------------
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

grade_from_line() {
    echo "$1" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'
}

format_case_row() {
    local case_name="$1"
    local line="$2"
    local launch_rc="${3:-0}"
    local grade

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc"
        return
    fi

    line=$(echo "$line" | sed 's/^[[:space:]]*//')
    grade=$(grade_from_line "$line")
    if [ "$grade" = "" ]; then
        echo "case=$case_name  grade=fail  reason=missing_result"
        return
    fi

    line=$(echo "$line" | sed 's/grade=[^, ]*[[:space:]]*//')


    echo "case=$case_name  grade=$grade  $line"
}

case_row_passed() {
    local line="$1"
    local grade
    grade=$(grade_from_line "$line")
    [ "$grade" = "pass" ]
}

get_case_config() {
    CASE_NAME="$1"
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "baseline_cycle_pass" ]; then
        :
    elif [ "$CASE_NAME" = "lazy_wait_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-lazy-wait-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "first_busy_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-first-busy-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "busy_to_lazy_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-busy-to-lazy-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/short-cycle-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "deprecated_zaic_aliases_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-deprecated-zaic-aliases-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/deprecated-zaic-aliases-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "short_cycle_three_count_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-short-cycle-three-count-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/short-cycle-three-count-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "explicit_initially_lazy_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-explicit-initially-lazy-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/explicit-initially-lazy-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "zaic_base_period_peak_aliases_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-zaic-base-period-peak-aliases-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/zaic-base-period-peak-aliases-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "reset_on_reactivation_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-reset-on-reactivation-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/reset-on-reactivation-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "reset_false_continuous_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-reset-false-continuous-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/reset-false-continuous-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "reset_false_visual_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-reset-false-visual-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/reset-false-visual-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "initially_busy_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-initially-busy-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/initially-busy-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "zero_speed_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-zero-speed-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/zero-speed-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_period_lazy_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-period-lazy-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_period_lazy_nonnumeric_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-period-lazy-nonnumeric-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_period_busy_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-period-busy-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_period_speed_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-period-speed-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_initially_busy_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-initially-busy-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_reset_upon_running_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-reset-upon-running-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_basewidth_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-basewidth-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_zaic_basewidth_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-zaic-basewidth-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_peakwidth_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-peakwidth-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_period_peakwidth_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-period-peakwidth-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_summit_delta_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-summit-delta-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_zaic_summit_delta_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-zaic-summit-delta-fail-vehicle.xbhv"
    else
        echo "$ME: Unknown case: [$CASE_NAME]"
        return 1
    fi
    if echo "$CASE_NAME" | grep -Eq '^bad_.*_fail$'; then
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
    fi
    return 0
}

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

run_case() {
    local case_idx="$1"
    local case_name="$2"
    local line
    local result_line
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
    apply_case_patches || {
        echo "case=$case_name  grade=fail  reason=prepare_error" >> "$RESULTS_FILE"
        cd "$HARNESS_DIR"
        return 1
    }
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
            echo "case=$case_name  grade=pass  reason=just_make" >> "$RESULTS_FILE"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" >> "$RESULTS_FILE"
        return 1
    fi

    sleep 1

    if [ "$launch_rc" = 0 ]; then
        line=$(wait_for_result_line results.txt 60)
    else
        line=""
    fi
    result_line=$(format_case_row "$case_name" "$line" "$launch_rc")

    echo "$result_line" >> "$RESULTS_FILE"
    if ! case_row_passed "$result_line"; then
        ALL_OK="no"
        cd "$HARNESS_DIR"
        return 1
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
    local veh_moos_stem="$case_dir/meta_vehicle.moos"
    local veh_bhv_stem="$case_dir/meta_vehicle.bhv"
    local shore_xfile="$case_dir/meta_shoreside.moosx"
    local veh_moos_xfile="$case_dir/meta_vehicle.moosx"
    local veh_bhv_xfile="$case_dir/meta_vehicle.bhvx"

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$veh_moos_stem" "$VEH_MOOS_PATCH" --targ="$veh_moos_xfile"
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$veh_bhv_stem" "$VEH_BHV_PATCH" --targ="$veh_bhv_xfile"
    fi
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_row_file
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local line
    local result_line
    local xargs
    local launch_rc

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_row_file="$CASE_ROW_DIR/${case_tag}.txt"

    get_case_config "$case_name" || {
        echo "case=$case_name  grade=fail  reason=case_config_error" > "$case_row_file"
        return 1
    }

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=prepare_error" > "$case_row_file"
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
        perl -e 'setpgrp(0, 0); exec @ARGV or die $!' xlaunch.sh $xargs
    )
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  grade=pass  reason=just_make" > "$case_row_file"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" > "$case_row_file"
        return 1
    fi

    if [ "$launch_rc" = 0 ]; then
        line=$(wait_for_result_line "$case_dir/results.txt" 60)
    else
        line=""
    fi
    result_line=$(format_case_row "$case_name" "$line" "$launch_rc")

    echo "$result_line" > "$case_row_file"

    if case_row_passed "$result_line"; then
        return 0
    fi
    return 1
}

#------------------------------------------------------------
#  Part 5: Run requested cases serially or in isolated waves.
#------------------------------------------------------------
trap cleanup EXIT

ALL_CASES=(
    baseline_cycle_pass
    lazy_wait_pass
    first_busy_pass
    busy_to_lazy_pass
    deprecated_zaic_aliases_pass
    short_cycle_three_count_pass
    explicit_initially_lazy_pass
    zaic_base_period_peak_aliases_pass
    reset_on_reactivation_pass
    reset_false_continuous_pass
    initially_busy_pass
    zero_speed_pass
    bad_period_lazy_fail
    bad_period_lazy_nonnumeric_fail
    bad_period_busy_fail
    bad_period_speed_fail
    bad_initially_busy_fail
    bad_reset_upon_running_fail
    bad_basewidth_fail
    bad_zaic_basewidth_fail
    bad_peakwidth_fail
    bad_period_peakwidth_fail
    bad_summit_delta_fail
    bad_zaic_summit_delta_fail
)

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    case_idx=0
    for ONE_CASE in "${RUN_CASES[@]}"; do
        run_case "$case_idx" "$ONE_CASE" || {
            if ! grep -q "case=$ONE_CASE  " "$RESULTS_FILE" 2>/dev/null; then
                echo "case=$ONE_CASE  grade=fail  reason=case_run_error" >> "$RESULTS_FILE"
            fi
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
        case_idx=$((case_idx + 1))
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_periodic_speed_XXXXXX")
    CASE_ROW_DIR="$RUN_ROOT/case_rows"
    mkdir -p "$CASE_ROW_DIR"

    case_idx=0
    wave_pids=""
    wave_count=0
    for ONE_CASE in "${RUN_CASES[@]}"; do
        run_case_isolated "$case_idx" "$ONE_CASE" &
        wave_pids="$wave_pids $!"
        wave_count=$((wave_count + 1))
        case_idx=$((case_idx + 1))

        if [ "$wave_count" -ge "$JOBS" ]; then
            for pid in $wave_pids; do
                wait "$pid" || ALL_OK="no"
            done
            wave_pids=""
            wave_count=0
            stop_mission_apps "$RUN_ROOT"
        fi
    done

    if [ "$wave_count" -gt 0 ]; then
        for pid in $wave_pids; do
            wait "$pid" || ALL_OK="no"
        done
        stop_mission_apps "$RUN_ROOT"
    fi

    for ONE_CASE in "${RUN_CASES[@]}"; do
        case_tag=""
        for result_file in "$CASE_ROW_DIR"/*.txt; do
            if grep -q "case=$ONE_CASE  " "$result_file" 2>/dev/null; then
                cat "$result_file" >> "$RESULTS_FILE"
                line=`tail -n 1 "$result_file"`
                if ! case_row_passed "$line"; then
                    ALL_OK="no"
                fi
                case_tag="found"
                break
            fi
        done
        if [ "$case_tag" = "" ]; then
            echo "case=$ONE_CASE  grade=fail  reason=missing_result_file" >> "$RESULTS_FILE"
            ALL_OK="no"
        fi
    done
fi

if [ "$JUST_MAKE" = "yes" ]; then
    echo "$ME: Just_make complete for cases: ${RUN_CASES[*]}"
    exit 0
fi

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
