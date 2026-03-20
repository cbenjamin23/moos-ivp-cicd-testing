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
MAX_TIME="65"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="9000"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/cmgr_missions/cmgr_unit"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_RESULT_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_STEM="$MISSION_DIR/meta_vehicle.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_XFILE="$MISSION_DIR/meta_vehicle.moosx"
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
        echo "  --jobs=<n>         Run up to n cases per wave"
        echo "  --port_base=<n>    Base shoreside MOOSDB port for wave mode"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=detect_baseline_pass"
        echo "  ./zlaunch.sh --case=count_two_pass"
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
        ktm >/dev/null 2>&1 || true
    fi
    cd "$start_dir"
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
        remove_tree "$RUN_ROOT"
    fi
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""
    VEH_PATCH=""

    if [ "$CASE_NAME" = "detect_baseline_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-baseline-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "detect_strict_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-strict-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-strict-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_edge_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-edge-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-edge-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_edge_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-edge-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-edge-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_delayed_spoof_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-delayed-spoof-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-delayed-spoof-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_short_duration_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-short-duration-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-short-duration-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_early_checkpoint_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-early-checkpoint-absent-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "detect_far_spoof_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-far-spoof-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-far-spoof-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_near_spoof_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-near-spoof-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-near-spoof-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_cpa_only_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-cpa-only-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-cpa-only-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "closest_contact_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/closest-contact-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "post_all_ranges_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/post-all-ranges-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/post-all-ranges-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "post_closest_relbng_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/post-closest-relbng-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/post-closest-relbng-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_alert_add_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-alert-add-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/runtime-alert-add-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_alert_disable_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-alert-disable-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/runtime-alert-disable-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "filter_match_type_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/filter-match-type-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/filter-match-type-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "disable_contact_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/disable-contact-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/disable-contact-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "enable_contact_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/enable-contact-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/enable-contact-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "early_warning_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/early-warning-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/early-warning-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "count_one_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/count-one-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "list_intruder_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/list-intruder-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "range_report_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/range-report-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "report_request_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/report-request-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/report-request-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "range_tight_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/range-tight-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "count_two_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/count-two-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/multi-baseline-vehicle.xmoos"
    elif [ "$CASE_NAME" = "list_alpha_bravo_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/list-alpha-bravo-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/multi-baseline-vehicle.xmoos"
    elif [ "$CASE_NAME" = "closest_alpha_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/closest-alpha-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/multi-baseline-vehicle.xmoos"
    elif [ "$CASE_NAME" = "closest_bravo_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/closest-bravo-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/closest-bravo-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "bravo_far_count_one_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/bravo-far-count-one-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/bravo-far-count-one-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "late_bravo_count_two_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/late-bravo-count-two-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/late-bravo-count-two-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "stale_bravo_drop_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/stale-bravo-drop-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/stale-bravo-drop-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "reject_range_retire_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/reject-range-retire-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/reject-range-retire-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "alpha_far_bravo_only_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/alpha-far-bravo-only-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/alpha-far-bravo-only-pass-vehicle.xmoos"
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
    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    ktm >/dev/null 2>&1 || true
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME $TIME_WARP"
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

    line=`tail -n 1 results.txt 2>/dev/null`
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="ok"
    if [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  $line" >> "$RESULTS_FILE"
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
        echo "case=$case_name  expected=$EXPECTED  actual=script_error  status=error" > "$case_result_file"
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
            echo "case=$case_name  expected=just_make  actual=just_make  status=ok" > "$case_result_file"
            return 0
        fi
        echo "case=$case_name  expected=just_make  actual=script_error  status=error" > "$case_result_file"
        return 1
    fi

    line=`tail -n 1 "$case_dir/results.txt" 2>/dev/null`
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="ok"
    if [ "$launch_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi

    echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  $line" > "$case_result_file"

    if [ "$status" = "ok" ]; then
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

CASES="detect_baseline_pass detect_strict_absent_pass detect_edge_pass detect_edge_absent_pass detect_delayed_spoof_pass detect_short_duration_absent_pass detect_early_checkpoint_absent_pass detect_far_spoof_absent_pass detect_near_spoof_pass detect_cpa_only_pass closest_contact_pass post_all_ranges_pass post_closest_relbng_pass runtime_alert_add_pass runtime_alert_disable_absent_pass filter_match_type_absent_pass disable_contact_pass enable_contact_pass early_warning_pass count_one_pass list_intruder_pass range_report_pass report_request_pass range_tight_pass count_two_pass list_alpha_bravo_pass closest_alpha_pass closest_bravo_pass bravo_far_count_one_pass late_bravo_count_two_pass stale_bravo_drop_pass reject_range_retire_pass alpha_far_bravo_only_pass"
if [ "$CASE" != "" ]; then
    CASES="$CASE"
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for case_name in $CASES; do
        run_case "$case_name" || {
            echo "case=$case_name  expected=unknown  actual=script_error  status=error" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_cmgr_unit_XXXXXX")
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
                status=`echo "$line" | sed -n 's/.*status=\([^ ]*\).*/\1/p'`
                if [ "$status" != "ok" ]; then
                    ALL_OK="no"
                fi
            else
                ALL_OK="no"
            fi
        done

        ktm >/dev/null 2>&1 || true
        sleep 1
        remaining_cases="$next_remaining"
    done
fi

if [ "$ALL_OK" != "yes" ]; then
    exit 1
fi

exit 0
