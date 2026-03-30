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
KEEP_WORKDIRS="no"
RESULTS_FILE="$PWD/results.txt"
HARNESS_DIR="$PWD"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/colregs_missions/colregs_unit"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
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
        echo "                     head_on_colregs_pass"
        echo "                     head_on_cpa_fallback_pass"
        echo "                     head_on_port_offset_pass"
        echo "                     head_on_starboard_offset_pass"
        echo "                     crossing_starboard_giveway_pass"
        echo "                     crossing_starboard_giveway_far_pass"
        echo "                     crossing_starboard_giveway_close_pass"
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
    else
        echo "$ME: Unknown case [$CASE_NAME]"
        exit 2
    fi
}

run_case() {
    local case_name="$1"
    local expected
    local line actual status

    get_case_config "$case_name"
    expected="$EXPECTED"

    cd "$MISSION_DIR"
    ktm >/dev/null 2>&1 || true
    ./clean.sh >/dev/null 2>&1 || true
    clear_xfiles
    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    fi
    : > results.txt
    xlaunch.sh --max_time=$MAX_TIME --mmod=$MMOD --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP

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
    status="ok"
    if [ "$actual" != "$expected" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    echo "case=$case_name  expected=$expected  actual=$actual  status=$status  $line" >> "$RESULTS_FILE"
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
        ./launch.sh --xlaunched --mmod=$MMOD --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP
        launch_rc=$?

        if [ "$JUST_MAKE" != "yes" ] && [ "$launch_rc" = 0 ]; then
            uMayFinish --alias="uMayFinish_${case_idx}" --max_time=${MAX_TIME} targ_shoreside.moos
            launch_rc=$?
            pkill -INT -P $$ >/dev/null 2>&1 || true
            sleep 2
            if [ ! -z "$(tail -c 1 <"results.txt" 2>/dev/null)" ]; then
                echo "" >> results.txt
            fi
        fi

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

    line=$(wait_for_result_line "$case_dir/results.txt" 32)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    status="ok"
    if [ "$actual" = "missing" ]; then
        status="error"
        if [ "$launch_rc" != "0" ]; then
            actual="script_error"
        fi
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  launch_rc=$launch_rc  $line" > "$case_result_file"
    else
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  $line" > "$case_result_file"
    fi

    [ "$status" = "ok" ]
}

CASES="head_on_colregs_pass head_on_cpa_fallback_pass head_on_port_offset_pass head_on_starboard_offset_pass crossing_starboard_giveway_pass crossing_starboard_giveway_far_pass crossing_starboard_giveway_close_pass crossing_port_standon_pass crossing_port_standon_unsure_bow_pass crossing_port_standon_stern_pass crossing_port_standon_far_pass crossing_port_standon_far_unsure_bow_pass crossing_port_standon_close_pass crossing_port_standon_close_unsure_bow_pass crossing_port_standon_unsure_pass overtaking_starboard_pass overtaking_starboard_small_gap_pass overtaking_starboard_large_gap_pass overtaking_starboard_mirror_pass overtaking_starboard_mirror_small_gap_pass overtaking_starboard_mirror_large_gap_pass"
if [ "$CASE" != "" ]; then
    CASES="$CASE"
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

    ktm >/dev/null 2>&1 || true

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
