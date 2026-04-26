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
MAX_TIME="80"
CASE=""
JOBS="1"
PORT_BASE="9800"
PORT_BASE_SET="no"
KEEP_WORKDIRS="no"
RESULTS_FILE="$PWD/results.txt"
HARNESS_DIR="$PWD"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/colregs_missions/colregs_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
RUN_ROOT=""
CASE_RESULT_DIR=""

if [ -f "$TEARDOWN_HELPER" ]; then
    . "$TEARDOWN_HELPER"
else
    echo "$ME: Missing teardown helper: $TEARDOWN_HELPER"
    exit 1
fi

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
        echo "                     head_on_colregs_pass"
        echo "                     head_on_cpa_fallback_pass"
        echo "                     head_on_port_offset_pass"
        echo "                     head_on_starboard_offset_pass"
        echo "                     crossing_starboard_giveway_pass"
        echo "                     crossing_starboard_giveway_far_pass"
        echo "                     crossing_starboard_giveway_close_pass"
        echo "                     crossing_starboard_giveway_bow_pass"
        echo "                     crossing_port_standon_pass"
        echo "                     crossing_port_standon_unsure_bow_pass"
        echo "                     crossing_port_standon_stern_pass"
        echo "                     crossing_port_standon_far_pass"
        echo "                     crossing_port_standon_far_unsure_bow_pass"
        echo "                     crossing_port_standon_close_pass"
        echo "                     crossing_port_standon_close_unsure_bow_pass"
        echo "                     crossing_port_standon_unsure_pass"
        echo "                     overtaking_starboard_pass"
        echo "                     overtaking_starboard_mirror_pass"
        echo "                     overtaking_starboard_small_gap_pass"
        echo "                     overtaking_starboard_large_gap_pass"
        echo "                     overtaking_starboard_mirror_small_gap_pass"
        echo "                     overtaking_starboard_mirror_large_gap_pass"
        echo "                     overtaken_port_standon_pass"
        echo "                     overtaken_starboard_standon_pass"
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

clear_xfiles() {
    rm -f "$SHORE_XFILE"
}

cleanup() {
    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        clear_xfiles
        ./clean.sh >/dev/null 2>&1 || true
        stop_mission_apps "$MISSION_DIR"
    fi
    cd "$start_dir"
    if [ "$RUN_ROOT" != "" ]; then
        stop_mission_apps "$RUN_ROOT"
    fi
    if [ "$JUST_MAKE" = "yes" ] && [ -f "$RESULTS_FILE" ]; then
        rm -f "$RESULTS_FILE"
    fi
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
        rm -rf "$RUN_ROOT"
    fi
}

stop_mission_apps() {
    local mission_root="${1:-$MISSION_DIR}"
    harness_teardown_stop_root "$mission_root"
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
    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED="pass"
    SHORE_PATCH=""
    MMOD="$CASE_NAME"

    if [ "$CASE_NAME" = "head_on_colregs_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-colregs-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "head_on_cpa_fallback_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-cpa-fallback-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "head_on_port_offset_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-port-offset-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "head_on_starboard_offset_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-starboard-offset-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "crossing_starboard_giveway_pass" ] || \
         [ "$CASE_NAME" = "crossing_starboard_giveway_far_pass" ] || \
         [ "$CASE_NAME" = "crossing_starboard_giveway_close_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/crossing-starboard-giveway-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "crossing_starboard_giveway_bow_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/crossing-starboard-giveway-bow-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "crossing_port_standon_pass" ] || \
         [ "$CASE_NAME" = "crossing_port_standon_far_pass" ] || \
         [ "$CASE_NAME" = "crossing_port_standon_close_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/crossing-port-standon-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "crossing_port_standon_unsure_bow_pass" ] || \
         [ "$CASE_NAME" = "crossing_port_standon_far_unsure_bow_pass" ] || \
         [ "$CASE_NAME" = "crossing_port_standon_close_unsure_bow_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/crossing-port-standon-unsure-bow-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "crossing_port_standon_unsure_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/crossing-port-standon-unsure-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "crossing_port_standon_stern_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/crossing-port-standon-stern-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaking_starboard_pass" ] || \
         [ "$CASE_NAME" = "overtaking_starboard_small_gap_pass" ] || \
         [ "$CASE_NAME" = "overtaking_starboard_large_gap_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaking-starboard-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaking_starboard_mirror_pass" ] || \
         [ "$CASE_NAME" = "overtaking_starboard_mirror_small_gap_pass" ] || \
         [ "$CASE_NAME" = "overtaking_starboard_mirror_large_gap_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaking-starboard-mirror-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaken_port_standon_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaken-port-standon-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaken_starboard_standon_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaken-starboard-standon-pass-shoreside.xmoos"
    else
        echo "$ME: Unknown case [$CASE_NAME]"
        exit 2
    fi
}

run_case() {
    local case_name="$1"
    local expected
    local line actual status
    local shore_mport veh_mport shore_pshare veh_pshare
    local xargs

    get_case_config "$case_name"
    expected="$EXPECTED"

    cd "$MISSION_DIR"
    stop_mission_apps "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1 || true
    clear_xfiles
    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    fi
    if [ "$JUST_MAKE" != "yes" ]; then
        : > results.txt
    fi
    xargs="--max_time=$MAX_TIME --mmod=$MMOD --nogui"
    if [ "$PORT_BASE_SET" = "yes" ]; then
        shore_mport=$PORT_BASE
        veh_mport=$((shore_mport + 1))
        shore_pshare=$((PORT_BASE + 200))
        veh_pshare=$((shore_pshare + 1))
        xargs="$xargs --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare"
    fi
    xlaunch.sh $xargs ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP

    if [ "$JUST_MAKE" = "yes" ]; then
        clear_xfiles
        cd "$HARNESS_DIR"
        return 0
    fi

    line=$(wait_for_result_line results.txt 32)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    status="success"
    if [ "$actual" != "$expected" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    echo "case=$case_name  case_result=$status  expected=$expected  actual=$actual  $line" >> "$RESULTS_FILE"
    stop_mission_apps "$MISSION_DIR"
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
        xlaunch.sh --max_time=$MAX_TIME --mmod=$MMOD --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP
        launch_rc=$?
        echo "$launch_rc" > launch_rc.txt
    )
    launch_rc=$(cat "$case_dir/launch_rc.txt" 2>/dev/null || echo "1")

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = "0" ]; then
            echo "case=$case_name  case_result=success  expected=just_make  actual=just_make" > "$case_result_file"
            return 0
        fi
        echo "case=$case_name  case_result=error  expected=just_make  actual=script_error" > "$case_result_file"
        return 1
    fi

    line=$(wait_for_result_line "$case_dir/results.txt" 32)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    status="success"
    if [ "$actual" = "missing" ]; then
        status="error"
        if [ "$launch_rc" != "0" ]; then
            actual="script_error"
        fi
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  launch_rc=$launch_rc  $line" > "$case_result_file"
    else
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" > "$case_result_file"
    fi

    [ "$status" = "success" ]
}

CASES="head_on_colregs_pass head_on_cpa_fallback_pass head_on_port_offset_pass head_on_starboard_offset_pass crossing_starboard_giveway_pass crossing_starboard_giveway_far_pass crossing_starboard_giveway_close_pass crossing_port_standon_pass crossing_port_standon_unsure_bow_pass crossing_port_standon_stern_pass crossing_port_standon_far_pass crossing_port_standon_close_pass crossing_port_standon_close_unsure_bow_pass crossing_port_standon_unsure_pass overtaking_starboard_pass overtaking_starboard_small_gap_pass overtaking_starboard_large_gap_pass overtaking_starboard_mirror_pass overtaking_starboard_mirror_small_gap_pass overtaking_starboard_mirror_large_gap_pass overtaken_port_standon_pass overtaken_starboard_standon_pass"
if [ "$CASE" != "" ]; then
    CASES="$CASE"
fi

if [ "$JUST_MAKE" = "yes" ]; then
    RESULTS_FILE=$(mktemp "$HARNESS_DIR/.just_make_results_XXXXXX")
fi

: > "$RESULTS_FILE"
trap cleanup EXIT

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for case_name in $CASES; do
        run_case "$case_name"
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_colregs_unit_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

    stop_mission_apps "$MISSION_DIR"

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
            stop_mission_apps "$RUN_ROOT"
        fi
    done

    if [ "$wave_pids" != "" ]; then
        for pid in $wave_pids; do
            wait "$pid" || ALL_OK="no"
        done
        stop_mission_apps "$RUN_ROOT"
    fi

    for ONE_CASE in $CASES; do
        case_file=$(find "$CASE_RESULT_DIR" -maxdepth 1 -type f -name "*_${ONE_CASE}.txt" | sort | head -n 1)
        if [ "$case_file" = "" ]; then
            echo "case=$ONE_CASE  case_result=error  expected=unknown  actual=missing" >> "$RESULTS_FILE"
            ALL_OK="no"
        else
            cat "$case_file" >> "$RESULTS_FILE"
            grep -q " case_result=success " "$case_file" || ALL_OK="no"
        fi
    done
fi

[ "$ALL_OK" = "yes" ]
