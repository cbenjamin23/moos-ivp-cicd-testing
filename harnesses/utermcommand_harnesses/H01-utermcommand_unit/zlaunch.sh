#!/usr/bin/env bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-utermcommand_unit
#   Author: Charles Benjamin
#   LastEd: Jul 2026
#------------------------------------------------------------

need_bash=5.1
if [ -z "${BASH_VERSION:-}" ]; then
    echo "zlaunch.sh: run this harness as ./zlaunch.sh with Bash >= $need_bash." >&2
    exit 2
fi

have_bash51() {
    (( BASH_VERSINFO[0] > 5 || (BASH_VERSINFO[0] == 5 && BASH_VERSINFO[1] >= 1) ))
}

if ! have_bash51; then
    if [ "${HARNESS_DISABLE_BASH_REEXEC:-}" != 1 ]; then
        for bash_candidate in "${HARNESS_BASH:-}" /opt/homebrew/bin/bash /usr/local/bin/bash /home/linuxbrew/.linuxbrew/bin/bash; do
            [ -n "$bash_candidate" ] && [ -x "$bash_candidate" ] || continue
            if "$bash_candidate" -c '(( BASH_VERSINFO[0] > 5 || (BASH_VERSINFO[0] == 5 && BASH_VERSINFO[1] >= 1) ))' 2>/dev/null; then
                echo "zlaunch.sh: re-running with $bash_candidate for Bash >= $need_bash" >&2
                exec "$bash_candidate" "$0" "$@"
            fi
        done
    fi
    echo "zlaunch.sh: Bash >= $need_bash is required for rolling --jobs scheduling." >&2
    echo "Detected Bash: $BASH_VERSION" >&2
    echo "On macOS, install Homebrew Bash or run: HARNESS_BASH=/opt/homebrew/bin/bash ./zlaunch.sh" >&2
    exit 2
fi

set -u

ME=$(basename "$0")
HARNESS_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_DIR=$(cd "$HARNESS_DIR/../../.." && pwd)
MISSION_DIR="$REPO_DIR/missions/utermcommand_missions/utermcommand_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/moos_scoped_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
RUNS_DIR="$HARNESS_DIR/.harness_runs"
LOCK_DIR="$HARNESS_DIR/.harness_runs.lock"
RUN_ROOT=""

TIME_WARP=10
MAX_TIME=180
CLIENT_START_TIMEOUT=20
TERMINAL_TIMEOUT=12
JOBS=1
PORT_BASE=9000
PORT_STRIDE=30
PSHARE_OFFSET=$((PORT_STRIDE / 2))
KEEP_WORKDIRS=no
VERBOSE=no
JUST_MAKE=no
LOG_MODE=minimal
DISPLAY_ARGS=(--nogui)
CASE=""
HAVE_LOCK=no
CLEANED=no
CLEANUP_FAILED=no
FINISH_FATAL_REASON=""

CASES=(
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

declare -a SELECTED_CASES CASE_RESULT
declare -A PID_CASE PID_RESULT PID_LOG PID_PORT_BASE

usage() {
    local case_name
    cat <<EOF
$ME [OPTIONS] [time_warp]

Options:
  --help, -h         Show this help message
  --verbose, -v      Show rolling scheduler events
  --just_make, -j    Prepare each case without launching it
  --log=<mode>       Logging mode: minimal (default) or full
  --max_time=<secs>  Maximum time passed to each stem mission
  --case=<name>      Run one named case
  --jobs=<n>         Run up to n cases concurrently with rolling scheduling
  --port_base=<n>    Base MOOS port for per-case blocks
  --keep_workdirs    Keep this invocation's isolated case directories
  --gui              Launch one explicit case with GUI support
  --nogui, -ng       Headless launch, no GUI (default)

Cases:
EOF
    for case_name in "${CASES[@]}"; do
        printf '  %s\n' "$case_name"
    done
    cat <<EOF

Examples:
  ./$ME
  ./$ME --log=full
  ./$ME --case=numeric_command_pass
  ./$ME --jobs=2 --port_base=22000
  ./$ME --just_make --case=ambiguous_partial_absent_pass
EOF
}

die() {
    echo "$ME: $*" >&2
    exit 2
}

is_uint() {
    [[ "$1" =~ ^[0-9]+$ ]]
}

for arg in "$@"; do
    case "$arg" in
        --case=*)
            CASE="${arg#--case=}"
            [ -n "$CASE" ] || die "--case requires a nonempty case name"
            ;;
        --jobs=*) JOBS="${arg#--jobs=}" ;;
        --port_base=*) PORT_BASE="${arg#--port_base=}" ;;
        --max_time=*) MAX_TIME="${arg#--max_time=}" ;;
        --log=*) LOG_MODE="${arg#--log=}" ;;
        --keep_workdirs) KEEP_WORKDIRS=yes ;;
        --verbose|-v) VERBOSE=yes ;;
        --just_make|-j) JUST_MAKE=yes ;;
        --gui) DISPLAY_ARGS=() ;;
        --nogui|-ng) DISPLAY_ARGS=(--nogui) ;;
        --help|-h) usage; exit 0 ;;
        *[!0-9]*|'') die "bad argument: $arg" ;;
        *) TIME_WARP="$arg" ;;
    esac
done

is_uint "$TIME_WARP" && [ "${#TIME_WARP}" -le 9 ] || die "time warp must be a bounded positive integer"
is_uint "$JOBS" && [ "${#JOBS}" -le 9 ] || die "--jobs must be a bounded positive integer"
is_uint "$PORT_BASE" && [ "${#PORT_BASE}" -le 5 ] || die "--port_base must be an integer from 1 through 65535"
is_uint "$MAX_TIME" && [ "${#MAX_TIME}" -le 9 ] || die "--max_time must be a bounded positive integer"
case "$LOG_MODE" in
    minimal|full) ;;
    *) die "--log must be minimal or full" ;;
esac

TIME_WARP=$((10#$TIME_WARP))
JOBS=$((10#$JOBS))
PORT_BASE=$((10#$PORT_BASE))
MAX_TIME=$((10#$MAX_TIME))

[ "$TIME_WARP" -gt 0 ] || die "time warp must be a positive integer"
[ "$JOBS" -gt 0 ] || die "--jobs must be a positive integer"
[ "$PORT_BASE" -gt 0 ] && [ "$PORT_BASE" -le 65535 ] || die "--port_base must be an integer from 1 through 65535"
[ "$MAX_TIME" -gt 0 ] || die "--max_time must be a positive integer"

[ -d "$MISSION_DIR" ] || die "mission directory not found: $MISSION_DIR"
[ -f "$TEARDOWN_HELPER" ] || die "missing teardown helper: $TEARDOWN_HELPER"
# shellcheck source=/dev/null
. "$TEARDOWN_HELPER"

select_cases() {
    local case_name
    SELECTED_CASES=()
    if [ -n "$CASE" ]; then
        for case_name in "${CASES[@]}"; do
            if [ "$case_name" = "$CASE" ]; then
                SELECTED_CASES=("$case_name")
                return 0
            fi
        done
        die "unknown case: $CASE"
    fi
    SELECTED_CASES=("${CASES[@]}")
    [ "${#SELECTED_CASES[@]}" -gt 0 ] || die "no cases selected"
}

field_value() {
    local line="$1"
    local key="$2"
    local field
    local -a fields=()
    read -r -a fields <<< "$line"
    for field in "${fields[@]}"; do
        case "$field" in
            "$key"=*) printf '%s\n' "${field#*=}"; return 0 ;;
        esac
    done
    return 1
}

field_count() {
    local line="$1"
    local key="$2"
    local field
    local count=0
    local -a fields=()
    read -r -a fields <<< "$line"
    for field in "${fields[@]}"; do
        case "$field" in
            "$key"=*) count=$((count + 1)) ;;
        esac
    done
    printf '%s\n' "$count"
}

without_case_field() {
    local line="$1"
    local field
    local output=""
    local -a fields=()
    read -r -a fields <<< "$line"
    for field in "${fields[@]}"; do
        case "$field" in
            case=*) ;;
            *) output="${output:+$output }$field" ;;
        esac
    done
    printf '%s\n' "$output"
}

runner_provenance() {
    local line="$1"
    local field
    local output=""
    local -a fields=()
    read -r -a fields <<< "$line"
    for field in "${fields[@]}"; do
        case "$field" in
            case=*) field="mission_case=${field#*=}" ;;
            grade=*) field="mission_grade=${field#*=}" ;;
            reason=*) field="mission_reason=${field#*=}" ;;
            launch_rc=*) field="mission_launch_rc=${field#*=}" ;;
        esac
        output="${output:+$output }$field"
    done
    printf '%s\n' "$output"
}

stop_root() {
    if ! moos_scoped_teardown_stop_root "$1"; then
        echo "$ME: scoped teardown failed for $1" >&2
        return 1
    fi
}

cleanup_runtime() {
    local pid
    local root_stopped=yes
    [ "$CLEANED" = no ] || return 0
    for pid in "${!PID_CASE[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    wait 2>/dev/null || true
    if [ -n "$RUN_ROOT" ] && [ -d "$RUN_ROOT" ]; then
        if ! stop_root "$RUN_ROOT"; then
            root_stopped=no
            CLEANUP_FAILED=yes
        fi
        if [ "$KEEP_WORKDIRS" != yes ] && [ "$root_stopped" = yes ] &&
           [ "$CLEANUP_FAILED" = no ]; then
            rm -rf "$RUN_ROOT"
        fi
    fi
    rmdir "$RUNS_DIR" 2>/dev/null || true
    if [ "$HAVE_LOCK" = yes ]; then
        if [ "$CLEANUP_FAILED" = yes ]; then
            echo "$ME: retaining safety lock after teardown failure: $LOCK_DIR" >&2
            echo "$ME: retained run root for manual recovery: $RUN_ROOT" >&2
        else
            rmdir "$LOCK_DIR" 2>/dev/null || true
            HAVE_LOCK=no
        fi
    fi
    CLEANED=yes
}

on_signal() {
    exit 130
}

trap cleanup_runtime EXIT
trap on_signal INT TERM PIPE

get_case_config() {
    CASE_NAME="$1"
    CMD_LINES=()
    INPUT_BYTES=""
    INPUT_EXTRA_BYTES=""
    PASS_CONDITIONS=()
    FAIL_CONDITIONS=()
    REPORT_COLUMNS=()

    case "$CASE_NAME" in
        numeric_command_pass)
            CMD_LINES=("CMD = num,TERM_NUM,42")
            INPUT_BYTES=$'num\n'
            PASS_CONDITIONS=("TERM_NUM = 42")
            REPORT_COLUMNS=("term_num=\$[TERM_NUM]")
            ;;
        quoted_string_command_pass)
            CMD_LINES=('CMD = str,TERM_STR,"alpha007"')
            INPUT_BYTES=$'str\n'
            PASS_CONDITIONS=("TERM_STR = alpha007")
            REPORT_COLUMNS=("term_str=\$[TERM_STR]")
            ;;
        unique_partial_command_pass)
            CMD_LINES=("CMD = panic,TERM_PARTIAL,true")
            INPUT_BYTES=$'pan\n'
            PASS_CONDITIONS=("TERM_PARTIAL = true")
            REPORT_COLUMNS=("term_partial=\$[TERM_PARTIAL]")
            ;;
        duplicate_key_first_wins_pass)
            CMD_LINES=("CMD = dup,TERM_DUP,first" "CMD = dup,TERM_DUP,second")
            INPUT_BYTES=$'dup\n'
            PASS_CONDITIONS=("TERM_DUP = first")
            FAIL_CONDITIONS=("TERM_DUP = second")
            REPORT_COLUMNS=("term_dup=\$[TERM_DUP]")
            ;;
        multiple_commands_pass)
            CMD_LINES=("CMD = aa,TERM_A,11" "CMD = bb,TERM_B,bravo")
            INPUT_BYTES=$'aa\n'
            INPUT_EXTRA_BYTES=$'bb\n'
            PASS_CONDITIONS=("TERM_A = 11" "TERM_B = bravo")
            REPORT_COLUMNS=("term_a=\$[TERM_A]" "term_b=\$[TERM_B]")
            ;;
        arrow_syntax_command_pass)
            CMD_LINES=("CMD = arrow-->TERM_ARROW-->ok")
            INPUT_BYTES=$'arrow\n'
            PASS_CONDITIONS=("TERM_ARROW = ok")
            REPORT_COLUMNS=("term_arrow=\$[TERM_ARROW]")
            ;;
        unquoted_string_command_pass)
            CMD_LINES=("CMD = raw,TERM_RAW,bravo")
            INPUT_BYTES=$'raw\n'
            PASS_CONDITIONS=("TERM_RAW = bravo")
            REPORT_COLUMNS=("term_raw=\$[TERM_RAW]")
            ;;
        negative_numeric_command_pass)
            CMD_LINES=("CMD = neg,TERM_NEG,-3.5")
            INPUT_BYTES=$'neg\n'
            PASS_CONDITIONS=("TERM_NEG = -3.5")
            REPORT_COLUMNS=("term_neg=\$[TERM_NEG]")
            ;;
        config_key_case_normalized_pass)
            CMD_LINES=("CMD = MiXeD,TERM_MIXED,ok")
            INPUT_BYTES=$'mixed\n'
            PASS_CONDITIONS=("TERM_MIXED = ok")
            REPORT_COLUMNS=("term_mixed=\$[TERM_MIXED]")
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
            ;;
        invalid_command_config_ignored_pass)
            CMD_LINES=("CMD = novar" "CMD = good,TERM_GOOD,ok")
            INPUT_BYTES=$'good\n'
            PASS_CONDITIONS=("TERM_GOOD = ok")
            REPORT_COLUMNS=("term_good=\$[TERM_GOOD]")
            ;;
        unknown_command_absent_pass)
            CMD_LINES=("CMD = good,TERM_BAD,true")
            INPUT_BYTES=$'bogus\n'
            PASS_CONDITIONS=("TEST_EVAL_READY = true")
            FAIL_CONDITIONS=("TERM_BAD = true")
            REPORT_COLUMNS=("term_bad=\$[TERM_BAD]")
            ;;
        *) return 1 ;;
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
        echo "  report_column = grade=\$[GRADE]"
        echo "  report_column = form=\$[MISSION_FORM]"
        echo "  report_column = mmod=\$[MMOD]"
        echo "  report_column = eval=\$[TEST_EVAL_READY]"
        for item in "${REPORT_COLUMNS[@]}"; do
            echo "  report_column = $item"
        done
        echo "  report_column = mhash=\$[MHASH_SHORT]"
        echo "  report_file   = results.txt"
        echo "}"
    } > "$case_dir/case_eval.moos"
}

run_terminal_session() {
    local case_dir="$1"
    local input_bytes="$2"
    local output_mode="$3"
    local term_pid
    local deadline
    local stop_attempt

    if [ "$output_mode" = append ]; then
        (
            printf '%s' "$input_bytes"
            sleep 0.8
            printf 'q'
        ) | uTermCommand "$case_dir/termcommand.moos" --verbose=false >> "$case_dir/termcommand.out" 2>&1 &
    else
        (
            printf '%s' "$input_bytes"
            sleep 0.8
            printf 'q'
        ) | uTermCommand "$case_dir/termcommand.moos" --verbose=false > "$case_dir/termcommand.out" 2>&1 &
    fi
    term_pid=$!
    deadline=$((SECONDS + TERMINAL_TIMEOUT))
    while kill -0 "$term_pid" 2>/dev/null; do
        if [ "$SECONDS" -ge "$deadline" ]; then
            kill -TERM "$term_pid" 2>/dev/null || true
            for ((stop_attempt = 0; stop_attempt < 20; stop_attempt++)); do
                kill -0 "$term_pid" 2>/dev/null || break
                sleep 0.1
            done
            if kill -0 "$term_pid" 2>/dev/null; then
                kill -KILL "$term_pid" 2>/dev/null || true
            fi
            wait "$term_pid" 2>/dev/null || true
            return 124
        fi
        sleep 0.1
    done
    wait "$term_pid"
}

run_termcommand() {
    local case_dir="$1"
    run_terminal_session "$case_dir" "$INPUT_BYTES" replace || return $?
    if [ -n "$INPUT_EXTRA_BYTES" ]; then
        run_terminal_session "$case_dir" "$INPUT_EXTRA_BYTES" append || return $?
    fi
}

run_ready_poke() {
    local port="$1"
    local output="$2"
    local ready_value="$3"
    local poke_pid
    local deadline=$((SECONDS + CLIENT_START_TIMEOUT))
    local stop_attempt

    uPokeDB --quiet --host=localhost --port="$port" \
        "TEST_EVAL_READY=$ready_value" > "$output" 2>&1 &
    poke_pid=$!
    while kill -0 "$poke_pid" 2>/dev/null; do
        if grep -Eq "TEST_EVAL_READY[[:space:]]+uPokeDB.*\"$ready_value\"" "$output" 2>/dev/null; then
            kill -TERM "$poke_pid" 2>/dev/null || true
            for ((stop_attempt = 0; stop_attempt < 20; stop_attempt++)); do
                kill -0 "$poke_pid" 2>/dev/null || break
                sleep 0.1
            done
            if kill -0 "$poke_pid" 2>/dev/null; then
                kill -KILL "$poke_pid" 2>/dev/null || true
            fi
            wait "$poke_pid" 2>/dev/null || true
            return 0
        fi
        if [ "$SECONDS" -ge "$deadline" ]; then
            kill -TERM "$poke_pid" 2>/dev/null || true
            for ((stop_attempt = 0; stop_attempt < 20; stop_attempt++)); do
                kill -0 "$poke_pid" 2>/dev/null || break
                sleep 0.1
            done
            if kill -0 "$poke_pid" 2>/dev/null; then
                kill -KILL "$poke_pid" 2>/dev/null || true
            fi
            wait "$poke_pid" 2>/dev/null || true
            return 124
        fi
        sleep 0.1
    done
    wait "$poke_pid"
}

prepare_case() {
    local case_name="$1"
    local workdir="$2"
    local port="$3"

    rm -rf "$workdir"
    mkdir -p "$workdir" || return 1
    cp -R "$MISSION_DIR"/. "$workdir"/ || return 1
    (
        cd "$workdir" || exit 1
        ./clean.sh >/dev/null 2>&1
    ) || return 1
    get_case_config "$case_name" || return 1
    write_case_files "$workdir" "$port"
}

write_result() {
    local case_name="$1"
    local result_file="$2"
    local launch_rc="$3"
    local stimulus_rc="$4"
    local workdir="$5"
    local line
    local grade_count
    local mission_grade
    local mission_rows
    local provenance

    if [ "$JUST_MAKE" = yes ]; then
        if [ "$launch_rc" -eq 0 ]; then
            printf 'case=%s grade=pass mode=just_make\n' "$case_name" > "$result_file"
        else
            printf 'case=%s grade=fail reason=launch_error launch_rc=%s mode=just_make\n' \
                "$case_name" "$launch_rc" > "$result_file"
        fi
        return 0
    fi

    if [ ! -f "$workdir/results.txt" ]; then
        printf 'case=%s grade=fail reason=missing_result_file launch_rc=%s stimulus_rc=%s\n' \
            "$case_name" "$launch_rc" "$stimulus_rc" > "$result_file"
        return 0
    fi

    mission_rows=$(awk 'NF {count++} END {print count+0}' "$workdir/results.txt")
    if [ "$mission_rows" -eq 0 ]; then
        printf 'case=%s grade=fail reason=missing_result launch_rc=%s stimulus_rc=%s\n' \
            "$case_name" "$launch_rc" "$stimulus_rc" > "$result_file"
        return 0
    fi
    if [ "$mission_rows" -ne 1 ]; then
        printf 'case=%s grade=fail reason=duplicate_results result_rows=%s\n' \
            "$case_name" "$mission_rows" > "$result_file"
        return 0
    fi

    line=$(awk 'NF {print; exit}' "$workdir/results.txt")
    grade_count=$(field_count "$line" grade)
    if [ "$grade_count" -eq 0 ]; then
        printf 'case=%s grade=fail reason=missing_result launch_rc=%s stimulus_rc=%s\n' \
            "$case_name" "$launch_rc" "$stimulus_rc" > "$result_file"
        return 0
    fi
    if [ "$grade_count" -ne 1 ]; then
        printf 'case=%s grade=fail reason=malformed_result grade_fields=%s\n' \
            "$case_name" "$grade_count" > "$result_file"
        return 0
    fi

    mission_grade=$(field_value "$line" grade || true)
    if [ "$mission_grade" != pass ] && [ "$mission_grade" != fail ]; then
        printf 'case=%s grade=fail reason=malformed_result mission_grade=%s\n' \
            "$case_name" "${mission_grade:-missing}" > "$result_file"
        return 0
    fi

    if [ "$stimulus_rc" -ne 0 ]; then
        provenance=$(runner_provenance "$line")
        printf 'case=%s grade=fail reason=stimulus_error stimulus_rc=%s%s\n' \
            "$case_name" "$stimulus_rc" "${provenance:+ $provenance}" > "$result_file"
        return 0
    fi

    if [ "$launch_rc" -ne 0 ]; then
        provenance=$(runner_provenance "$line")
        printf 'case=%s grade=fail reason=launch_error launch_rc=%s%s\n' \
            "$case_name" "$launch_rc" "${provenance:+ $provenance}" > "$result_file"
        return 0
    fi

    line=$(without_case_field "$line")
    printf 'case=%s %s\n' "$case_name" "$line" > "$result_file"
}

run_case() {
    local case_name="$1"
    local workdir="$2"
    local result_file="$3"
    local case_base="$4"
    local launch_rc=0
    local stimulus_rc=0
    local startup_rc=0
    local ready_rc=0
    local stem_pid=0
    local launch_args
    local result_line
    local grade

    prepare_case "$case_name" "$workdir" "$((case_base + 0))" || {
        printf 'case=%s grade=fail reason=prepare_error\n' "$case_name" > "$result_file"
        return 1
    }

    if [ "$JUST_MAKE" = yes ]; then
        (
            cd "$workdir" || exit 1
            launch_args=(
                --just_make
                --max_time="$MAX_TIME"
                "${DISPLAY_ARGS[@]}"
                --shore_mport="$((case_base + 0))"
                --shore_pshare="$((case_base + PSHARE_OFFSET))"
                "$TIME_WARP"
            )
            ./zlaunch.sh --log="$LOG_MODE" "${launch_args[@]}"
        ) || launch_rc=$?
    else
        (
            cd "$workdir" || exit 1
            : > results.txt
            launch_args=(
                --external_stimulus
                --max_time="$MAX_TIME"
                "${DISPLAY_ARGS[@]}"
                --shore_mport="$((case_base + 0))"
                --shore_pshare="$((case_base + PSHARE_OFFSET))"
                "$TIME_WARP"
            )
            ./zlaunch.sh --log="$LOG_MODE" "${launch_args[@]}"
        ) &
        stem_pid=$!
        run_ready_poke "$((case_base + 0))" "$workdir/eval_not_ready.out" false || startup_rc=$?
        if [ "$startup_rc" -eq 0 ]; then
            run_termcommand "$workdir" || stimulus_rc=$?
        else
            stimulus_rc=$startup_rc
        fi
        run_ready_poke "$((case_base + 0))" "$workdir/eval_ready.out" true || ready_rc=$?
        if [ "$stimulus_rc" -eq 0 ] && [ "$ready_rc" -ne 0 ]; then
            stimulus_rc=$ready_rc
        fi
        wait "$stem_pid" || launch_rc=$?
    fi

    write_result "$case_name" "$result_file" "$launch_rc" "$stimulus_rc" "$workdir"
    if ! stop_root "$workdir"; then
        printf 'case=%s grade=fail reason=teardown_error\n' "$case_name" > "$result_file"
    fi

    result_line=$(awk 'NF {print; exit}' "$result_file" 2>/dev/null)
    grade=$(field_value "$result_line" grade || true)
    [ "$grade" = pass ]
}

start_case() {
    local case_idx="$1"
    local case_name="${SELECTED_CASES[$case_idx]}"
    local case_dir
    local workdir
    local result_file
    local log_file
    local case_base
    local pid

    case_base=$((PORT_BASE + case_idx * PORT_STRIDE))
    case_dir="$RUN_ROOT/case_$(printf '%03d' "$case_idx")_$case_name"
    workdir="$case_dir/mission"
    result_file="$case_dir/result.row"
    log_file="$case_dir/run.log"

    mkdir -p "$case_dir"
    CASE_RESULT[case_idx]="$result_file"

    (
        local rc=0
        set +e
        run_case "$case_name" "$workdir" "$result_file" "$case_base" > "$log_file" 2>&1
        rc=$?
        if [ ! -s "$result_file" ]; then
            printf 'case=%s grade=fail reason=missing_result launch_rc=%s\n' \
                "$case_name" "$rc" > "$result_file"
        fi
        exit "$rc"
    ) &

    pid=$!
    PID_CASE[$pid]="$case_name"
    PID_RESULT[$pid]="$result_file"
    PID_LOG[$pid]="$log_file"
    PID_PORT_BASE[$pid]="$case_base"

    if [ "$VERBOSE" = yes ]; then
        printf 'event=start epoch=%s pid=%s case=%s port_base=%s workdir=%s\n' \
            "$(date +%s)" "$pid" "$case_name" "$case_base" "$workdir"
    fi
}

finish_one() {
    local done_pid=""
    local wait_rc=0
    local case_name
    local line
    local grade
    local reason

    FINISH_FATAL_REASON=""
    wait -p done_pid -n || wait_rc=$?
    if [ -z "${done_pid:-}" ]; then
        echo "$ME: wait returned without a completed pid rc=$wait_rc" >&2
        FINISH_FATAL_REASON=scheduler_state_error
        return 2
    fi

    case_name="${PID_CASE[$done_pid]:-}"
    if [ -z "$case_name" ]; then
        echo "$ME: unknown completed pid $done_pid rc=$wait_rc" >&2
        FINISH_FATAL_REASON=scheduler_state_error
        return 2
    fi

    line=$(awk 'NF {print; exit}' "${PID_RESULT[$done_pid]}" 2>/dev/null)
    grade=$(field_value "$line" grade || true)
    reason=$(field_value "$line" reason || true)
    if [ "$wait_rc" -ne 0 ] && [ "$grade" = pass ]; then
        printf 'case=%s grade=fail reason=worker_error worker_rc=%s\n' \
            "$case_name" "$wait_rc" > "${PID_RESULT[$done_pid]}"
        grade=fail
    fi
    if [ "$VERBOSE" = yes ]; then
        printf 'event=finish epoch=%s pid=%s case=%s rc=%s grade=%s port_base=%s log=%s\n' \
            "$(date +%s)" "$done_pid" "$case_name" "$wait_rc" "${grade:-missing}" \
            "${PID_PORT_BASE[$done_pid]}" "${PID_LOG[$done_pid]}"
    fi

    unset 'PID_CASE[$done_pid]' 'PID_RESULT[$done_pid]' 'PID_LOG[$done_pid]'
    unset 'PID_PORT_BASE[$done_pid]'
    if [ "$reason" = teardown_error ]; then
        FINISH_FATAL_REASON=teardown_error
        return 2
    fi
    [ "$grade" = pass ]
}

stop_refill_after_infrastructure_error() {
    local next_idx="$1"
    local total="$2"
    local fatal_reason="$3"
    local case_idx
    local case_name
    local case_dir
    local result_file

    CLEANUP_FAILED=yes
    echo "$ME: stopping scheduler refill after $fatal_reason" >&2

    for ((case_idx = next_idx; case_idx < total; case_idx++)); do
        case_name="${SELECTED_CASES[$case_idx]}"
        case_dir="$RUN_ROOT/case_$(printf '%03d' "$case_idx")_$case_name"
        result_file="$case_dir/result.row"
        CASE_RESULT[case_idx]="$result_file"
        if mkdir -p "$case_dir"; then
            printf 'case=%s grade=fail reason=scheduler_aborted_after_%s\n' \
                "$case_name" "$fatal_reason" > "$result_file"
        else
            echo "$ME: unable to record aborted case: $case_name" >&2
        fi
    done
}

aggregate_results() {
    local total="${#SELECTED_CASES[@]}"
    local case_idx
    local case_name
    local result_file
    local row_count
    local line
    local row_case
    local grade_count
    local grade

    FINAL_FAILURES=0
    RESULT_ROWS=0
    : > "$RESULTS_FILE"

    for ((case_idx = 0; case_idx < total; case_idx++)); do
        case_name="${SELECTED_CASES[$case_idx]}"
        result_file="${CASE_RESULT[$case_idx]:-}"
        line=""

        if [ -n "$result_file" ] && [ -f "$result_file" ]; then
            row_count=$(awk 'NF {count++} END {print count+0}' "$result_file")
            if [ "$row_count" -eq 1 ]; then
                line=$(awk 'NF {print; exit}' "$result_file")
            else
                line="case=$case_name grade=fail reason=invalid_row_count result_rows=$row_count"
            fi
        else
            line="case=$case_name grade=fail reason=missing_result_file"
        fi

        row_case=$(field_value "$line" case || true)
        grade_count=$(field_count "$line" grade)
        if [ "$row_case" != "$case_name" ]; then
            line="case=$case_name grade=fail reason=result_case_mismatch"
        elif [ "$grade_count" -ne 1 ]; then
            line="case=$case_name grade=fail reason=malformed_result grade_fields=$grade_count"
        else
            grade=$(field_value "$line" grade || true)
            if [ "$grade" != pass ] && [ "$grade" != fail ]; then
                line="case=$case_name grade=fail reason=malformed_result mission_grade=${grade:-missing}"
            fi
        fi

        printf '%s\n' "$line" >> "$RESULTS_FILE"
        RESULT_ROWS=$((RESULT_ROWS + 1))
        grade=$(field_value "$line" grade || true)
        [ "$grade" = pass ] || FINAL_FAILURES=$((FINAL_FAILURES + 1))
    done
}

select_cases
total=${#SELECTED_CASES[@]}
if [ "${#DISPLAY_ARGS[@]}" -eq 0 ] && [ "$total" -ne 1 ]; then
    die "--gui requires one explicit --case"
fi
last_port=$((PORT_BASE + (total - 1) * PORT_STRIDE + PSHARE_OFFSET))
[ "$last_port" -le 65535 ] || die "selected cases require ports through $last_port"

mkdir -p "$RUNS_DIR" || die "unable to create run directory: $RUNS_DIR"
mkdir "$LOCK_DIR" 2>/dev/null || die "another harness run appears active for $HARNESS_DIR"
HAVE_LOCK=yes
RUN_ROOT="$RUNS_DIR/run_$(date +%Y%m%dT%H%M%S)_$$"
mkdir -p "$RUN_ROOT" || die "unable to create run root: $RUN_ROOT"
: > "$RESULTS_FILE"

SECONDS=0
active=0
next=0
scheduler_broken=no

while [ "$next" -lt "$total" ] || [ "$active" -gt 0 ]; do
    while [ "$next" -lt "$total" ] && [ "$active" -lt "$JOBS" ]; do
        start_case "$next"
        next=$((next + 1))
        active=$((active + 1))
    done
    if [ "$active" -gt 0 ]; then
        finish_rc=0
        finish_one || finish_rc=$?
        if [ "$finish_rc" -eq 2 ] && [ "$FINISH_FATAL_REASON" = scheduler_state_error ]; then
            stop_refill_after_infrastructure_error "$next" "$total" "$FINISH_FATAL_REASON"
            next=$total
            scheduler_broken=yes
            break
        fi
        active=$((active - 1))
        if [ "$finish_rc" -eq 2 ]; then
            stop_refill_after_infrastructure_error "$next" "$total" "$FINISH_FATAL_REASON"
            next=$total
        fi
    fi
done

if [ "$scheduler_broken" = yes ]; then
    cleanup_runtime
fi

aggregate_results

if [ "$RESULT_ROWS" -ne "$total" ] || { [ "$total" -gt 0 ] && [ "$RESULT_ROWS" -eq 0 ]; }; then
    echo "$ME: expected $total result rows but wrote $RESULT_ROWS" >&2
    FINAL_FAILURES=$((FINAL_FAILURES + 1))
fi

cleanup_runtime
elapsed_seconds=$SECONDS
if [ -d "$RUN_ROOT" ]; then
    kept_root="$RUN_ROOT"
else
    kept_root="none"
fi
if [ "$CLEANUP_FAILED" = yes ]; then
    FINAL_FAILURES=$((FINAL_FAILURES + 1))
fi
trap - EXIT INT TERM PIPE

printf 'results=%s failures=%s total=%s jobs=%s log_mode=%s elapsed_seconds=%s bash=%s workdirs=%s\n' \
    "$RESULTS_FILE" "$FINAL_FAILURES" "$total" "$JOBS" "$LOG_MODE" "$elapsed_seconds" "$BASH_VERSION" "$kept_root"

[ "$FINAL_FAILURES" -eq 0 ]
