#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: Mar 2026
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
MAX_TIME="70"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="9300"
PORT_BASE_SET="no"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/collision_behavior_missions/collision_behavior_motion"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
RESULT_ROW_DIR=""
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
        echo "  --jobs=<n>         Run up to n cases per wave"
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=default_resolve_pass"
        echo "  ./zlaunch.sh --case=no_alert_request_fail"
        echo "  ./zlaunch.sh --jobs=4 --port_base=17000 10"
        echo "  ./zlaunch.sh --jobs=2"
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
        PORT_BASE_SET="yes"
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

wait_for_result_line() {
    local results_path="$1"
    local attempts="${2:-60}"
    local line=""
    local attempt

    for attempt in $(seq 1 "$attempts"); do
        line=$(tail -n 1 "$results_path" 2>/dev/null)
        if echo "$line" | grep -q 'grade='; then
            echo "$line"
            return 0
        fi
        sleep 0.25
    done

    echo "$line"
    return 1
}

grade_from_line() {
    echo "$1" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'
}

format_result_row() {
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

result_row_passed() {
    local line="$1"
    local grade
    grade=$(grade_from_line "$line")
    [ "$grade" = "pass" ]
}

#------------------------------------------------------------
#  Part 4: Define cleanup and patch application helpers.
#------------------------------------------------------------
remove_tree() {
    local targ="$1"
    if [ "$targ" != "" ] && [ -d "$targ" ]; then
        rm -rf "$targ"
    fi
}

clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_MOOS_XFILE" "$VEHICLE_BHV_XFILE"
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

stop_mission_apps() {
    local mission_root="${1:-$MISSION_DIR}"
    harness_teardown_stop_root "$mission_root"
}

get_case_config() {
    CASE_NAME="$1"
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "default_resolve_pass" ]; then
        true
    elif [ "$CASE_NAME" = "no_alert_request_absent_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "post_per_contact_info_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/post-per-contact-info-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/post-per-contact-info-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "behavior_filter_absent_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/behavior-filter-absent-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/behavior-filter-absent-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/behavior-filter-absent-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "head_on_resolve_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-resolve-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/head-on-resolve-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "pwt_outer_too_small_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/pwt-outer-too-small-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/pwt-outer-inactive-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "pwt_grade_quadratic_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/pwt-grade-quadratic-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "pwt_grade_quasi_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/pwt-grade-quasi-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "use_refinery_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/use-refinery-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "contact_type_required_absent_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/contact-type-required-absent-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/contact-type-required-absent-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/contact-type-required-absent-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "no_extrapolate_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/no-extrapolate-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "no_alert_request_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/no-alert-request-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/no-alert-request-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_pwt_inner_dist_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-pwt-inner-dist-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_pwt_outer_dist_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-pwt-outer-dist-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_min_util_cpa_dist_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-min-util-cpa-dist-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_max_util_cpa_dist_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-max-util-cpa-dist-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_pwt_grade_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-pwt-grade-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_completed_dist_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-completed-dist-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_time_on_leg_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-time-on-leg-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_decay_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-decay-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_collision_depth_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-collision-depth-fail-vehicle.xbhv"
    else
        echo "$ME: Unknown case: [$CASE_NAME]"
        return 1
    fi
    return 0
}

apply_case_patches() {
    clear_xfiles

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE" || return 1
    fi

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_MOOS_STEM" "$VEH_MOOS_PATCH" --targ="$VEHICLE_MOOS_XFILE" || return 1
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_BHV_STEM" "$VEH_BHV_PATCH" --targ="$VEHICLE_BHV_XFILE" || return 1
    fi
}

#------------------------------------------------------------
#  Part 5: Run one case in the shared stem mission directory.
#------------------------------------------------------------
run_case() {
    local case_name="$1"
    local case_idx="${RUN_CASE_IDX:-0}"
    RUN_CASE_IDX=$((case_idx + 1))
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local case_base
    local line
    local result_line
    local launch_rc
    get_case_config "$case_name" || {
        echo "case=$case_name  grade=fail  reason=case_config_error" >> "$RESULTS_FILE"
        ALL_OK="no"
        return 1
    }

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR" || return 1
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || {
        echo "case=$case_name  grade=fail  reason=prepare_error" >> "$RESULTS_FILE"
        ALL_OK="no"
        cd "$HARNESS_DIR" || return 1
        return 1
    }
    : > results.txt

    XARGS=("--max_time=$MAX_TIME" "--mmod=$case_name" "$TIME_WARP")
    if [ "$PORT_BASE_SET" = "yes" ]; then
        case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
        shore_mport=$((case_base + 0))
        veh_mport=$((case_base + 1))
        shore_pshare=$((case_base + 10))
        veh_pshare=$((case_base + 11))
        XARGS+=("--shore_mport=$shore_mport" "--veh_mport=$veh_mport" "--shore_pshare=$shore_pshare" "--veh_pshare=$veh_pshare")
    fi
    if [ "$NOGUI" != "" ]; then
        XARGS+=("$NOGUI")
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
        XARGS+=("--just_make")
    fi

    vecho "Running case [$case_name] with xlaunch args: ${XARGS[*]}"
    xlaunch.sh "${XARGS[@]}"
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR" || return 1
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  grade=pass  reason=just_make" >> "$RESULTS_FILE"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" >> "$RESULTS_FILE"
        ALL_OK="no"
        return 1
    fi

    if [ "$launch_rc" = 0 ]; then
        line=$(wait_for_result_line results.txt 60)
    else
        line=""
    fi
    result_line=$(format_result_row "$case_name" "$line" "$launch_rc")

    echo "$result_line" >> "$RESULTS_FILE"
    if ! result_row_passed "$result_line"; then
        ALL_OK="no"
    fi
    cd "$HARNESS_DIR" || return 1
}

#------------------------------------------------------------
#  Part 6: Prepare and run one isolated case copy.
#------------------------------------------------------------
prepare_case_dir() {
    local case_dir="$1"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir" || exit 1
        ./clean.sh >/dev/null 2>&1 || true
    )

    local shore_stem="$case_dir/meta_shoreside.moos"
    local veh_moos_stem="$case_dir/meta_vehicle.moos"
    local veh_bhv_stem="$case_dir/meta_vehicle.bhv"
    local shore_xfile="$case_dir/meta_shoreside.moosx"
    local veh_moos_xfile="$case_dir/meta_vehicle.moosx"
    local veh_bhv_xfile="$case_dir/meta_vehicle.bhvx"

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile" || return 1
    fi

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$veh_moos_stem" "$VEH_MOOS_PATCH" --targ="$veh_moos_xfile" || return 1
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$veh_bhv_stem" "$VEH_BHV_PATCH" --targ="$veh_bhv_xfile" || return 1
    fi
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local result_row_file
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local case_base
    local line
    local result_line
    local xargs
    local launch_rc

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    result_row_file="$RESULT_ROW_DIR/${case_tag}.txt"

    get_case_config "$case_name" || {
        echo "case=$case_name  grade=fail  reason=case_config_error" > "$result_row_file"
        return 1
    }

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=prepare_error" > "$result_row_file"
        return 1
    }

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    veh_mport=$((case_base + 1))
    shore_pshare=$((case_base + 10))
    veh_pshare=$((case_base + 11))

    (
        cd "$case_dir" || exit 1
        : > results.txt
        xargs=("--max_time=$MAX_TIME" "--mmod=$case_name" "--shore_mport=$shore_mport" "--veh_mport=$veh_mport" "--shore_pshare=$shore_pshare" "--veh_pshare=$veh_pshare" "$TIME_WARP")
        if [ "$NOGUI" != "" ]; then
            xargs+=("$NOGUI")
        fi
        if [ "$JUST_MAKE" = "yes" ]; then
            xargs+=("--just_make")
        fi
        xlaunch.sh "${xargs[@]}"
    )
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  grade=pass  reason=just_make" > "$result_row_file"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" > "$result_row_file"
        return 1
    fi

    if [ "$launch_rc" = 0 ]; then
        line=$(wait_for_result_line "$case_dir/results.txt" 60)
    else
        line=""
    fi
    result_line=$(format_result_row "$case_name" "$line" "$launch_rc")

    echo "$result_line" > "$result_row_file"

    if result_row_passed "$result_line"; then
        return 0
    fi
    return 1
}

#------------------------------------------------------------
#  Part 7: Select the case set, run, and report.
#------------------------------------------------------------
trap cleanup EXIT

ALL_CASES=(
    default_resolve_pass
    no_alert_request_absent_pass
    post_per_contact_info_pass
    behavior_filter_absent_pass
    head_on_resolve_pass
    pwt_outer_too_small_fail
    pwt_grade_quadratic_pass
    pwt_grade_quasi_pass
    use_refinery_pass
    contact_type_required_absent_pass
    no_extrapolate_pass
    no_alert_request_fail
    bad_pwt_inner_dist_fail
    bad_pwt_outer_dist_fail
    bad_min_util_cpa_dist_fail
    bad_max_util_cpa_dist_fail
    bad_pwt_grade_fail
    bad_completed_dist_fail
    bad_time_on_leg_fail
    bad_decay_fail
    bad_collision_depth_fail
)

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for ONE_CASE in "${RUN_CASES[@]}"; do
        run_case "$ONE_CASE" || {
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_collision_behavior_XXXXXX")
    RESULT_ROW_DIR="$RUN_ROOT/result_rows"
    mkdir -p "$RESULT_ROW_DIR"

    stop_mission_apps "$RUN_ROOT"

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

    for result_file in $(find "$RESULT_ROW_DIR" -type f | sort); do
        cat "$result_file" >> "$RESULTS_FILE"
        echo >> "$RESULTS_FILE"
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
