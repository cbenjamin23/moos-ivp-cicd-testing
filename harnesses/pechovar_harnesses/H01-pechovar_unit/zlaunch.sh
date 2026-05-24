#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-pechovar_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; cleanup; exit 1; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP="10"
MAX_TIME="20"
JOBS="1"
PORT_BASE="17600"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/pechovar_missions/pechovar_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
RUN_ROOT=""
CASE_ROW_DIR=""
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
    numeric_echo_pass
    string_echo_pass
    bool_switch_true_pass
    bool_switch_mixed_case_pass
    bool_switch_upper_false_pass
    bool_switch_numeric_passthrough_pass
    bool_switch_nonbool_passthrough_pass
    multi_target_echo_pass
    acyclic_chain_echo_pass
    duplicate_mapping_single_post_pass
    default_repeated_source_posts_each_pass
    latest_only_repeated_source_pass
    flip_component_pass
    flip_explicit_param_syntax_pass
    flip_filter_match_pass
    flip_multiple_filters_match_pass
    flip_filter_case_insensitive_match_pass
    flip_missing_filter_allows_pass
    flip_filter_suppresses_pass
    flip_multiple_filters_suppress_pass
    flip_custom_separator_pass
    flip_component_case_sensitive_pass
    flip_unmapped_component_omitted_pass
    flip_no_mapped_fields_no_output_pass
    flip_duplicate_component_repeats_pass
    flip_empty_value_pass
    cycle_guard_no_echo_pass
    self_cycle_guard_no_echo_pass
    indirect_cycle_blocks_all_pass
    invalid_flip_no_output_pass
    invalid_flip_alias_no_output_pass
    shared_source_dual_flip_pass
    dual_flip_independent_pass
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

add_timer_event() {
    local value="$2"
    if [[ "$value" == *","* || "$value" == *"|"* || "$value" == *"#"* || "$value" == *"@"* ]]; then
        value="\"$value\""
    fi
    TIMER_EVENTS+=("  event = var=$1, val=$value, time=$3")
}

get_case_config() {
    CASE_NAME="$1"
    ECHO_LINES=()
    TIMER_EVENTS=()
    PASS_CONDITIONS=()
    REPORT_COLUMNS=()
    REQUIRED_TOKENS=()
    ABSENT_TOKENS=()
    COUNT_VAR=""
    COUNT_EXPECTED=""

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    REPORT_COLUMNS=("out_num=\$[ECHO_OUT_NUM]" "out_str=\$[ECHO_OUT_STR]" "out_bool=\$[ECHO_OUT_BOOL]" "out_a=\$[ECHO_OUT_A]" "out_b=\$[ECHO_OUT_B]" "latest=\$[ECHO_OUT_LATEST]" "flip_out=\$[FLIP_OUT]")

    case "$CASE_NAME" in
        numeric_echo_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_NUM -> ECHO_OUT_NUM")
            add_timer_event ECHO_SRC_NUM 42.5 0.5
            PASS_CONDITIONS=("ECHO_OUT_NUM = 42.5")
            REQUIRED_TOKENS=(" ECHO_OUT_NUM ")
            ;;
        string_echo_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_STR -> ECHO_OUT_STR")
            add_timer_event ECHO_SRC_STR ready 0.5
            PASS_CONDITIONS=("ECHO_OUT_STR = ready")
            REQUIRED_TOKENS=(" ECHO_OUT_STR ")
            ;;
        bool_switch_true_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_BOOL !-> ECHO_OUT_BOOL")
            add_timer_event ECHO_SRC_BOOL true 0.5
            PASS_CONDITIONS=("ECHO_OUT_BOOL = false")
            REQUIRED_TOKENS=(" ECHO_OUT_BOOL ")
            ;;
        bool_switch_mixed_case_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_BOOL !-> ECHO_OUT_BOOL")
            add_timer_event ECHO_SRC_BOOL True 0.5
            PASS_CONDITIONS=("ECHO_OUT_BOOL = False")
            REQUIRED_TOKENS=(" ECHO_OUT_BOOL ")
            ;;
        bool_switch_upper_false_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_BOOL !-> ECHO_OUT_BOOL")
            add_timer_event ECHO_SRC_BOOL FALSE 0.5
            PASS_CONDITIONS=("ECHO_OUT_BOOL = TRUE")
            REQUIRED_TOKENS=(" ECHO_OUT_BOOL ")
            ;;
        bool_switch_numeric_passthrough_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_NUM !-> ECHO_OUT_NUM")
            add_timer_event ECHO_SRC_NUM 7 0.5
            PASS_CONDITIONS=("ECHO_OUT_NUM = 7")
            REQUIRED_TOKENS=(" ECHO_OUT_NUM ")
            ;;
        bool_switch_nonbool_passthrough_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_BOOL !-> ECHO_OUT_BOOL")
            add_timer_event ECHO_SRC_BOOL maybe 0.5
            PASS_CONDITIONS=("ECHO_OUT_BOOL = maybe")
            REQUIRED_TOKENS=(" ECHO_OUT_BOOL ")
            ;;
        multi_target_echo_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_MULTI -> ECHO_OUT_A" "  Echo = ECHO_SRC_MULTI -> ECHO_OUT_B")
            add_timer_event ECHO_SRC_MULTI alpha 0.5
            PASS_CONDITIONS=("ECHO_OUT_A = alpha" "ECHO_OUT_B = alpha")
            REQUIRED_TOKENS=(" ECHO_OUT_A " " ECHO_OUT_B ")
            ;;
        acyclic_chain_echo_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_STR -> ECHO_OUT_A" "  Echo = ECHO_OUT_A -> ECHO_OUT_B")
            add_timer_event ECHO_SRC_STR chain 0.5
            PASS_CONDITIONS=("ECHO_OUT_A = chain" "ECHO_OUT_B = chain")
            REQUIRED_TOKENS=(" ECHO_OUT_A " " ECHO_OUT_B ")
            ;;
        duplicate_mapping_single_post_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_STR -> ECHO_OUT_STR" "  Echo = ECHO_SRC_STR -> ECHO_OUT_STR")
            add_timer_event ECHO_SRC_STR dedupe 0.5
            PASS_CONDITIONS=("ECHO_OUT_STR = dedupe")
            REQUIRED_TOKENS=(" ECHO_OUT_STR ")
            COUNT_VAR="ECHO_OUT_STR"
            COUNT_EXPECTED="1"
            ;;
        default_repeated_source_posts_each_pass)
            ECHO_LINES=("  Echo = ECHO_SRC_LATEST -> ECHO_OUT_LATEST")
            add_timer_event ECHO_SRC_LATEST first 0.5
            add_timer_event ECHO_SRC_LATEST second 0.7
            PASS_CONDITIONS=("ECHO_OUT_LATEST = second")
            REQUIRED_TOKENS=(" ECHO_OUT_LATEST ")
            COUNT_VAR="ECHO_OUT_LATEST"
            COUNT_EXPECTED="2"
            ;;
        latest_only_repeated_source_pass)
            ECHO_LINES=("  echo_latest_only = true" "  Echo = ECHO_SRC_LATEST -> ECHO_OUT_LATEST")
            add_timer_event ECHO_SRC_LATEST first 0.5
            add_timer_event ECHO_SRC_LATEST second 0.7
            PASS_CONDITIONS=("ECHO_OUT_LATEST = second")
            REQUIRED_TOKENS=(" ECHO_OUT_LATEST ")
            ;;
        flip_component_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = dest_separator = #" "  FLIP:click = x -> xcenter" "  FLIP:click = y -> ycenter")
            add_timer_event FLIP_SRC "x=10,y=20,type=redeploy" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=10#ycenter=20")
            ;;
        flip_explicit_param_syntax_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = filter = type == redeploy" "  FLIP:click = component = x -> xcenter")
            add_timer_event FLIP_SRC "type=redeploy,x=18" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=18")
            ;;
        flip_filter_match_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = dest_separator = #" "  FLIP:click = type == redeploy" "  FLIP:click = x -> xcenter" "  FLIP:click = y -> ycenter")
            add_timer_event FLIP_SRC "type=redeploy,x=11,y=21" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=11#ycenter=21")
            ;;
        flip_multiple_filters_match_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = type == redeploy" "  FLIP:click = mode == survey" "  FLIP:click = x -> xcenter" "  FLIP:click = y -> ycenter")
            add_timer_event FLIP_SRC "type=redeploy,mode=survey,x=19,y=29" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=19,ycenter=29")
            ;;
        flip_filter_case_insensitive_match_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = type == redeploy" "  FLIP:click = x -> xcenter" "  FLIP:click = y -> ycenter")
            add_timer_event FLIP_SRC "TYPE=REDEPLOY,x=14,y=24" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=14,ycenter=24")
            ;;
        flip_missing_filter_allows_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = type == redeploy" "  FLIP:click = x -> xcenter" "  FLIP:click = y -> ycenter")
            add_timer_event FLIP_SRC "x=12,y=22" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=12,ycenter=22")
            ;;
        flip_filter_suppresses_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = type == redeploy" "  FLIP:click = x -> xcenter" "  FLIP:click = y -> ycenter")
            add_timer_event FLIP_SRC "type=loiter,x=12,y=22" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            ABSENT_TOKENS=(" FLIP_OUT ")
            ;;
        flip_multiple_filters_suppress_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = type == redeploy" "  FLIP:click = mode == survey" "  FLIP:click = x -> xcenter" "  FLIP:click = y -> ycenter")
            add_timer_event FLIP_SRC "type=redeploy,mode=loiter,x=20,y=30" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            ABSENT_TOKENS=(" FLIP_OUT ")
            ;;
        flip_custom_separator_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = source_separator = |" "  FLIP:click = dest_separator = @" "  FLIP:click = mode == survey" "  FLIP:click = x -> xpos" "  FLIP:click = y -> ypos")
            add_timer_event FLIP_SRC "mode=survey|x=5|y=-2" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xpos=5@ypos=-2")
            ;;
        flip_component_case_sensitive_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = X -> xcenter")
            add_timer_event FLIP_SRC "x=15,X=16" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=16")
            ABSENT_TOKENS=("xcenter=15")
            ;;
        flip_unmapped_component_omitted_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = x -> xcenter")
            add_timer_event FLIP_SRC "x=8,z=99" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=8")
            ABSENT_TOKENS=("xcenter=8,z=99")
            ;;
        flip_no_mapped_fields_no_output_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = x -> xcenter")
            add_timer_event FLIP_SRC "y=33" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            ABSENT_TOKENS=(" FLIP_OUT ")
            ;;
        flip_duplicate_component_repeats_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = x -> xcenter" "  FLIP:click = y -> ycenter")
            add_timer_event FLIP_SRC "x=10,x=11,y=20" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=10,xcenter=11,ycenter=20")
            ;;
        flip_empty_value_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = x -> xcenter")
            add_timer_event FLIP_SRC "x=" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=")
            ;;
        cycle_guard_no_echo_pass)
            ECHO_LINES=("  Echo = CYCLE_A -> CYCLE_B" "  Echo = CYCLE_B -> CYCLE_A")
            add_timer_event CYCLE_A one 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            ABSENT_TOKENS=(" CYCLE_B ")
            ;;
        self_cycle_guard_no_echo_pass)
            ECHO_LINES=("  Echo = CYCLE_A -> CYCLE_A")
            add_timer_event CYCLE_A self 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            COUNT_VAR="CYCLE_A"
            COUNT_EXPECTED="1"
            ;;
        indirect_cycle_blocks_all_pass)
            ECHO_LINES=("  Echo = CYCLE_A -> CYCLE_B" "  Echo = CYCLE_B -> CYCLE_C" "  Echo = CYCLE_C -> CYCLE_A" "  Echo = ECHO_SRC_STR -> ECHO_OUT_STR")
            add_timer_event CYCLE_A one 0.5
            add_timer_event ECHO_SRC_STR should_not_echo 0.6
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            ABSENT_TOKENS=(" CYCLE_B " " ECHO_OUT_STR ")
            ;;
        invalid_flip_no_output_pass)
            ECHO_LINES=("  FLIP:bad = source_variable = FLIP_SRC" "  FLIP:bad = dest_variable = FLIP_OUT")
            add_timer_event FLIP_SRC "x=13,y=23" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            ABSENT_TOKENS=(" FLIP_OUT ")
            ;;
        invalid_flip_alias_no_output_pass)
            ECHO_LINES=("  FLIP:bad = source_variable = FLIP_SRC" "  FLIP:bad = dest_variable = FLIP_SRC" "  FLIP:bad = x -> xcenter")
            add_timer_event FLIP_SRC "x=17" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            ABSENT_TOKENS=("xcenter=17")
            ;;
        shared_source_dual_flip_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = x -> xcenter" "  FLIP:mark = source_variable = FLIP_SRC" "  FLIP:mark = dest_variable = ECHO_OUT_STR" "  FLIP:mark = y -> ypos")
            add_timer_event FLIP_SRC "x=31,y=41" 0.5
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=31" "ypos=41")
            ;;
        dual_flip_independent_pass)
            ECHO_LINES=("  FLIP:click = source_variable = FLIP_SRC" "  FLIP:click = dest_variable = FLIP_OUT" "  FLIP:click = x -> xcenter" "  FLIP:alt = source_variable = ECHO_SRC_STR" "  FLIP:alt = dest_variable = ECHO_OUT_STR" "  FLIP:alt = word -> word_out")
            add_timer_event FLIP_SRC "x=44" 0.5
            add_timer_event ECHO_SRC_STR "word=bravo" 0.6
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            REQUIRED_TOKENS=("xcenter=44" "word_out=bravo")
            ;;
    esac

    add_timer_event TEST_EVAL_READY true 1.5
}

write_case_files() {
    local case_dir="$1"
    local line

    {
        echo "//-------------------------------------------------"
        echo "// FILE: case_echo.moos"
        echo "// NAME: Charles Benjamin"
        echo "//-------------------------------------------------"
        echo ""
        echo "ProcessConfig = pEchoVar"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        for line in "${ECHO_LINES[@]}"; do
            echo "$line"
        done
        echo "}"
    } > "$case_dir/case_echo.moos"

    {
        echo "//-------------------------------------------------"
        echo "// FILE: case_timer.moos"
        echo "// NAME: Charles Benjamin"
        echo "//-------------------------------------------------"
        echo ""
        echo "ProcessConfig = uTimerScript"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        for line in "${TIMER_EVENTS[@]}"; do
            echo "$line"
        done
        echo "}"
    } > "$case_dir/case_timer.moos"

    {
        echo "//-------------------------------------------------"
        echo "// FILE: case_eval.moos"
        echo "// NAME: Charles Benjamin"
        echo "//-------------------------------------------------"
        echo ""
        echo "ProcessConfig = pMissionEval"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        echo ""
        echo "  lead_condition = TEST_EVAL_READY = true"
        for line in "${PASS_CONDITIONS[@]}"; do
            echo "  pass_condition = $line"
        done
        echo ""
        echo "  result_flag  = MISSION_EVALUATED = true"
        echo "  mission_form = pechovar_unit"
        echo "  mission_mod  = \$(MMOD:=numeric_echo_pass)"
        echo ""
        echo "  prereport_column = form=\$[MISSION_FORM]"
        echo "  prereport_column = mmod=\$[MMOD]"
        echo "  report_column    = grade=\$[GRADE]"
        echo "  report_column    = eval=\$[TEST_EVAL_READY]"
        for line in "${REPORT_COLUMNS[@]}"; do
            echo "  report_column    = $line"
        done
        echo "  report_column    = mhash=\$[MHASH_SHORT]"
        echo "  report_file      = results.txt"
        echo "}"
    } > "$case_dir/case_eval.moos"
}

wait_for_result_line() {
    local results_path="$1"
    local attempts="${2:-80}"
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

check_alog_evidence() {
    local root="$1"
    local token
    local count

    if ! find "$root" -type f -name '*.alog' | grep -q .; then
        echo "missing_alog"
        return 1
    fi

    for token in "${REQUIRED_TOKENS[@]}"; do
        if ! find "$root" -type f -name '*.alog' -exec grep -Fq "$token" {} +; then
            echo "missing_token:$token"
            return 1
        fi
    done

    for token in "${ABSENT_TOKENS[@]}"; do
        if find "$root" -type f -name '*.alog' -exec grep -Fq "$token" {} +; then
            echo "unexpected_token:$token"
            return 1
        fi
    done

    if [ "$COUNT_VAR" != "" ]; then
        count=$(find "$root" -type f -name '*.alog' -exec grep -h " $COUNT_VAR " {} + | wc -l | tr -d ' ')
        if [ "$count" != "$COUNT_EXPECTED" ]; then
            echo "bad_count:$COUNT_VAR:$count"
            return 1
        fi
    fi

    echo "alog_ok"
    return 0
}


emit_case_row() {
    local case_name="$1"
    local status="$2"
    local expected="$3"
    local actual="$4"
    shift 4
    local line="$1"
    shift || true
    local grade

    grade=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    line=$(echo "$line" | sed 's/grade=[^, ]*[[:space:]]*//')

    if [ "$grade" != "" ]; then
        echo "case=$case_name  grade=$grade  $line  $*"
    elif [ "$status" = "success" ]; then
        echo "case=$case_name  grade=fail  reason=missing_result  $line  $*"
    else
        echo "case=$case_name  grade=fail  reason=$status  $line  $*"
    fi
}

run_case() {
    local case_name="$1"
    local case_idx="$2"
    local case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    local shore_mport=$((case_base + 0))
    local shore_pshare=$((case_base + 10))
    local case_dir="$RUN_ROOT/$case_name"
    local line=""
    local actual="fail"
    local evidence="not_checked"
    local launch_rc=0
    local status="success"

    get_case_config "$case_name" || return 1

    rm -rf "$case_dir"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR/." "$case_dir/"
    write_case_files "$case_dir"

    if [ "$JUST_MAKE" = "yes" ]; then
        (cd "$case_dir" && ./launch.sh --just_make --mmod="$case_name" --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" "$TIME_WARP")
        echo "case=$case_name  grade=pass  form=pechovar_unit  mmod=$case_name  done=true  reason=just_make"
        return 0
    fi

    (cd "$case_dir" && ./clean.sh >/dev/null 2>&1 && xlaunch.sh --max_time="$MAX_TIME" --mmod="$case_name" --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --nogui "$TIME_WARP" >/dev/null 2>&1)
    launch_rc=$?

    line=$(wait_for_result_line "$case_dir/results.txt")
    if echo "$line" | grep -q 'grade=pass'; then
        actual="pass"
    fi

    if [ "$actual" = "pass" ] && [ "$launch_rc" -eq 0 ]; then
        evidence=$(check_alog_evidence "$case_dir")
        if [ "$evidence" != "alog_ok" ]; then
            actual="fail"
        fi
    else
        evidence="mission_or_launch_failed"
    fi

    if [ "$actual" != "pass" ]; then
        status="mismatch"
    fi

    emit_case_row "$case_name" "$status" "pass" "$actual" "$line" "evidence=$evidence"

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

run_cases() {
    local cases=("$@")
    local idx=0
    local active=0
    local pids=()
    local names=()
    local pid
    local name
    local outfile
    local rc

    : > "$RESULTS_FILE"
    RUN_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/pechovar_harness.XXXXXX")
    CASE_ROW_DIR="$RUN_ROOT/case_rows"
    mkdir -p "$CASE_ROW_DIR"

    for name in "${cases[@]}"; do
        outfile="$CASE_ROW_DIR/$name.out"
        (run_case "$name" "$idx") > "$outfile" 2>&1 &
        pids+=("$!")
        names+=("$name")
        idx=$((idx + 1))
        active=$((active + 1))

        if [ "$active" -ge "$JOBS" ]; then
            for pid in "${pids[@]}"; do
                wait "$pid" || ALL_OK="no"
            done
            for name in "${names[@]}"; do
                cat "$CASE_ROW_DIR/$name.out" | tee -a "$RESULTS_FILE"
            done
            pids=()
            names=()
            active=0
        fi
    done

    for pid in "${pids[@]}"; do
        wait "$pid" || ALL_OK="no"
    done
    for name in "${names[@]}"; do
        cat "$CASE_ROW_DIR/$name.out" | tee -a "$RESULTS_FILE"
    done
}

if [ "$CASE" != "" ]; then
    if ! case_known "$CASE"; then
        echo "$ME: Unknown case: $CASE"
        exit 1
    fi
    SELECTED_CASES=("$CASE")
else
    SELECTED_CASES=("${ALL_CASES[@]}")
fi

run_cases "${SELECTED_CASES[@]}"
cleanup

if [ "$ALL_OK" = "yes" ]; then
    echo "$ME: all ${#SELECTED_CASES[@]} case(s) matched expected outcomes"
    exit 0
fi

echo "$ME: one or more cases failed"
exit 1
