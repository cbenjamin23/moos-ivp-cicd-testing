#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-utermcommand_unit
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
PORT_BASE="16200"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/utermcommand_missions/utermcommand_unit"
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
    numeric_command_pass
    quoted_string_command_pass
    unique_partial_command_pass
    duplicate_key_first_wins_pass
    multiple_commands_pass
    arrow_syntax_command_pass
    unquoted_string_command_pass
    negative_numeric_command_pass
    config_key_case_normalized_pass
    uppercase_input_absent_pass
    exact_over_ambiguous_pass
    ambiguous_partial_absent_pass
    delete_edit_command_pass
    invalid_command_config_ignored_pass
    unknown_command_absent_pass
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
    CMD_LINES=()
    INPUT_BYTES=""
    INPUT_EXTRA_BYTES=""
    PASS_CONDITIONS=()
    FAIL_CONDITIONS=()
    REPORT_COLUMNS=()
    CHECK_TOKENS=()

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    case "$CASE_NAME" in
        numeric_command_pass)
            CMD_LINES=("CMD = num,TERM_NUM,42")
            INPUT_BYTES=$'num\n'
            PASS_CONDITIONS=("TERM_NUM = 42")
            REPORT_COLUMNS=("term_num=\$[TERM_NUM]")
            CHECK_TOKENS=("term_num=42")
            ;;
        quoted_string_command_pass)
            CMD_LINES=("CMD = str,TERM_STR,\"alpha007\"")
            INPUT_BYTES=$'str\n'
            PASS_CONDITIONS=("TERM_STR = alpha007")
            REPORT_COLUMNS=("term_str=\$[TERM_STR]")
            CHECK_TOKENS=("term_str=alpha007")
            ;;
        unique_partial_command_pass)
            CMD_LINES=("CMD = panic,TERM_PARTIAL,true")
            INPUT_BYTES=$'pan\n'
            PASS_CONDITIONS=("TERM_PARTIAL = true")
            REPORT_COLUMNS=("term_partial=\$[TERM_PARTIAL]")
            CHECK_TOKENS=("term_partial=true")
            ;;
        duplicate_key_first_wins_pass)
            CMD_LINES=("CMD = dup,TERM_DUP,first" "CMD = dup,TERM_DUP,second")
            INPUT_BYTES=$'dup\n'
            PASS_CONDITIONS=("TERM_DUP = first")
            FAIL_CONDITIONS=("TERM_DUP = second")
            REPORT_COLUMNS=("term_dup=\$[TERM_DUP]")
            CHECK_TOKENS=("term_dup=first")
            ;;
        multiple_commands_pass)
            CMD_LINES=("CMD = aa,TERM_A,11" "CMD = bb,TERM_B,bravo")
            INPUT_BYTES=$'aa\n'
            INPUT_EXTRA_BYTES=$'bb\n'
            PASS_CONDITIONS=("TERM_A = 11" "TERM_B = bravo")
            REPORT_COLUMNS=("term_a=\$[TERM_A]" "term_b=\$[TERM_B]")
            CHECK_TOKENS=("term_a=11" "term_b=bravo")
            ;;
        arrow_syntax_command_pass)
            CMD_LINES=("CMD = arrow-->TERM_ARROW-->ok")
            INPUT_BYTES=$'arrow\n'
            PASS_CONDITIONS=("TERM_ARROW = ok")
            REPORT_COLUMNS=("term_arrow=\$[TERM_ARROW]")
            CHECK_TOKENS=("term_arrow=ok")
            ;;
        unquoted_string_command_pass)
            CMD_LINES=("CMD = raw,TERM_RAW,bravo")
            INPUT_BYTES=$'raw\n'
            PASS_CONDITIONS=("TERM_RAW = bravo")
            REPORT_COLUMNS=("term_raw=\$[TERM_RAW]")
            CHECK_TOKENS=("term_raw=bravo")
            ;;
        negative_numeric_command_pass)
            CMD_LINES=("CMD = neg,TERM_NEG,-3.5")
            INPUT_BYTES=$'neg\n'
            PASS_CONDITIONS=("TERM_NEG = -3.5")
            REPORT_COLUMNS=("term_neg=\$[TERM_NEG]")
            CHECK_TOKENS=("term_neg=-3.5")
            ;;
        config_key_case_normalized_pass)
            CMD_LINES=("CMD = MiXeD,TERM_MIXED,ok")
            INPUT_BYTES=$'mixed\n'
            PASS_CONDITIONS=("TERM_MIXED = ok")
            REPORT_COLUMNS=("term_mixed=\$[TERM_MIXED]")
            CHECK_TOKENS=("term_mixed=ok")
            ;;
        uppercase_input_absent_pass)
            CMD_LINES=("CMD = lower,TERM_UPPER,true")
            INPUT_BYTES=$'LOWER\n'
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            FAIL_CONDITIONS=("TERM_UPPER = true")
            REPORT_COLUMNS=("term_upper=\$[TERM_UPPER]")
            ;;
        exact_over_ambiguous_pass)
            CMD_LINES=("CMD = pan,TERM_EXACT,yes" "CMD = panic,TERM_LONG,no")
            INPUT_BYTES=$'pan\n'
            PASS_CONDITIONS=("TERM_EXACT = yes")
            FAIL_CONDITIONS=("TERM_LONG = no")
            REPORT_COLUMNS=("term_exact=\$[TERM_EXACT]" "term_long=\$[TERM_LONG]")
            CHECK_TOKENS=("term_exact=yes")
            ;;
        ambiguous_partial_absent_pass)
            CMD_LINES=("CMD = pan,TERM_AMBIG_A,true" "CMD = pat,TERM_AMBIG_B,true")
            INPUT_BYTES=$'pa\n'
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            FAIL_CONDITIONS=("TERM_AMBIG_A = true" "TERM_AMBIG_B = true")
            REPORT_COLUMNS=("term_ambig_a=\$[TERM_AMBIG_A]" "term_ambig_b=\$[TERM_AMBIG_B]")
            ;;
        delete_edit_command_pass)
            CMD_LINES=("CMD = raw,TERM_DELETE,edited")
            INPUT_BYTES=$'raww\177\n'
            PASS_CONDITIONS=("TERM_DELETE = edited")
            REPORT_COLUMNS=("term_delete=\$[TERM_DELETE]")
            CHECK_TOKENS=("term_delete=edited")
            ;;
        invalid_command_config_ignored_pass)
            CMD_LINES=("CMD = novar" "CMD = good,TERM_GOOD,ok")
            INPUT_BYTES=$'good\n'
            PASS_CONDITIONS=("TERM_GOOD = ok")
            REPORT_COLUMNS=("term_good=\$[TERM_GOOD]")
            CHECK_TOKENS=("term_good=ok")
            ;;
        unknown_command_absent_pass)
            CMD_LINES=("CMD = good,TERM_BAD,true")
            INPUT_BYTES=$'bogus\n'
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            FAIL_CONDITIONS=("TERM_BAD = true")
            REPORT_COLUMNS=("term_bad=\$[TERM_BAD]")
            ;;
    esac
}

write_case_files() {
    local case_dir="$1"
    local port="$2"
    local item
    {
        echo "ServerHost = localhost"
        echo "ServerPort = $port"
        echo "Community  = term"
        echo ""
        echo "ProcessConfig = uTermCommand"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        for item in "${CMD_LINES[@]}"; do
            echo "  $item"
        done
        echo "}"
    } > "$case_dir/termcommand.moos"

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
        echo "  mission_form = utermcommand_unit"
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

run_termcommand() {
    local case_dir="$1"
    (
        cd "$case_dir"
        (
            printf "%s" "$INPUT_BYTES"
            sleep 0.8
            printf "q"
        ) | uTermCommand termcommand.moos --verbose=false > termcommand.out 2>&1 &
        term_pid=$!
        deadline=$((SECONDS + 8))
        while kill -0 "$term_pid" 2>/dev/null; do
            if [ "$SECONDS" -ge "$deadline" ]; then
                kill "$term_pid" 2>/dev/null || true
                wait "$term_pid" 2>/dev/null || true
                exit 124
            fi
            sleep 0.1
        done
        wait "$term_pid"
        first_rc=$?
        if [ "$first_rc" != 0 ]; then
            exit "$first_rc"
        fi
        if [ "$INPUT_EXTRA_BYTES" != "" ]; then
            (
                printf "%s" "$INPUT_EXTRA_BYTES"
                sleep 0.8
                printf "q"
            ) | uTermCommand termcommand.moos --verbose=false >> termcommand.out 2>&1 &
            term_pid=$!
            deadline=$((SECONDS + 8))
            while kill -0 "$term_pid" 2>/dev/null; do
                if [ "$SECONDS" -ge "$deadline" ]; then
                    kill "$term_pid" 2>/dev/null || true
                    wait "$term_pid" 2>/dev/null || true
                    exit 124
                fi
                sleep 0.1
            done
            wait "$term_pid"
        fi
    )
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

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    local shore_mport=$((case_base + 0))
    local shore_pshare=$((case_base + 10))
    local tag
    local case_dir
    local case_row_file
    local line
    local actual
    local status="success"
    local term_rc=0
    local check

    get_case_config "$case_name" || return 1

    tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$tag"
    case_row_file="$CASE_ROW_DIR/$tag.txt"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/

    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )
    write_case_files "$case_dir" "$shore_mport"
    (
        cd "$case_dir"
        ./launch.sh --just_make --xlaunched --nogui --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --mmod="$case_name" "$TIME_WARP" >/dev/null
    )
    if [ "$JUST_MAKE" = "yes" ]; then
        echo "case=$case_name  grade=pass  reason=just_make" > "$case_row_file"
        return 0
    fi

    (
        cd "$case_dir"
        : > results.txt
        ./launch.sh --xlaunched --nogui --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --mmod="$case_name" "$TIME_WARP" > launch.out 2>&1
    )
    sleep 0.5
    run_termcommand "$case_dir"
    term_rc=$?
    uPokeDB --quiet --host=localhost --port="$shore_mport" TEST_EVAL_READY=true > "$case_dir/eval_ready.out" 2>&1 || true
    (
        cd "$case_dir"
        uMayFinish --max_time="$MAX_TIME" targ_shoreside.moos > mayfinish.out 2>&1 || true
    )
    line=$(wait_for_results "$case_dir/results.txt" 120)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    if [ "$term_rc" != 0 ] || [ "$actual" != "pass" ]; then
        status="mismatch"
    fi
    for check in "${CHECK_TOKENS[@]}"; do
        if ! echo "$line" | grep -q "$check"; then
            status="mismatch"
        fi
    done

    emit_case_row "$case_name" "$status" "pass" "$actual" "$line" "term_rc=$term_rc" > "$case_row_file"
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
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_utermcommand_unit_XXXXXX")
CASE_ROW_DIR="$RUN_ROOT/case_rows"
mkdir -p "$CASE_ROW_DIR"

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
        if [ -f "$CASE_ROW_DIR/$case_tag.txt" ]; then
            cat "$CASE_ROW_DIR/$case_tag.txt" >> "$RESULTS_FILE"
        else
            echo "case=${case_tag#*_}  grade=fail  reason=missing_result" >> "$RESULTS_FILE"
            ALL_OK="no"
        fi
    done

    remaining_cases="$next_remaining"
done

if grep -Eq '(^|[[:space:]])grade=fail([[:space:]]|$)' "$RESULTS_FILE"; then
    ALL_OK="no"
fi

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
