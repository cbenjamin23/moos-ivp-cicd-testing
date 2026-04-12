#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP="10"
VERBOSE=""
JUST_MAKE=""
MAX_TIME="120"
CASE=""
JOBS="1"
PORT_BASE="10280"
KEEP_WORKDIRS="no"
RESULTS_FILE="$PWD/results.txt"
HARNESS_DIR="$PWD"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/colregs_missions/colregs_unit"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
BHV_STEM="$MISSION_DIR/meta_vehicle.bhv"
BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"
RUN_ROOT=""
CASE_RESULT_DIR=""

for ARGI; do
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
        echo "                     min_util_cpa_low_pass"
        echo "                     min_util_cpa_default_pass"
        echo "                     min_util_cpa_high_pass"
        echo "                     headon_only_false_pass"
        echo "                     headon_only_true_pass"
        echo "                     giveway_bow_dist_default_pass"
        echo "                     giveway_bow_dist_high_pass"
        echo "                     max_util_cpa_default_pass"
        echo "                     max_util_cpa_high_pass"
        echo "                     velocity_filter_off_pass"
        echo "                     velocity_filter_on_pass"
        echo "                     eval_tol_default_pass"
        echo "                     eval_tol_short_pass"
        echo "                     eval_tol_long_pass"
        echo "                     pwt_outer_dist_default_pass"
        echo "                     pwt_outer_dist_high_pass"
        echo "                     pwt_inner_dist_low_pass"
        echo "                     pwt_inner_dist_default_pass"
        echo "                     pwt_inner_dist_high_pass"
        echo "                     pwt_grade_linear_pass"
        echo "                     pwt_grade_quasi_pass"
        echo "                     pwt_grade_quadratic_pass"
        echo "                     pts_port_turns_ok_true_pass"
        echo "                     pts_port_turns_ok_false_pass"
        echo "                     turn_radius_default_pass"
        echo "                     turn_radius_high_pass"
        echo "                     refinery_on_pass"
        echo "                     refinery_off_pass"
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
    else
        echo "$ME: Bad arg: $ARGI"
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
    local attempts="${2:-40}"
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

clear_xfiles() {
    rm -f "$SHORE_XFILE" "$BHV_XFILE"
}

cleanup() {
    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        clear_xfiles
        ./clean.sh >/dev/null 2>&1 || true
        ktm >/dev/null 2>&1 || true
    fi
    cd "$start_dir"
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
        rm -rf "$RUN_ROOT"
    fi
}

prepare_case_dir() {
    local case_dir="$1"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )

    local shore_stem="$case_dir/meta_shoreside.moos"
    local shore_xfile="$case_dir/meta_shoreside.moosx"
    local bhv_stem="$case_dir/meta_vehicle.bhv"
    local bhv_xfile="$case_dir/meta_vehicle.bhvx"
    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi
    if [ "$BHV_PATCH" != "" ]; then
        nspatch --stem="$bhv_stem" "$BHV_PATCH" --targ="$bhv_xfile"
    fi
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED="pass"
    SHORE_PATCH=""
    BHV_PATCH=""

    if [ "$CASE_NAME" = "min_util_cpa_low_pass" ]; then
        MMOD="crossing_port_standon_band350_bow_pass"
        SHORE_PATCH="$HARNESS_DIR/min-util-cpa-low-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/min-util-cpa-low-avdcol.xbhv"
    elif [ "$CASE_NAME" = "min_util_cpa_default_pass" ]; then
        MMOD="crossing_port_standon_band350_bow_pass"
        SHORE_PATCH="$HARNESS_DIR/min-util-cpa-default-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "min_util_cpa_high_pass" ]; then
        MMOD="crossing_port_standon_band350_bow_pass"
        SHORE_PATCH="$HARNESS_DIR/min-util-cpa-high-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/min-util-cpa-high-avdcol.xbhv"
    elif [ "$CASE_NAME" = "headon_only_false_pass" ]; then
        MMOD="crossing_starboard_giveway_pass"
        SHORE_PATCH="$HARNESS_DIR/headon-only-false-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "headon_only_true_pass" ]; then
        MMOD="crossing_starboard_giveway_pass"
        SHORE_PATCH="$HARNESS_DIR/headon-only-true-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/headon-only-true-avdcol.xbhv"
    elif [ "$CASE_NAME" = "giveway_bow_dist_default_pass" ]; then
        MMOD="giveway_bowdist_above_pass"
        SHORE_PATCH="$HARNESS_DIR/giveway-bow-dist-default-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_bow_dist_high_pass" ]; then
        MMOD="giveway_bowdist_above_pass"
        SHORE_PATCH="$HARNESS_DIR/giveway-bow-dist-high-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/giveway-bow-dist-high-avdcol.xbhv"
    elif [ "$CASE_NAME" = "max_util_cpa_default_pass" ]; then
        MMOD="crossing_port_standon_band315_bow_pass"
        SHORE_PATCH="$HARNESS_DIR/max-util-cpa-default-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "max_util_cpa_high_pass" ]; then
        MMOD="crossing_port_standon_band315_bow_pass"
        SHORE_PATCH="$HARNESS_DIR/max-util-cpa-high-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/max-util-cpa-high-avdcol.xbhv"
    elif [ "$CASE_NAME" = "velocity_filter_off_pass" ]; then
        MMOD="crossing_port_standon_band350_bow_pass"
        SHORE_PATCH="$HARNESS_DIR/velocity-filter-off-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "velocity_filter_on_pass" ]; then
        MMOD="crossing_port_standon_band350_bow_pass"
        SHORE_PATCH="$HARNESS_DIR/velocity-filter-on-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/velocity-filter-on-avdcol.xbhv"
    elif [ "$CASE_NAME" = "eval_tol_default_pass" ]; then
        MMOD="overtaking_starboard_range_far_pass"
        SHORE_PATCH="$HARNESS_DIR/eval-tol-default-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "eval_tol_short_pass" ]; then
        MMOD="overtaking_starboard_range_far_pass"
        SHORE_PATCH="$HARNESS_DIR/eval-tol-short-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/eval-tol-short-avdcol.xbhv"
    elif [ "$CASE_NAME" = "eval_tol_long_pass" ]; then
        MMOD="overtaking_starboard_range_far_pass"
        SHORE_PATCH="$HARNESS_DIR/eval-tol-long-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/eval-tol-long-avdcol.xbhv"
    elif [ "$CASE_NAME" = "pwt_outer_dist_default_pass" ]; then
        MMOD="crossing_port_standon_southwest_outer_above_pass"
        SHORE_PATCH="$HARNESS_DIR/pwt-outer-dist-default-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "pwt_outer_dist_high_pass" ]; then
        MMOD="crossing_port_standon_southwest_outer_above_pass"
        SHORE_PATCH="$HARNESS_DIR/pwt-outer-dist-high-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/pwt-outer-dist-high-avdcol.xbhv"
    elif [ "$CASE_NAME" = "pwt_inner_dist_low_pass" ]; then
        MMOD="crossing_port_standon_southwest_outer_above_pass"
        SHORE_PATCH="$HARNESS_DIR/pwt-inner-dist-low-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/pwt-inner-dist-low-avdcol.xbhv"
    elif [ "$CASE_NAME" = "pwt_inner_dist_default_pass" ]; then
        MMOD="crossing_port_standon_southwest_outer_above_pass"
        SHORE_PATCH="$HARNESS_DIR/pwt-inner-dist-default-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "pwt_inner_dist_high_pass" ]; then
        MMOD="crossing_port_standon_southwest_outer_above_pass"
        SHORE_PATCH="$HARNESS_DIR/pwt-inner-dist-high-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/pwt-inner-dist-high-avdcol.xbhv"
    elif [ "$CASE_NAME" = "pwt_grade_linear_pass" ]; then
        MMOD="crossing_port_standon_southwest_outer_above_pass"
        SHORE_PATCH="$HARNESS_DIR/pwt-grade-linear-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "pwt_grade_quasi_pass" ]; then
        MMOD="crossing_port_standon_southwest_outer_above_pass"
        SHORE_PATCH="$HARNESS_DIR/pwt-grade-quasi-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/pwt-grade-quasi-avdcol.xbhv"
    elif [ "$CASE_NAME" = "pwt_grade_quadratic_pass" ]; then
        MMOD="crossing_port_standon_southwest_outer_above_pass"
        SHORE_PATCH="$HARNESS_DIR/pwt-grade-quadratic-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/pwt-grade-quadratic-avdcol.xbhv"
    elif [ "$CASE_NAME" = "pts_port_turns_ok_true_pass" ]; then
        MMOD="giveway_turngap_edge_pass"
        SHORE_PATCH="$HARNESS_DIR/pts-port-turns-ok-true-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "pts_port_turns_ok_false_pass" ]; then
        MMOD="giveway_turngap_edge_pass"
        SHORE_PATCH="$HARNESS_DIR/pts-port-turns-ok-false-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/pts-port-turns-ok-false-avdcol.xbhv"
    elif [ "$CASE_NAME" = "turn_radius_default_pass" ]; then
        MMOD="giveway_turngap_edge_pass"
        SHORE_PATCH="$HARNESS_DIR/turn-radius-default-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "turn_radius_high_pass" ]; then
        MMOD="giveway_turngap_edge_pass"
        SHORE_PATCH="$HARNESS_DIR/turn-radius-high-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/turn-radius-high-avdcol.xbhv"
    elif [ "$CASE_NAME" = "refinery_on_pass" ]; then
        MMOD="head_on_cpa_fallback_pass"
        SHORE_PATCH="$HARNESS_DIR/refinery-on-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "refinery_off_pass" ]; then
        MMOD="head_on_cpa_fallback_pass"
        SHORE_PATCH="$HARNESS_DIR/refinery-off-pass-shoreside.xmoos"
        BHV_PATCH="$HARNESS_DIR/refinery-off-avdcol.xbhv"
    else
        echo "$ME: Unknown case [$CASE_NAME]"
        exit 2
    fi
}

run_case() {
    local case_name="$1"
    local line actual status

    get_case_config "$case_name"

    cd "$MISSION_DIR"
    ktm >/dev/null 2>&1 || true
    ./clean.sh >/dev/null 2>&1 || true
    clear_xfiles
    [ "$SHORE_PATCH" != "" ] && nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    [ "$BHV_PATCH" != "" ] && nspatch --stem="$BHV_STEM" "$BHV_PATCH" --targ="$BHV_XFILE"
    : > results.txt
    xlaunch.sh --max_time=$MAX_TIME --mmod=$MMOD --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP

    if [ "$JUST_MAKE" = "yes" ]; then
        clear_xfiles
        cd "$HARNESS_DIR"
        return 0
    fi

    line=$(wait_for_result_line results.txt 40)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    [ "$actual" = "" ] && actual="missing"
    status="ok"
    if [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  $line" >> "$RESULTS_FILE"
    ktm >/dev/null 2>&1 || true
    clear_xfiles
    cd "$HARNESS_DIR"
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
    local launch_rc

    get_case_config "$case_name"
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
        xlaunch.sh --max_time=$MAX_TIME --mmod=$MMOD --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP
        launch_rc=$?
        echo "$launch_rc" > launch_rc.txt
    )
    launch_rc=$(cat "$case_dir/launch_rc.txt" 2>/dev/null || echo "1")

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = "0" ]; then
            echo "case=$case_name  expected=just_make  actual=just_make  status=ok" > "$case_result_file"
            return 0
        fi
        echo "case=$case_name  expected=just_make  actual=script_error  status=error" > "$case_result_file"
        return 1
    fi

    line=$(wait_for_result_line "$case_dir/results.txt" 40)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    [ "$actual" = "" ] && actual="missing"
    status="ok"
    if [ "$actual" = "missing" ]; then
        status="error"
        ALL_OK="no"
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  launch_rc=$launch_rc  $line" > "$case_result_file"
    if [ "$launch_rc" != "0" ] && [ "$actual" = "missing" ]; then
        return 1
    fi
    if [ "$actual" != "$EXPECTED" ]; then
        return 1
    fi
    return 0
}

if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES="min_util_cpa_low_pass min_util_cpa_default_pass min_util_cpa_high_pass headon_only_false_pass headon_only_true_pass giveway_bow_dist_default_pass giveway_bow_dist_high_pass max_util_cpa_default_pass max_util_cpa_high_pass velocity_filter_off_pass velocity_filter_on_pass eval_tol_default_pass eval_tol_short_pass eval_tol_long_pass pwt_outer_dist_default_pass pwt_outer_dist_high_pass pwt_inner_dist_low_pass pwt_inner_dist_default_pass pwt_inner_dist_high_pass pwt_grade_linear_pass pwt_grade_quasi_pass pwt_grade_quadratic_pass pts_port_turns_ok_true_pass pts_port_turns_ok_false_pass turn_radius_default_pass turn_radius_high_pass refinery_on_pass refinery_off_pass"
fi

: > "$RESULTS_FILE"
trap cleanup EXIT

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for ONE_CASE in $CASES; do
        run_case "$ONE_CASE"
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_colregs_parameters_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

    ktm >/dev/null 2>&1 || true
    clear_xfiles

    case_idx=0
    wave_pids=""
    wave_count=0
    for ONE_CASE in $CASES; do
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
            ktm >/dev/null 2>&1 || true
        fi
    done

    if [ "$wave_pids" != "" ]; then
        for pid in $wave_pids; do
            wait "$pid" || ALL_OK="no"
        done
        ktm >/dev/null 2>&1 || true
    fi

    for ONE_CASE in $CASES; do
        case_file=$(find "$CASE_RESULT_DIR" -maxdepth 1 -type f -name "*_${ONE_CASE}.txt" | sort | head -n 1)
        if [ "$case_file" = "" ]; then
            echo "case=$ONE_CASE  expected=unknown  actual=missing  status=error" >> "$RESULTS_FILE"
            ALL_OK="no"
        else
            cat "$case_file" >> "$RESULTS_FILE"
            grep -q " status=ok " "$case_file" || ALL_OK="no"
        fi
    done
fi

[ "$ALL_OK" = "yes" ]
