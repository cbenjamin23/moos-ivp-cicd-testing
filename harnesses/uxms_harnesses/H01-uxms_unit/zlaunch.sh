#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-uxms_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; cleanup; exit 1; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP="10"
MAX_TIME="30"
JOBS="1"
PORT_BASE="15200"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""
CAPTURE_SECONDS="2.8"

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/uxms_missions/uxms_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
RUN_ROOT=""
CASE_RESULT_DIR=""
ALL_OK="yes"

if [ -f "$TEARDOWN_HELPER" ]; then
    . "$TEARDOWN_HELPER"
else
    echo "$ME: Missing teardown helper: $TEARDOWN_HELPER"
    exit 1
fi

usage() {
    echo "$ME [OPTIONS] [time_warp]"
    echo ""
    echo "Options:"
    echo "  --help, -h           Show this help message"
    echo "  --case=<name>        Run one named case"
    echo "  --jobs=<n>           Run up to n cases per wave"
    echo "  --port_base=<n>      Base MOOSDB port for per-case blocks"
    echo "  --max_time=<secs>    Max time passed to uMayFinish"
    echo "  --just_make, -j      Generate target files only"
    echo "  --keep_workdirs      Keep temp mission copies"
    echo "  --verbose, -v        Verbose output"
}

for ARGI; do
    if [ "$ARGI" = "--help" ] || [ "$ARGI" = "-h" ]; then
        usage
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" ] && [ "$TIME_WARP" = 10 ]; then
        TIME_WARP="$ARGI"
    elif [ "${ARGI:0:7}" = "--case=" ]; then
        CASE="${ARGI#--case=*}"
    elif [ "${ARGI:0:7}" = "--jobs=" ]; then
        JOBS="${ARGI#--jobs=*}"
    elif [ "${ARGI:0:12}" = "--port_base=" ]; then
        PORT_BASE="${ARGI#--port_base=*}"
    elif [ "${ARGI:0:11}" = "--max_time=" ]; then
        MAX_TIME="${ARGI#--max_time=*}"
    elif [ "$ARGI" = "--just_make" ] || [ "$ARGI" = "-j" ]; then
        JUST_MAKE="yes"
    elif [ "$ARGI" = "--keep_workdirs" ]; then
        KEEP_WORKDIRS="yes"
    elif [ "$ARGI" = "--verbose" ] || [ "$ARGI" = "-v" ]; then
        VERBOSE="yes"
    elif [ "$ARGI" = "--gui" ] || [ "$ARGI" = "--nogui" ] || [ "$ARGI" = "-ng" ]; then
        true
    else
        echo "$ME: Bad arg: $ARGI"
        exit 1
    fi
done

if ! echo "$JOBS" | grep -Eq '^[0-9]+$' || [ "$JOBS" -lt 1 ]; then
    echo "$ME: Bad --jobs value: $JOBS"
    exit 1
fi
if ! echo "$PORT_BASE" | grep -Eq '^[0-9]+$'; then
    echo "$ME: Bad --port_base value: $PORT_BASE"
    exit 1
fi

ALL_CASES=(
    scoped_var_pass
    scoped_excludes_noise_pass
    all_mode_pass
    all_shortcut_pass
    show_source_pass
    show_time_only_pass
    show_community_only_pass
    show_time_community_pass
    show_source_community_pass
    history_var_pass
    trunc_long_payload_pass
    clean_cli_scope_pass
    config_var_pass
    filter_prefix_pass
    src_timer_pass
    novirgins_pass
    novirgins_shortcut_pass
    shortcut_source_pass
    shortcut_source_time_pass
    filter_specific_pass
    filter_no_match_absent_pass
    colorany_pass
    events_mode_pass
    paused_shortcut_pass
    shortcut_trunc_pass
    alll_mode_pass
    server_underscore_args_pass
    moos_alias_args_pass
    moos_underscore_args_pass
    alias_proc_name_pass
    colormap_pair_pass
    clean_shortcut_scope_pass
)

cleanup() {
    if [ "$RUN_ROOT" != "" ]; then
        harness_teardown_stop_root "$RUN_ROOT"
    fi
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ] && [ -d "$RUN_ROOT" ]; then
        rm -rf "$RUN_ROOT"
    fi
}

case_known() {
    local wanted="$1"
    local one
    for one in "${ALL_CASES[@]}"; do
        if [ "$one" = "$wanted" ]; then
            return 0
        fi
    done
    return 1
}

get_case_config() {
    CASE_NAME="$1"
    XMS_ARGS=()
    XMS_USE_FILE="no"
    XMS_CONNECT_MODE="serverhost"
    CHECK_TOKENS=()
    ABSENT_TOKENS=()

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    case "$CASE_NAME" in
        scoped_var_pass)
            XMS_ARGS=("--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        scoped_excludes_noise_pass)
            XMS_ARGS=("--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ABSENT_TOKENS=("XMS_NOISE")
            ;;
        all_mode_pass)
            XMS_ARGS=("--all" "--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("DB_UPTIME" "XMS_ALPHA")
            ;;
        all_shortcut_pass)
            XMS_ARGS=("-a" "--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("DB_UPTIME" "XMS_ALPHA")
            ;;
        show_source_pass)
            XMS_ARGS=("--show=source" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "uTimerScript")
            ;;
        show_time_only_pass)
            XMS_ARGS=("--show=time" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "(T")
            ;;
        show_community_only_pass)
            XMS_ARGS=("--show=community" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "shoreside")
            ;;
        show_time_community_pass)
            XMS_ARGS=("--show=time,community" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "shoreside")
            ;;
        show_source_community_pass)
            XMS_ARGS=("--show=source,community" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "uTimerScript" "shoreside")
            ;;
        history_var_pass)
            XMS_ARGS=("--history=XMS_HIST" "--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("XMS_HIST" "third")
            ;;
        trunc_long_payload_pass)
            XMS_ARGS=("--trunc=25" "--mode=streaming" "--termint=0.2" "XMS_LONG")
            CHECK_TOKENS=("XMS_LONG" "abcdefghijklmnopqrstuvwxyz")
            ABSENT_TOKENS=("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
            ;;
        clean_cli_scope_pass)
            XMS_USE_FILE="yes"
            XMS_ARGS=("--clean" "--mode=streaming" "--termint=0.2" "XMS_NUM")
            CHECK_TOKENS=("XMS_NUM" "42")
            ABSENT_TOKENS=("XMS_ALPHA")
            ;;
        config_var_pass)
            XMS_USE_FILE="yes"
            XMS_ARGS=("--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("XMS_ALPHA" "XMS_SECONDARY")
            ;;
        filter_prefix_pass)
            XMS_ARGS=("--all" "--filter=XMS_" "--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("XMS_ALPHA" "XMS_NUM")
            ABSENT_TOKENS=("DB_UPTIME")
            ;;
        src_timer_pass)
            XMS_ARGS=("--src=uTimerScript" "--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("XMS_ALPHA" "XMS_NUM")
            ;;
        novirgins_pass)
            XMS_ARGS=("--novirgins" "--mode=streaming" "--termint=0.2" "XMS_ALPHA" "XMS_MISSING")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ABSENT_TOKENS=("XMS_MISSING" "n/a")
            ;;
        novirgins_shortcut_pass)
            XMS_ARGS=("-g" "--mode=streaming" "--termint=0.2" "XMS_ALPHA" "XMS_MISSING")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ABSENT_TOKENS=("XMS_MISSING" "n/a")
            ;;
        shortcut_source_pass)
            XMS_ARGS=("-s" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "uTimerScript")
            ;;
        shortcut_source_time_pass)
            XMS_ARGS=("-st" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "uTimerScript")
            ;;
        filter_specific_pass)
            XMS_ARGS=("--all" "--filter=XMS_A" "--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ABSENT_TOKENS=("XMS_NUM" "DB_UPTIME")
            ;;
        filter_no_match_absent_pass)
            XMS_ARGS=("--all" "--filter=NO_SUCH_PREFIX" "--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("SCOPING")
            ABSENT_TOKENS=("XMS_ALPHA" "DB_UPTIME")
            ;;
        colorany_pass)
            XMS_ARGS=("--colorany=XMS_ALPHA" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        events_mode_pass)
            XMS_ARGS=("--mode=events" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        paused_shortcut_pass)
            XMS_ARGS=("-p" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        shortcut_trunc_pass)
            XMS_ARGS=("-t" "--mode=streaming" "--termint=0.2" "XMS_LONG")
            CHECK_TOKENS=("XMS_LONG" "abcdefghijklmnopqrstuvwxyz")
            ABSENT_TOKENS=("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
            ;;
        alll_mode_pass)
            XMS_ARGS=("--alll" "--mode=streaming" "--termint=0.2")
            CHECK_TOKENS=("DB_UPTIME" "XMS_ALPHA")
            ;;
        server_underscore_args_pass)
            XMS_CONNECT_MODE="server_underscore"
            XMS_ARGS=("--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        moos_alias_args_pass)
            XMS_CONNECT_MODE="moos_alias"
            XMS_ARGS=("--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        moos_underscore_args_pass)
            XMS_CONNECT_MODE="moos_underscore"
            XMS_ARGS=("--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        alias_proc_name_pass)
            XMS_ARGS=("--alias=uXMS_ci" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        colormap_pair_pass)
            XMS_ARGS=("--colormap=XMS_ALPHA,red" "--mode=streaming" "--termint=0.2" "XMS_ALPHA")
            CHECK_TOKENS=("XMS_ALPHA" "alpha")
            ;;
        clean_shortcut_scope_pass)
            XMS_USE_FILE="yes"
            XMS_ARGS=("-c" "--mode=streaming" "--termint=0.2" "XMS_NUM")
            CHECK_TOKENS=("XMS_NUM" "42")
            ABSENT_TOKENS=("XMS_ALPHA")
            ;;
    esac
}

wait_for_results() {
    local file="$1"
    local attempts="${2:-120}"
    local line=""
    local i
    for ((i=0; i<attempts; i++)); do
        line=$(tail -n 1 "$file" 2>/dev/null)
        if echo "$line" | grep -q 'grade='; then
            echo "$line"
            return 0
        fi
        sleep 0.25
    done
    echo "$line"
    return 1
}

run_xms_capture() {
    local case_dir="$1"
    local port="$2"
    local output="$case_dir/uxms.out"
    local cmd=()

    if [ "$XMS_USE_FILE" = "yes" ]; then
        cmd=("uXMS" "$case_dir/targ_shoreside.moos" "${XMS_ARGS[@]}")
    elif [ "$XMS_CONNECT_MODE" = "server_underscore" ]; then
        cmd=("uXMS" "--server_host=localhost" "--server_port=$port" "${XMS_ARGS[@]}")
    elif [ "$XMS_CONNECT_MODE" = "moos_alias" ]; then
        cmd=("uXMS" "--mooshost=localhost" "--moosport=$port" "${XMS_ARGS[@]}")
    elif [ "$XMS_CONNECT_MODE" = "moos_underscore" ]; then
        cmd=("uXMS" "--moos_host=localhost" "--moos_port=$port" "${XMS_ARGS[@]}")
    else
        cmd=("uXMS" "--serverhost=localhost" "--serverport=$port" "${XMS_ARGS[@]}")
    fi

    (
        sleep "$CAPTURE_SECONDS"
        printf 'q'
    ) | "${cmd[@]}" > "$output" 2>&1
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    local shore_mport=$((case_base + 0))
    local shore_pshare=$((case_base + 10))
    local tag
    local case_dir
    local case_result_file
    local line
    local actual
    local status="success"
    local check

    get_case_config "$case_name" || return 1

    tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$tag"
    case_result_file="$CASE_RESULT_DIR/$tag.txt"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/

    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
        ./launch.sh --just_make --xlaunched --nogui --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --mmod="$case_name" "$TIME_WARP" >/dev/null
    )
    if [ "$JUST_MAKE" = "yes" ]; then
        echo "case=$case_name  case_result=success  expected=just_make  actual=just_make" > "$case_result_file"
        return 0
    fi

    (
        cd "$case_dir"
        : > results.txt
        ./launch.sh --xlaunched --nogui --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --mmod="$case_name" "$TIME_WARP" > launch.out 2>&1
    )
    sleep 1.0
    run_xms_capture "$case_dir" "$shore_mport"
    (
        cd "$case_dir"
        uMayFinish --max_time="$MAX_TIME" targ_shoreside.moos > mayfinish.out 2>&1 || true
    )
    line=$(wait_for_results "$case_dir/results.txt" 120)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    if [ "$actual" != "pass" ]; then
        status="mismatch"
    fi
    for check in "${CHECK_TOKENS[@]}"; do
        if ! grep -q "$check" "$case_dir/uxms.out"; then
            status="mismatch"
        fi
    done
    for check in "${ABSENT_TOKENS[@]}"; do
        if grep -q "$check" "$case_dir/uxms.out"; then
            status="mismatch"
        fi
    done

    echo "case=$case_name  case_result=$status  expected=pass  actual=$actual  output_checks=${#CHECK_TOKENS[@]}  absent_checks=${#ABSENT_TOKENS[@]}  $line" > "$case_result_file"
    harness_teardown_stop_root "$case_dir"

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: $MISSION_DIR"
    exit 1
fi

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    if ! case_known "$CASE"; then
        echo "$ME: Unknown case: $CASE"
        exit 1
    fi
    RUN_CASES=("$CASE")
fi

trap cleanup EXIT
: > "$RESULTS_FILE"
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_uxms_unit_XXXXXX")
CASE_RESULT_DIR="$RUN_ROOT/case_results"
mkdir -p "$CASE_RESULT_DIR"

case_index=0
remaining_cases="${RUN_CASES[*]}"
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
        if [ -f "$CASE_RESULT_DIR/$case_tag.txt" ]; then
            cat "$CASE_RESULT_DIR/$case_tag.txt" >> "$RESULTS_FILE"
        else
            echo "case=${case_tag#*_}  case_result=error  expected=unknown  actual=missing_result" >> "$RESULTS_FILE"
            ALL_OK="no"
        fi
    done

    remaining_cases="$next_remaining"
done

if grep -q 'case_result=mismatch\|case_result=error' "$RESULTS_FILE"; then
    ALL_OK="no"
fi

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
