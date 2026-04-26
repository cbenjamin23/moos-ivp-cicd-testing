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
PORT_BASE="9000"
PORT_BASE_SET="no"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/obmgr_missions/obmgr_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_RESULT_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_STEM="$MISSION_DIR/meta_vehicle.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_XFILE="$MISSION_DIR/meta_vehicle.moosx"
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
        echo "  --port_base=<n>    Base shoreside MOOSDB port for wave mode"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=given_baseline_pass"
        echo "  ./zlaunch.sh --case=points_cluster_dist_pass"
        echo "  ./zlaunch.sh --jobs=4"
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
    local attempts="${2:-24}"
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
    rm -f "$SHORE_XFILE" "$VEHICLE_XFILE" "$VEHICLE_BHV_XFILE"
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
    EXPECTED=""
    SHORE_PATCH=""
    VEH_PATCH=""
    REQUIRED_TOKEN=""

    if [ "$CASE_NAME" = "given_baseline_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-baseline-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "no_alert_request_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "given_far_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-far-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-far-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "given_general_alert_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-general-alert-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-general-alert-pass-vehicle.xmoos"
        REQUIRED_TOKEN="gen_alert=name=gen_ob_far#poly=pts={35,-62:39,-62:39,-58:35,-58},label=ob_far#id=ob_far"
    elif [ "$CASE_NAME" = "general_alert_default_name_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/general-alert-default-name-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/general-alert-default-name-pass-vehicle.xmoos"
        REQUIRED_TOKEN="gen_alert=name=gen_alert_ob_far#poly=pts={35,-62:39,-62:39,-58:35,-58},label=ob_far#id=ob_far"
    elif [ "$CASE_NAME" = "given_duration_resolve_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-duration-resolve-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-duration-resolve-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "given_max_duration_reject_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-max-duration-reject-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-max-duration-reject-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "given_max_duration_missing_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-max-duration-missing-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-max-duration-missing-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "post_dist_always_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/post-dist-always-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/post-dist-always-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "post_dist_false_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/post-dist-false-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/post-dist-false-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "points_cluster_dist_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/points-cluster-dist-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/points-cluster-dist-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "custom_point_var_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/custom-point-var-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/custom-point-var-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "max_pts_per_cluster_trim_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/max-pts-per-cluster-trim-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/max-pts-per-cluster-trim-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "points_ignore_range_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/points-ignore-range-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/points-ignore-range-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "points_age_resolve_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/points-age-resolve-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/points-age-resolve-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "lasso_cluster_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/lasso-cluster-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/lasso-cluster-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "placeholder_hull_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/placeholder-hull-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/placeholder-hull-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "disable_obstacle_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/disable-obstacle-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/disable-obstacle-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "enable_obstacle_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/enable-obstacle-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/enable-obstacle-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "expunge_obstacle_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/expunge-obstacle-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/expunge-obstacle-pass-vehicle.xmoos"
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

    if [ "$VEH_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_STEM" "$VEH_PATCH" --targ="$VEHICLE_XFILE"
    fi
}

#------------------------------------------------------------
#  Part 5: Run one case in the shared stem mission directory.
#------------------------------------------------------------
run_case() {
    local case_name="$1"
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME $TIME_WARP"
    if [ "$PORT_BASE_SET" = "yes" ]; then
        shore_mport=$PORT_BASE
        veh_mport=$((shore_mport + 1))
        shore_pshare=$((PORT_BASE + 200))
        veh_pshare=$((shore_pshare + 1))
        XARGS="$XARGS --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare"
    fi
    if [ "$NOGUI" != "" ]; then
        XARGS="$XARGS $NOGUI"
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
        XARGS="$XARGS --just_make"
    fi

    vecho "Running case [$case_name] with xlaunch args: $XARGS"
    xlaunch.sh $XARGS

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR"
        return 0
    fi

    line=$(wait_for_result_line results.txt 24)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    if [ "" = "success" -a "$REQUIRED_TOKEN" != "" ]; then
        echo "$line" | grep -F "$REQUIRED_TOKEN" >/dev/null 2>&1
        if [ $? != 0 ]; then
            status="mismatch"
            ALL_OK="no"
        fi
    fi

    echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" >> "$RESULTS_FILE"
    cd "$HARNESS_DIR"
}

#------------------------------------------------------------
#  Part 6: Prepare and run one isolated case copy.
#------------------------------------------------------------
prepare_case_dir() {
    local case_dir="$1"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )

    local shore_stem="$case_dir/meta_shoreside.moos"
    local veh_stem="$case_dir/meta_vehicle.moos"
    local shore_xfile="$case_dir/meta_shoreside.moosx"
    local veh_xfile="$case_dir/meta_vehicle.moosx"

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi

    if [ "$VEH_PATCH" != "" ]; then
        nspatch --stem="$veh_stem" "$VEH_PATCH" --targ="$veh_xfile"
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
    local line
    local actual
    local status
    local xargs
    local launch_rc

    get_case_config "$case_name" || return 1

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_result_file="$CASE_RESULT_DIR/${case_tag}.txt"

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  case_result=error  expected=$EXPECTED  actual=script_error" > "$case_result_file"
        return 1
    }

    shore_mport=$((PORT_BASE + case_idx*20))
    veh_mport=$((shore_mport + 1))
    shore_pshare=$((PORT_BASE + 200 + case_idx*20))
    veh_pshare=$((shore_pshare + 1))

    (
        cd "$case_dir"
        : > results.txt
        xargs="--max_time=$MAX_TIME --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare $TIME_WARP"
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

    line=$(wait_for_result_line "$case_dir/results.txt" 24)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$launch_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi

    if [ "" = "success" -a "$REQUIRED_TOKEN" != "" ]; then
        echo "$line" | grep -F "$REQUIRED_TOKEN" >/dev/null 2>&1
        if [ $? != 0 ]; then
            status="mismatch"
        fi
    fi

    echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" > "$case_result_file"

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

#------------------------------------------------------------
#  Part 7: Validate the mission path, select the case set,
#          run the matrix, and report.
#------------------------------------------------------------
if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: [$MISSION_DIR]"
    exit 1
fi

trap cleanup EXIT

CASES="given_baseline_pass no_alert_request_absent_pass given_far_absent_pass given_general_alert_pass general_alert_default_name_pass given_duration_resolve_pass given_max_duration_reject_absent_pass given_max_duration_missing_absent_pass post_dist_always_pass post_dist_false_absent_pass points_cluster_dist_pass custom_point_var_pass max_pts_per_cluster_trim_pass points_ignore_range_absent_pass points_age_resolve_pass lasso_cluster_pass placeholder_hull_pass disable_obstacle_pass enable_obstacle_pass expunge_obstacle_pass"
if [ "$CASE" != "" ]; then
    CASES="$CASE"
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for case_name in $CASES; do
        run_case "$case_name" || {
            echo "case=$case_name  case_result=error  expected=unknown  actual=script_error" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_obmgr_unit_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

    case_index=0
    remaining_cases="$CASES"
    while [ "$remaining_cases" != "" ]; do
        launched=0
        pids=""
        wave_cases=""
        next_remaining=""

        for one_case in $remaining_cases; do
            if [ "$launched" -lt "$JOBS" ]; then
                run_case_isolated "$case_index" "$one_case" &
                pids="$pids $!"
                wave_cases="$wave_cases $(printf "%03d_%s" "$case_index" "$one_case")"
                case_index=$((case_index + 1))
                launched=$((launched + 1))
            else
                next_remaining="$next_remaining $one_case"
            fi
        done

        for pid in $pids; do
            wait "$pid" || ALL_OK="no"
        done

        for case_tag in $wave_cases; do
            if [ -f "$CASE_RESULT_DIR/${case_tag}.txt" ]; then
                cat "$CASE_RESULT_DIR/${case_tag}.txt" >> "$RESULTS_FILE"
                echo >> "$RESULTS_FILE"
                line=`tail -n 2 "$RESULTS_FILE" | head -n 1`
                case_result=`echo "$line" | sed -n 's/.*case_result=\([^ ]*\).*/\1/p'`
                if [ "$case_result" != "success" ]; then
                    ALL_OK="no"
                fi
            else
                ALL_OK="no"
            fi
        done

        stop_mission_apps "$RUN_ROOT"
        remaining_cases="$next_remaining"
    done
fi

if [ "$ALL_OK" != "yes" ]; then
    exit 1
fi

exit 0
