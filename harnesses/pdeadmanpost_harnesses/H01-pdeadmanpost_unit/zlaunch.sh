#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-pdeadmanpost_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
on_exit() { echo; echo "$ME: Halting all apps"; cleanup; exit 1; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP="10"
MAX_TIME="30"
JOBS="1"
PORT_BASE="15800"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/pdeadmanpost_missions/pdeadmanpost_unit"
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
    active_start_once_posts_pass
    active_start_false_no_initial_post_pass
    heartbeat_before_dead_suppresses_pass
    stop_heartbeat_posts_pass
    active_false_after_first_heartbeat_posts_pass
    numeric_deadflag_pass
    string_deadflag_pass
    multiple_deadflags_pass
    once_policy_single_post_pass
    repeat_policy_reposts_pass
    reset_policy_reposts_after_heartbeat_pass
    repeat_policy_inactive_reposts_pass
    reset_policy_no_repost_without_heartbeat_pass
    active_start_keepalive_suppresses_pass
    custom_heartbeat_var_pass
    negative_deadflag_pass
    zero_max_noheart_posts_pass
    false_deadflag_pass
    invalid_post_policy_falls_back_once_pass
    reset_policy_multiple_reposts_pass
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
    HEARTBEAT_EVENTS=()
    DEADFLAGS=()
    PASS_CONDITIONS=()
    FAIL_CONDITIONS=()
    REPORT_COLUMNS=()
    CHECK_TOKENS=()
    MIN_COUNTS=()
    MAX_COUNTS=()
    HEART_VAR="HEARTBEAT"
    MAX_NOHEART="0.5"
    ACTIVE_AT_START="true"
    POST_POLICY="once"
    EVAL_TIME="1.5"

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    case "$CASE_NAME" in
        active_start_once_posts_pass)
            DEADFLAGS=("DEAD_ONCE=true")
            PASS_CONDITIONS=("DEAD_ONCE = true")
            REPORT_COLUMNS=("dead_once=\$[DEAD_ONCE]")
            CHECK_TOKENS=("dead_once=true")
            ;;
        active_start_false_no_initial_post_pass)
            ACTIVE_AT_START="false"
            DEADFLAGS=("DEAD_NO_START=true")
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            FAIL_CONDITIONS=("DEAD_NO_START = true")
            REPORT_COLUMNS=("dead_no_start=\$[DEAD_NO_START]")
            ;;
        heartbeat_before_dead_suppresses_pass)
            ACTIVE_AT_START="false"
            MAX_NOHEART="1.0"
            DEADFLAGS=("DEAD_KEEPALIVE=true")
            HEARTBEAT_EVENTS=("0.2" "0.5" "0.8" "1.1" "1.4")
            EVAL_TIME="1.6"
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            FAIL_CONDITIONS=("DEAD_KEEPALIVE = true")
            REPORT_COLUMNS=("dead_keepalive=\$[DEAD_KEEPALIVE]")
            ;;
        stop_heartbeat_posts_pass)
            MAX_NOHEART="0.45"
            DEADFLAGS=("DEAD_STOP=true")
            HEARTBEAT_EVENTS=("0.2" "0.4")
            EVAL_TIME="1.2"
            PASS_CONDITIONS=("DEAD_STOP = true")
            REPORT_COLUMNS=("dead_stop=\$[DEAD_STOP]")
            CHECK_TOKENS=("dead_stop=true")
            ;;
        active_false_after_first_heartbeat_posts_pass)
            ACTIVE_AT_START="false"
            MAX_NOHEART="0.45"
            DEADFLAGS=("DEAD_AFTER_FIRST=true")
            HEARTBEAT_EVENTS=("0.2")
            EVAL_TIME="1.1"
            PASS_CONDITIONS=("DEAD_AFTER_FIRST = true")
            REPORT_COLUMNS=("dead_after_first=\$[DEAD_AFTER_FIRST]")
            CHECK_TOKENS=("dead_after_first=true")
            ;;
        numeric_deadflag_pass)
            DEADFLAGS=("DEAD_NUM=7.5")
            PASS_CONDITIONS=("DEAD_NUM = 7.5")
            REPORT_COLUMNS=("dead_num=\$[DEAD_NUM]")
            CHECK_TOKENS=("dead_num=7.5")
            ;;
        string_deadflag_pass)
            DEADFLAGS=("DEAD_STR=halt")
            PASS_CONDITIONS=("DEAD_STR = halt")
            REPORT_COLUMNS=("dead_str=\$[DEAD_STR]")
            CHECK_TOKENS=("dead_str=halt")
            ;;
        multiple_deadflags_pass)
            DEADFLAGS=("DEAD_MULTI_A=true" "DEAD_MULTI_B=alpha")
            PASS_CONDITIONS=("DEAD_MULTI_A = true" "DEAD_MULTI_B = alpha")
            REPORT_COLUMNS=("dead_multi_a=\$[DEAD_MULTI_A]" "dead_multi_b=\$[DEAD_MULTI_B]")
            CHECK_TOKENS=("dead_multi_a=true" "dead_multi_b=alpha")
            ;;
        once_policy_single_post_pass)
            MAX_NOHEART="0.35"
            DEADFLAGS=("DEAD_ONCE_SINGLE=true")
            PASS_CONDITIONS=("DEAD_ONCE_SINGLE = true")
            REPORT_COLUMNS=("dead_once_single=\$[DEAD_ONCE_SINGLE]")
            CHECK_TOKENS=("dead_once_single=true")
            MIN_COUNTS=("DEAD_ONCE_SINGLE:1")
            MAX_COUNTS=("DEAD_ONCE_SINGLE:1")
            ;;
        repeat_policy_reposts_pass)
            MAX_NOHEART="0.35"
            POST_POLICY="repeat"
            EVAL_TIME="1.4"
            DEADFLAGS=("DEAD_REPEAT=true")
            PASS_CONDITIONS=("DEAD_REPEAT = true")
            REPORT_COLUMNS=("dead_repeat=\$[DEAD_REPEAT]")
            CHECK_TOKENS=("dead_repeat=true")
            MIN_COUNTS=("DEAD_REPEAT:2")
            ;;
        reset_policy_reposts_after_heartbeat_pass)
            MAX_NOHEART="0.30"
            POST_POLICY="reset"
            HEARTBEAT_EVENTS=("0.8")
            EVAL_TIME="1.5"
            DEADFLAGS=("DEAD_RESET=true")
            PASS_CONDITIONS=("DEAD_RESET = true")
            REPORT_COLUMNS=("dead_reset=\$[DEAD_RESET]")
            CHECK_TOKENS=("dead_reset=true")
            MIN_COUNTS=("DEAD_RESET:2")
            ;;
        repeat_policy_inactive_reposts_pass)
            ACTIVE_AT_START="false"
            MAX_NOHEART="0.35"
            POST_POLICY="repeat"
            HEARTBEAT_EVENTS=("0.2")
            EVAL_TIME="1.2"
            DEADFLAGS=("DEAD_REPEAT_AFTER_FIRST=true")
            PASS_CONDITIONS=("DEAD_REPEAT_AFTER_FIRST = true")
            REPORT_COLUMNS=("dead_repeat_after_first=\$[DEAD_REPEAT_AFTER_FIRST]")
            CHECK_TOKENS=("dead_repeat_after_first=true")
            MIN_COUNTS=("DEAD_REPEAT_AFTER_FIRST:2")
            ;;
        reset_policy_no_repost_without_heartbeat_pass)
            MAX_NOHEART="0.35"
            POST_POLICY="reset"
            EVAL_TIME="1.4"
            DEADFLAGS=("DEAD_RESET_SINGLE=true")
            PASS_CONDITIONS=("DEAD_RESET_SINGLE = true")
            REPORT_COLUMNS=("dead_reset_single=\$[DEAD_RESET_SINGLE]")
            CHECK_TOKENS=("dead_reset_single=true")
            MIN_COUNTS=("DEAD_RESET_SINGLE:1")
            MAX_COUNTS=("DEAD_RESET_SINGLE:1")
            ;;
        active_start_keepalive_suppresses_pass)
            MAX_NOHEART="3.50"
            DEADFLAGS=("DEAD_ACTIVE_KEEPALIVE=true")
            HEARTBEAT_EVENTS=("0.2" "0.6" "1.0" "1.4" "1.8")
            EVAL_TIME="1.6"
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            FAIL_CONDITIONS=("DEAD_ACTIVE_KEEPALIVE = true")
            REPORT_COLUMNS=("dead_active_keepalive=\$[DEAD_ACTIVE_KEEPALIVE]")
            MAX_COUNTS=("DEAD_ACTIVE_KEEPALIVE:0")
            ;;
        custom_heartbeat_var_pass)
            ACTIVE_AT_START="false"
            MAX_NOHEART="0.35"
            HEART_VAR="ALT_HEART"
            HEARTBEAT_EVENTS=("0.2")
            EVAL_TIME="1.1"
            DEADFLAGS=("DEAD_ALT_HEART=true")
            PASS_CONDITIONS=("DEAD_ALT_HEART = true")
            REPORT_COLUMNS=("dead_alt_heart=\$[DEAD_ALT_HEART]")
            CHECK_TOKENS=("dead_alt_heart=true")
            MIN_COUNTS=("DEAD_ALT_HEART:1")
            ;;
        negative_deadflag_pass)
            DEADFLAGS=("DEAD_NEG=-3.25")
            PASS_CONDITIONS=("DEAD_NEG = -3.25")
            REPORT_COLUMNS=("dead_neg=\$[DEAD_NEG]")
            CHECK_TOKENS=("dead_neg=-3.25")
            ;;
        zero_max_noheart_posts_pass)
            MAX_NOHEART="0"
            EVAL_TIME="0.8"
            DEADFLAGS=("DEAD_ZERO_MAX=true")
            PASS_CONDITIONS=("DEAD_ZERO_MAX = true")
            REPORT_COLUMNS=("dead_zero_max=\$[DEAD_ZERO_MAX]")
            CHECK_TOKENS=("dead_zero_max=true")
            MIN_COUNTS=("DEAD_ZERO_MAX:1")
            ;;
        false_deadflag_pass)
            DEADFLAGS=("DEAD_FALSE=false")
            PASS_CONDITIONS=("DEAD_FALSE = false")
            REPORT_COLUMNS=("dead_false=\$[DEAD_FALSE]")
            CHECK_TOKENS=("dead_false=false")
            MIN_COUNTS=("DEAD_FALSE:1")
            ;;
        invalid_post_policy_falls_back_once_pass)
            MAX_NOHEART="0.35"
            POST_POLICY="invalid_policy"
            EVAL_TIME="1.3"
            DEADFLAGS=("DEAD_INVALID_POLICY=true")
            PASS_CONDITIONS=("DEAD_INVALID_POLICY = true")
            REPORT_COLUMNS=("dead_invalid_policy=\$[DEAD_INVALID_POLICY]")
            CHECK_TOKENS=("dead_invalid_policy=true")
            MIN_COUNTS=("DEAD_INVALID_POLICY:1")
            MAX_COUNTS=("DEAD_INVALID_POLICY:1")
            ;;
        reset_policy_multiple_reposts_pass)
            MAX_NOHEART="0.30"
            POST_POLICY="reset"
            HEARTBEAT_EVENTS=("1.2" "2.2")
            EVAL_TIME="3.4"
            DEADFLAGS=("DEAD_RESET_MULTI=true")
            PASS_CONDITIONS=("DEAD_RESET_MULTI = true")
            REPORT_COLUMNS=("dead_reset_multi=\$[DEAD_RESET_MULTI]")
            CHECK_TOKENS=("dead_reset_multi=true")
            MIN_COUNTS=("DEAD_RESET_MULTI:3")
            MAX_COUNTS=("DEAD_RESET_MULTI:3")
            ;;
    esac
}

write_case_files() {
    local case_dir="$1"
    local item
    {
        echo "ProcessConfig = pDeadManPost"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        echo "  heartbeat_var   = $HEART_VAR"
        echo "  max_noheart     = $MAX_NOHEART"
        echo "  active_at_start = $ACTIVE_AT_START"
        echo "  post_policy     = $POST_POLICY"
        for item in "${DEADFLAGS[@]}"; do
            echo "  deadflag        = $item"
        done
        echo "}"
    } > "$case_dir/case_deadman.moos"

    {
        echo "ProcessConfig = uTimerScript"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        for item in "${HEARTBEAT_EVENTS[@]}"; do
            echo "  event = var=$HEART_VAR, val=alive, time=$item"
        done
        echo "  event = var=TEST_EVAL_READY, val=true, time=$EVAL_TIME"
        echo "}"
    } > "$case_dir/case_timer.moos"

    {
        echo "ProcessConfig = pMissionEval"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        echo ""
        echo "  lead_condition = TEST_EVAL_READY = true"
        for item in "${PASS_CONDITIONS[@]}"; do
            echo "  pass_condition = $item"
        done
        for item in "${FAIL_CONDITIONS[@]}"; do
            echo "  fail_condition = $item"
        done
        echo ""
        echo "  result_flag  = MISSION_EVALUATED = true"
        echo "  mission_form = pdeadmanpost_unit"
        echo "  mission_mod  = $CASE_NAME"
        echo ""
        echo "  prereport_column = form=\$[MISSION_FORM]"
        echo "  prereport_column = mmod=\$[MMOD]"
        echo "  report_column    = grade=\$[GRADE]"
        echo "  report_column    = eval=\$[TEST_EVAL_READY]"
        for item in "${REPORT_COLUMNS[@]}"; do
            echo "  report_column    = $item"
        done
        echo "  report_column    = mhash=\$[MHASH_SHORT]"
        echo "  report_file      = results.txt"
        echo "}"
    } > "$case_dir/case_eval.moos"
}

count_alog_posts() {
    local case_dir="$1"
    local var="$2"
    local alog

    alog=$(find "$case_dir" -name '*.alog' -print | head -n 1)
    if [ "$alog" = "" ]; then
        echo 0
        return
    fi

    aloggrep "$var" "$alog" 2>/dev/null | awk -v v="$var" '$2 == v {count++} END {print count+0}'
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
    local var
    local expected
    local observed

    get_case_config "$case_name" || return 1

    tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$tag"
    case_result_file="$CASE_RESULT_DIR/$tag.txt"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    write_case_files "$case_dir"

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
        if ! echo "$line" | grep -q "$check"; then
            status="mismatch"
        fi
    done
    for check in "${MIN_COUNTS[@]}"; do
        var="${check%%:*}"
        expected="${check#*:}"
        observed=$(count_alog_posts "$case_dir" "$var")
        if [ "$observed" -lt "$expected" ]; then
            status="mismatch"
        fi
        line="$line  min_${var}=$expected  count_${var}=$observed"
    done
    for check in "${MAX_COUNTS[@]}"; do
        var="${check%%:*}"
        expected="${check#*:}"
        observed=$(count_alog_posts "$case_dir" "$var")
        if [ "$observed" -gt "$expected" ]; then
            status="mismatch"
        fi
        line="$line  max_${var}=$expected  count_${var}=$observed"
    done

    echo "case=$case_name  case_result=$status  expected=pass  actual=$actual  $line" > "$case_result_file"
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
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_pdeadmanpost_unit_XXXXXX")
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
