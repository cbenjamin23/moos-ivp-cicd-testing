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
PORT_BASE="10080"
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
        echo "                     head_on_execution_pass"
        echo "                     crossing_starboard_giveway_execution_pass"
        echo "                     crossing_port_standon_execution_pass"
        echo "                     overtaking_starboard_execution_pass"
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

extract_field() {
    local line="$1"
    local key="$2"
    echo "$line" | sed -n "s/.*${key}=\\([^ ]*\\).*/\\1/p"
}

float_lt() {
    awk -v a="$1" -v b="$2" 'BEGIN { exit !(a < b) }'
}

float_gt() {
    awk -v a="$1" -v b="$2" 'BEGIN { exit !(a > b) }'
}

validate_execution_line() {
    local line="$1"
    local closest
    local cn_port
    local wall_time
    local notes=""

    closest=$(extract_field "$line" "closest_range_ever")
    cn_port=$(extract_field "$line" "cn_port")
    wall_time=$(extract_field "$line" "wall_time")

    if [ "$closest" = "" ] || [ "$cn_port" = "" ] || [ "$wall_time" = "" ]; then
        echo "error|missing_metrics"
        return 0
    fi

    if float_lt "$closest" "$MIN_CPA"; then
        notes="${notes}cpa_low,"
    fi
    if float_gt "$closest" "$MAX_CPA"; then
        notes="${notes}cpa_high,"
    fi
    if [ "$cn_port" != "$EXPECT_CN_PORT" ]; then
        notes="${notes}wrong_side,"
    fi
    if float_gt "$wall_time" "$MAX_WALL"; then
        notes="${notes}slow,"
    fi

    if [ "$notes" = "" ]; then
        echo "ok|"
    else
        echo "exec_mismatch|${notes%,}"
    fi
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
    MIN_CPA=""
    MAX_CPA=""
    EXPECT_CN_PORT=""
    MAX_WALL=""

    if [ "$CASE_NAME" = "head_on_execution_pass" ]; then
        MMOD="head_on_colregs_pass"
        SHORE_PATCH="$HARNESS_DIR/head-on-execution-pass-shoreside.xmoos"
        MIN_CPA="16"
        MAX_CPA="20"
        EXPECT_CN_PORT="1"
        MAX_WALL="12"
    elif [ "$CASE_NAME" = "crossing_starboard_giveway_execution_pass" ]; then
        MMOD="crossing_starboard_giveway_pass"
        SHORE_PATCH="$HARNESS_DIR/crossing-starboard-giveway-execution-pass-shoreside.xmoos"
        MIN_CPA="18"
        MAX_CPA="24"
        EXPECT_CN_PORT="1"
        MAX_WALL="12"
    elif [ "$CASE_NAME" = "crossing_port_standon_execution_pass" ]; then
        MMOD="crossing_port_standon_pass"
        SHORE_PATCH="$HARNESS_DIR/crossing-port-standon-execution-pass-shoreside.xmoos"
        MIN_CPA="14"
        MAX_CPA="20"
        EXPECT_CN_PORT="0"
        MAX_WALL="12"
    elif [ "$CASE_NAME" = "overtaking_starboard_execution_pass" ]; then
        MMOD="overtaking_starboard_pass"
        SHORE_PATCH="$HARNESS_DIR/overtaking-starboard-execution-pass-shoreside.xmoos"
        MIN_CPA="17"
        MAX_CPA="21"
        EXPECT_CN_PORT="0"
        MAX_WALL="12"
    else
        echo "$ME: Unknown case [$CASE_NAME]"
        exit 2
    fi
}

run_case() {
    local case_name="$1"
    local line actual status exec_check exec_status exec_notes

    get_case_config "$case_name"

    cd "$MISSION_DIR"
    ktm >/dev/null 2>&1 || true
    ./clean.sh >/dev/null 2>&1 || true
    clear_xfiles
    nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    : > results.txt
    xlaunch.sh --max_time=$MAX_TIME --mmod=$MMOD --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP

    if [ "$JUST_MAKE" = "yes" ]; then
        clear_xfiles
        cd "$HARNESS_DIR"
        return 0
    fi

    line=$(wait_for_result_line results.txt 40)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    status="ok"
    exec_notes=""
    if [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    else
        exec_check=$(validate_execution_line "$line")
        exec_status="${exec_check%%|*}"
        exec_notes="${exec_check#*|}"
        if [ "$exec_status" != "ok" ]; then
            status="$exec_status"
            ALL_OK="no"
        fi
    fi

    echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  exec_notes=$exec_notes  $line" >> "$RESULTS_FILE"
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
    local exec_check
    local exec_status
    local exec_notes

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
            echo "case=$case_name  expected=just_make  actual=just_make  status=ok  exec_notes=" > "$case_result_file"
            return 0
        fi
        echo "case=$case_name  expected=just_make  actual=script_error  status=error  exec_notes=" > "$case_result_file"
        return 1
    fi

    line=$(wait_for_result_line "$case_dir/results.txt" 40)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    status="ok"
    exec_notes=""
    if [ "$actual" = "missing" ]; then
        status="error"
        ALL_OK="no"
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    else
        exec_check=$(validate_execution_line "$line")
        exec_status="${exec_check%%|*}"
        exec_notes="${exec_check#*|}"
        if [ "$exec_status" != "ok" ]; then
            status="$exec_status"
            ALL_OK="no"
        fi
    fi

    echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  exec_notes=$exec_notes  launch_rc=$launch_rc  $line" > "$case_result_file"
    if [ "$launch_rc" != "0" ] && [ "$actual" = "missing" ]; then
        return 1
    fi
    if [ "$actual" != "$EXPECTED" ] || [ "$status" != "ok" ]; then
        return 1
    fi
    return 0
}

if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES="head_on_execution_pass crossing_starboard_giveway_execution_pass crossing_port_standon_execution_pass overtaking_starboard_execution_pass"
fi

: > "$RESULTS_FILE"
trap cleanup EXIT

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for ONE_CASE in $CASES; do
        run_case "$ONE_CASE"
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_colregs_execution_XXXXXX")
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
