#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-utimerscript_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; cleanup; exit 1; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP="10"
MAX_TIME="40"
JOBS="1"
PORT_BASE="7300"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/utimerscript_missions/utimerscript_unit"
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
    timed_numeric_string_pass
    quoted_comma_payload_pass
    macro_count_nav_pass
    condition_gate_external_pass
    pause_unpause_external_pass
    forward_jump_next_pass
    reset_var_repost_pass
    reset_time_all_posted_pass
    delay_start_pass
    status_var_initial_pass
    silent_status_pass
    randvar_uniform_bounds_pass
    randvar_gaussian_bounds_pass
    randpair_poly_bounds_pass
    block_on_peer_pass
    atomic_condition_drop_pass
    pause_toggle_external_pass
    forward_positive_skip_pass
    custom_control_vars_pass
    time_warp_accelerates_pass
    delay_reset_pass
    reset_max_one_limit_pass
    amount_multiple_posts_pass
    dbtime_utc_macro_pass
    upon_awake_reset_pass
    upon_awake_restart_pass
    math_expression_pass
    idx_amount_expansion_pass
    time_window_event_pass
    reset_time_numeric_pass
    multi_condition_gate_pass
    shuffle_false_status_pass
    quit_event_pass
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
    EXPECTED="pass"
    CASE_MAX_TIME="$MAX_TIME"
    POKE_STEPS=()
    PASS_CONDITIONS=()
    REPORT_COLUMNS=()
    EXTRA_CHECKS=()
    TIMER_LINES=()

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    case "$CASE_NAME" in
        timed_numeric_string_pass)
            TIMER_LINES=(
                "script_name = timed_numeric_string"
                "event = var=UTS_NUM, val=42, time=0.2"
                "event = var=UTS_STR, val=alpha, time=0.3"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_NUM = 42" "UTS_STR = alpha")
            REPORT_COLUMNS=("uts_num=\$[UTS_NUM]" "uts_str=\$[UTS_STR]")
            ;;
        quoted_comma_payload_pass)
            TIMER_LINES=(
                "script_name = quoted_payload"
                "event = var=UTS_PAYLOAD, val=\"name=alpha,mode=survey\", time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            EXTRA_CHECKS=("line:payload=name=alpha,mode=survey")
            REPORT_COLUMNS=("payload=\$[UTS_PAYLOAD]")
            ;;
        macro_count_nav_pass)
            TIMER_LINES=(
                "script_name = macro_count_nav"
                "event = var=NAV_X, val=12.5, time=0.1"
                "event = var=NAV_Y, val=-4.25, time=0.1"
                "event = var=UTS_MACRO, val=\"x=\$[NAV_X],y=\$[NAV_Y],count=\$[COUNT],tcount=\$[TCOUNT]\", time=0.4"
                "event = var=TEST_EVAL_READY, val=true, time=0.7"
            )
            EXTRA_CHECKS=("line:macro=x=12.50,y=-4.25")
            REPORT_COLUMNS=("macro=\$[UTS_MACRO]")
            ;;
        condition_gate_external_pass)
            TIMER_LINES=(
                "script_name = condition_gate"
                "condition = UTS_GATE = true"
                "event = var=UTS_GATED, val=released, time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            POKE_STEPS=("0.8:UTS_GATE=true")
            PASS_CONDITIONS=("UTS_GATED = released")
            REPORT_COLUMNS=("gated=\$[UTS_GATED]" "gate=\$[UTS_GATE]")
            EXTRA_CHECKS=("line:gated=released")
            ;;
        pause_unpause_external_pass)
            TIMER_LINES=(
                "script_name = pause_unpause"
                "paused = true"
                "event = var=UTS_PAUSED_OUT, val=after_pause, time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            POKE_STEPS=("0.8:UTS_PAUSE=false")
            PASS_CONDITIONS=("UTS_PAUSED_OUT = after_pause")
            REPORT_COLUMNS=("paused_out=\$[UTS_PAUSED_OUT]")
            EXTRA_CHECKS=("line:paused_out=after_pause")
            ;;
        forward_jump_next_pass)
            TIMER_LINES=(
                "script_name = forward_jump"
                "event = var=UTS_LATE, val=jumped, time=30"
                "event = var=TEST_EVAL_READY, val=true, time=31"
            )
            POKE_STEPS=("0.7:UTS_FORWARD=0")
            PASS_CONDITIONS=("UTS_LATE = jumped")
            REPORT_COLUMNS=("late=\$[UTS_LATE]")
            EXTRA_CHECKS=("line:late=jumped")
            ;;
        reset_var_repost_pass)
            TIMER_LINES=(
                "script_name = reset_var_repost"
                "event = var=UTS_COUNT, val=\$[TCOUNT], time=0.2"
            )
            POKE_STEPS=("0.7:UTS_RESET=true" "0.7:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_COUNT >= 1")
            REPORT_COLUMNS=("uts_count=\$[UTS_COUNT]")
            EXTRA_CHECKS=("count:UTS_COUNT:2")
            ;;
        reset_time_all_posted_pass)
            TIMER_LINES=(
                "script_name = reset_time_all_posted"
                "reset_time = all-posted"
                "reset_max = 2"
                "event = var=UTS_REPEAT, val=\$[TCOUNT], time=0.2"
            )
            POKE_STEPS=("1.0:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_REPEAT >= 1")
            REPORT_COLUMNS=("repeat=\$[UTS_REPEAT]")
            EXTRA_CHECKS=("count:UTS_REPEAT:2")
            ;;
        delay_start_pass)
            TIMER_LINES=(
                "script_name = delay_start"
                "delay_start = 1.0"
                "event = var=UTS_DELAYED, val=true, time=0.1"
                "event = var=TEST_EVAL_READY, val=true, time=1.3"
            )
            PASS_CONDITIONS=("UTS_DELAYED = true")
            REPORT_COLUMNS=("delayed=\$[UTS_DELAYED]")
            ;;
        status_var_initial_pass)
            TIMER_LINES=(
                "script_name = status_case"
                "status_var = UTS_CUSTOM_STATUS"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            REPORT_COLUMNS=("custom_status=\$[UTS_CUSTOM_STATUS]")
            EXTRA_CHECKS=("line:custom_status=name=status_case" "line:pending=0")
            ;;
        silent_status_pass)
            TIMER_LINES=(
                "script_name = silent_status"
                "status_var = silent"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            EXTRA_CHECKS=("absent:UTS_STATUS")
            ;;
        randvar_uniform_bounds_pass)
            TIMER_LINES=(
                "script_name = randvar_uniform"
                "randvar = varname=RV, min=5, max=7, key=at_post"
                "event = var=UTS_RAND, val=\$[RV], time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_RAND >= 5" "UTS_RAND <= 7")
            REPORT_COLUMNS=("rand=\$[UTS_RAND]")
            ;;
        randvar_gaussian_bounds_pass)
            TIMER_LINES=(
                "script_name = randvar_gaussian"
                "randvar = varname=GV, type=gaussian, min=10, max=14, mu=12, sigma=0.5, key=at_post"
                "event = var=UTS_GAUSS, val=\$[GV], time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_GAUSS >= 10" "UTS_GAUSS <= 14")
            REPORT_COLUMNS=("gauss=\$[UTS_GAUSS]")
            ;;
        randpair_poly_bounds_pass)
            TIMER_LINES=(
                "script_name = randpair_poly"
                "rand_pair = var1=PX, var2=PY, type=poly, key=at_post, poly=\"pts={0,0:10,0:10,10:0,10}\""
                "event = var=UTS_RPX, val=\$[PX], time=0.2"
                "event = var=UTS_RPY, val=\$[PY], time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_RPX >= 0" "UTS_RPX <= 10" "UTS_RPY >= 0" "UTS_RPY <= 10")
            REPORT_COLUMNS=("rpx=\$[UTS_RPX]" "rpy=\$[UTS_RPY]")
            ;;
        block_on_peer_pass)
            TIMER_LINES=(
                "script_name = block_on_peer"
                "block_on = pMissionHash"
                "event = var=UTS_BLOCKED_OUT, val=unblocked, time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_BLOCKED_OUT = unblocked")
            REPORT_COLUMNS=("blocked_out=\$[UTS_BLOCKED_OUT]")
            EXTRA_CHECKS=("line:blocked_out=unblocked")
            ;;
        atomic_condition_drop_pass)
            CASE_MAX_TIME="50"
            TIMER_LINES=(
                "script_name = atomic_condition_drop"
                "script_atomic = true"
                "condition = UTS_RUN = true"
                "event = var=UTS_ATOMIC_FIRST, val=true, time=1"
                "event = var=UTS_ATOMIC_SECOND, val=true, time=10"
                "event = var=TEST_EVAL_READY, val=true, time=11"
            )
            POKE_STEPS=("0.4:UTS_RUN=true" "0.8:UTS_RUN=false")
            PASS_CONDITIONS=("UTS_ATOMIC_FIRST = true" "UTS_ATOMIC_SECOND = true")
            REPORT_COLUMNS=("atomic_first=\$[UTS_ATOMIC_FIRST]" "atomic_second=\$[UTS_ATOMIC_SECOND]" "run=\$[UTS_RUN]")
            EXTRA_CHECKS=("line:atomic_second=true")
            ;;
        pause_toggle_external_pass)
            TIMER_LINES=(
                "script_name = pause_toggle"
                "event = var=UTS_TOGGLE_OUT, val=after_toggle, time=0.5"
                "event = var=TEST_EVAL_READY, val=true, time=0.8"
            )
            POKE_STEPS=("0.1:UTS_PAUSE:=toggle" "0.9:UTS_PAUSE:=toggle")
            PASS_CONDITIONS=("UTS_TOGGLE_OUT = after_toggle")
            REPORT_COLUMNS=("toggle_out=\$[UTS_TOGGLE_OUT]")
            EXTRA_CHECKS=("line:toggle_out=after_toggle")
            ;;
        forward_positive_skip_pass)
            TIMER_LINES=(
                "script_name = forward_positive_skip"
                "event = var=UTS_FORWARD_POS, val=skipped, time=5"
                "event = var=TEST_EVAL_READY, val=true, time=6"
            )
            POKE_STEPS=("0.7:UTS_FORWARD=5")
            PASS_CONDITIONS=("UTS_FORWARD_POS = skipped")
            REPORT_COLUMNS=("forward_pos=\$[UTS_FORWARD_POS]")
            EXTRA_CHECKS=("line:forward_pos=skipped")
            ;;
        custom_control_vars_pass)
            TIMER_LINES=(
                "script_name = custom_controls"
                "pause_var = UTS_HOLD"
                "forward_var = UTS_SKIP"
                "reset_var = UTS_REDO"
                "paused = true"
                "event = var=UTS_CUSTOM_CTRL, val=\$[TCOUNT], time=5"
            )
            POKE_STEPS=("0.5:UTS_HOLD:=false" "0.5:UTS_SKIP=5" "0.5:UTS_REDO:=true" "0.5:UTS_SKIP=5" "0.7:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_CUSTOM_CTRL >= 1")
            REPORT_COLUMNS=("custom_ctrl=\$[UTS_CUSTOM_CTRL]")
            EXTRA_CHECKS=("line:custom_ctrl=1" "line:resets=1/any")
            ;;
        time_warp_accelerates_pass)
            TIMER_LINES=(
                "script_name = time_warp_accelerates"
                "time_warp = 4"
                "event = var=UTS_WARPED, val=true, time=2"
                "event = var=TEST_EVAL_READY, val=true, time=2.2"
            )
            PASS_CONDITIONS=("UTS_WARPED = true")
            REPORT_COLUMNS=("warped=\$[UTS_WARPED]")
            EXTRA_CHECKS=("line:time_warp=4")
            ;;
        delay_reset_pass)
            TIMER_LINES=(
                "script_name = delay_reset"
                "reset_time = all-posted"
                "reset_max = 1"
                "delay_reset = 1.0"
                "event = var=UTS_DELAY_RESET, val=\$[TCOUNT], time=0.2"
            )
            POKE_STEPS=("2.0:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_DELAY_RESET >= 1")
            REPORT_COLUMNS=("delay_reset_out=\$[UTS_DELAY_RESET]")
            EXTRA_CHECKS=("count:UTS_DELAY_RESET:2" "line:delay_reset=1")
            ;;
        reset_max_one_limit_pass)
            TIMER_LINES=(
                "script_name = reset_max_one_limit"
                "reset_time = all-posted"
                "reset_max = 1"
                "event = var=UTS_LIMITED_REPEAT, val=\$[TCOUNT], time=0.2"
            )
            POKE_STEPS=("2.0:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_LIMITED_REPEAT >= 1")
            REPORT_COLUMNS=("limited_repeat=\$[UTS_LIMITED_REPEAT]")
            EXTRA_CHECKS=("line:limited_repeat=1" "line:resets=1/1")
            ;;
        amount_multiple_posts_pass)
            TIMER_LINES=(
                "script_name = amount_multiple_posts"
                "event = var=UTS_AMOUNT, val=burst, time=0.2, amt=3"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_AMOUNT = burst")
            REPORT_COLUMNS=("amount=\$[UTS_AMOUNT]")
            EXTRA_CHECKS=("line:posted=4")
            ;;
        dbtime_utc_macro_pass)
            TIMER_LINES=(
                "script_name = dbtime_utc_macro"
                "event = var=UTS_DBTIME, val=\$[DBTIME], time=0.2"
                "event = var=UTS_UTC, val=\$[UTC], time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_DBTIME > 0" "UTS_UTC > 0")
            REPORT_COLUMNS=("dbtime=\$[UTS_DBTIME]" "utc=\$[UTS_UTC]")
            ;;
        upon_awake_reset_pass)
            TIMER_LINES=(
                "script_name = upon_awake_reset"
                "condition = UTS_AWAKE = true"
                "upon_awake = reset"
                "event = var=UTS_AWAKE_RESET, val=\$[TCOUNT], time=0.2"
            )
            POKE_STEPS=("0.1:UTS_AWAKE=true" "0.7:UTS_AWAKE=false" "0.7:UTS_AWAKE=true" "0.8:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_AWAKE_RESET >= 1")
            REPORT_COLUMNS=("awake_reset=\$[UTS_AWAKE_RESET]")
            EXTRA_CHECKS=("line:awake_reset=1" "line:resets=1/any")
            ;;
        upon_awake_restart_pass)
            TIMER_LINES=(
                "script_name = upon_awake_restart"
                "condition = UTS_RESTART_GATE = true"
                "upon_awake = restart"
                "event = var=UTS_AWAKE_RESTART, val=\$[TCOUNT], time=0.2"
            )
            POKE_STEPS=("0.1:UTS_RESTART_GATE=true" "0.7:UTS_RESTART_GATE=false" "0.7:UTS_RESTART_GATE=true" "0.8:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_AWAKE_RESTART >= 1")
            REPORT_COLUMNS=("awake_restart=\$[UTS_AWAKE_RESTART]")
            EXTRA_CHECKS=("line:awake_restart=1" "line:resets=0/any")
            ;;
        math_expression_pass)
            TIMER_LINES=(
                "script_name = math_expression"
                "event = var=UTS_MATH, val={{3+2}*4}, time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_MATH = 20")
            REPORT_COLUMNS=("math=\$[UTS_MATH]")
            EXTRA_CHECKS=("line:math=20")
            ;;
        idx_amount_expansion_pass)
            TIMER_LINES=(
                "script_name = idx_amount"
                "event = var=UTS_IDX, val=idx_\$[IDX], time=0.2, amt=3"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_IDX = idx_002")
            REPORT_COLUMNS=("idx=\$[UTS_IDX]")
            EXTRA_CHECKS=("line:idx=idx_002" "line:posted=4")
            ;;
        time_window_event_pass)
            TIMER_LINES=(
                "script_name = time_window_event"
                "event = var=UTS_WINDOWED, val=true, time=0.2:0.8"
                "event = var=TEST_EVAL_READY, val=true, time=1.1"
            )
            PASS_CONDITIONS=("UTS_WINDOWED = true")
            REPORT_COLUMNS=("windowed=\$[UTS_WINDOWED]")
            EXTRA_CHECKS=("line:windowed=true")
            ;;
        reset_time_numeric_pass)
            TIMER_LINES=(
                "script_name = reset_time_numeric"
                "reset_time = 0.6"
                "reset_max = 1"
                "event = var=UTS_NUMERIC_RESET, val=\$[TCOUNT], time=0.2"
            )
            POKE_STEPS=("1.4:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_NUMERIC_RESET >= 1")
            REPORT_COLUMNS=("numeric_reset=\$[UTS_NUMERIC_RESET]")
            EXTRA_CHECKS=("line:numeric_reset=1" "line:resets=1/1")
            ;;
        multi_condition_gate_pass)
            TIMER_LINES=(
                "script_name = multi_condition_gate"
                "condition = UTS_GATE_A = true"
                "condition = UTS_GATE_B = ready"
                "event = var=UTS_MULTI_GATED, val=released, time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            POKE_STEPS=("0.5:UTS_GATE_A=true" "0.5:UTS_GATE_B:=ready")
            PASS_CONDITIONS=("UTS_MULTI_GATED = released")
            REPORT_COLUMNS=("multi_gated=\$[UTS_MULTI_GATED]" "gate_a=\$[UTS_GATE_A]" "gate_b=\$[UTS_GATE_B]")
            EXTRA_CHECKS=("line:multi_gated=released")
            ;;
        shuffle_false_status_pass)
            TIMER_LINES=(
                "script_name = shuffle_false_status"
                "shuffle = false"
                "event = var=UTS_SHUFFLE_FALSE, val=true, time=0.2"
                "event = var=TEST_EVAL_READY, val=true, time=0.5"
            )
            PASS_CONDITIONS=("UTS_SHUFFLE_FALSE = true")
            REPORT_COLUMNS=("shuffle_false=\$[UTS_SHUFFLE_FALSE]")
            EXTRA_CHECKS=("line:shuffle=false")
            ;;
        quit_event_pass)
            TIMER_LINES=(
                "script_name = quit_event"
                "event = var=UTS_QUIT_READY, val=true, time=0.2"
                "event = quit, time=0.4"
            )
            POKE_STEPS=("1.0:TEST_EVAL_READY=true")
            PASS_CONDITIONS=("UTS_QUIT_READY = true")
            REPORT_COLUMNS=("quit_ready=\$[UTS_QUIT_READY]")
            EXTRA_CHECKS=("line:quit_ready=true")
            ;;
    esac
}

write_timer_config() {
    local case_dir="$1"
    local timer_file="$case_dir/case_utimerscript.moos"
    {
        echo "//-------------------------------------------------"
        echo "// FILE: case_utimerscript.moos"
        echo "// NAME: Charles Benjamin"
        echo "//-------------------------------------------------"
        echo ""
        echo "ProcessConfig = uTimerScript"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        for line in "${TIMER_LINES[@]}"; do
            echo "  $line"
        done
        echo "}"
    } > "$timer_file"
}

write_eval_config() {
    local case_dir="$1"
    local eval_file="$case_dir/case_eval.moos"
    {
        echo "//-------------------------------------------------"
        echo "// FILE: case_eval.moos"
        echo "// NAME: Charles Benjamin"
        echo "//-------------------------------------------------"
        echo ""
        echo "ProcessConfig = pMissionHash"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        echo ""
        echo "  mission_hash_var = MISSION_HASH"
        echo "  mhash_short_var  = MHASH"
        echo "}"
        echo ""
        echo "//----------------------------------------------------"
        echo "ProcessConfig = pMissionEval"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        echo ""
        echo "  lead_condition = TEST_EVAL_READY = true"
        for cond in "${PASS_CONDITIONS[@]}"; do
            echo "  pass_condition = $cond"
        done
        echo ""
        echo "  result_flag  = MISSION_EVALUATED = true"
        echo "  mission_form = utimerscript_unit"
        echo "  mission_mod  = $CASE_NAME"
        echo ""
        echo "  prereport_column = form=\$[MISSION_FORM]"
        echo "  prereport_column = mmod=\$[MMOD]"
        echo "  report_column    = grade=\$[GRADE]"
        echo "  report_column    = eval=\$[TEST_EVAL_READY]"
        for col in "${REPORT_COLUMNS[@]}"; do
            echo "  report_column    = $col"
        done
        echo "  report_column    = status=\$[UTS_STATUS]"
        echo "  report_column    = mhash=\$[MHASH_SHORT]"
        echo "  report_file      = results.txt"
        echo "}"
    } > "$eval_file"
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

find_alog() {
    local case_dir="$1"
    find "$case_dir" -name '*.alog' -print | sort | tail -n 1
}

alog_var_line_matches() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | rg "$var" | rg -q "$pattern"
}

alog_var_count() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || {
        echo 0
        return
    }
    aloggrep "$alog" "$var" 2>/dev/null | rg "^[[:space:]]*[0-9].*[[:space:]]$var[[:space:]]" | wc -l | tr -d ' '
}

run_pokes() {
    local port="$1"
    local step
    local delay
    local batch

    for step in "${POKE_STEPS[@]}"; do
        delay="${step%%:*}"
        batch="${step#*:}"
        sleep "$delay"
        uPokeDB --quiet --host=localhost --port="$port" $batch >/dev/null 2>&1 || return 1
    done
}

extra_checks_ok() {
    local case_dir="$1"
    local result_line="$2"
    local check
    local kind
    local rest
    local var
    local pattern
    local min_count
    local actual_count

    for check in "${EXTRA_CHECKS[@]}"; do
        kind="${check%%:*}"
        rest="${check#*:}"
        case "$kind" in
            alog)
                var="${rest%%:*}"
                pattern="${rest#*:}"
                alog_var_line_matches "$case_dir" "$var" "$pattern" || return 1
                ;;
            absent)
                var="$rest"
                actual_count=$(alog_var_count "$case_dir" "$var")
                [ "$actual_count" -eq 0 ] || return 1
                ;;
            count)
                var="${rest%%:*}"
                min_count="${rest#*:}"
                actual_count=$(alog_var_count "$case_dir" "$var")
                [ "$actual_count" -ge "$min_count" ] || return 1
                ;;
            count_exact)
                var="${rest%%:*}"
                min_count="${rest#*:}"
                actual_count=$(alog_var_count "$case_dir" "$var")
                [ "$actual_count" -eq "$min_count" ] || return 1
                ;;
            line)
                echo "$result_line" | rg -Fq "$rest" || return 1
                ;;
            *)
                return 1
                ;;
        esac
    done
    return 0
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
    local launch_rc=0
    local poke_rc=0

    get_case_config "$case_name" || return 1

    tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$tag"
    case_result_file="$CASE_RESULT_DIR/$tag.txt"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/

    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )
    write_timer_config "$case_dir"
    write_eval_config "$case_dir"

    (
        cd "$case_dir"
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
    launch_rc=$?
    sleep 0.5
    run_pokes "$shore_mport"
    poke_rc=$?
    (
        cd "$case_dir"
        uMayFinish --max_time="$CASE_MAX_TIME" targ_shoreside.moos > mayfinish.out 2>&1 || true
    )
    sleep 0.5
    line=$(wait_for_results "$case_dir/results.txt" 160)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^, ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    if [ "$launch_rc" != 0 ] || [ "$poke_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    elif ! extra_checks_ok "$case_dir" "$line"; then
        status="evidence_mismatch"
    fi

    echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" > "$case_result_file"
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
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_utimerscript_unit_XXXXXX")
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

cat "$RESULTS_FILE"

if grep -q 'case_result=mismatch\|case_result=error\|case_result=evidence_mismatch' "$RESULTS_FILE"; then
    ALL_OK="no"
fi

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
