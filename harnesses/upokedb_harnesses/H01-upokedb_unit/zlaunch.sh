#!/usr/bin/env bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-upokedb_unit
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
MISSION_DIR="$REPO_DIR/missions/upokedb_missions/upokedb_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/moos_scoped_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
RUNS_DIR="$HARNESS_DIR/.harness_runs"
LOCK_DIR="$HARNESS_DIR/.harness_runs.lock"
RUN_ROOT=""

TIME_WARP=10
MAX_TIME=30
JOBS=1
PORT_BASE=9000
PORT_STRIDE=30
PSHARE_OFFSET=$((PORT_STRIDE / 2))
KEEP_WORKDIRS=no
VERBOSE=no
JUST_MAKE=no
DISPLAY_ARGS=(--nogui)
CASE=""
HAVE_LOCK=no
CLEANED=no
CLEANUP_FAILED=no
FINISH_FATAL_REASON=""

CASES=(
    numeric_direct_pass
    string_forced_pass
    multiple_mixed_pass
    overwrite_existing_pass
    cache_config_pass
    mission_file_connection_pass
    quiet_mode_pass
    negative_double_pass
    boolean_true_pass
    repeated_batch_pass
    mixed_numeric_string_batch_pass
    cache_string_numeric_pass
    host_alias_args_pass
    server_alias_args_pass
    server_underscore_args_pass
    quiet_short_alias_pass
    cache_short_alias_pass
    zero_double_pass
    scientific_literal_pass
    large_double_pass
    cache_forced_string_pass
    cache_boolean_pass
    utc_macro_pass
    moostime_macro_pass
    forced_utc_string_pass
    duplicate_same_invocation_pass
    cache_duplicate_first_wins_pass
    cache_cli_mixed_pass
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
  --max_time=<secs>  Maximum time passed to each stem mission
  --case=<name>      Run one named case
  --jobs=<n>         Run up to n cases concurrently with rolling scheduling
  --port_base=<n>    Base MOOS port for per-case blocks
  --keep_workdirs    Keep this invocation's isolated case directories
  --gui              Accepted for wrapper parity
  --nogui, -ng       Headless launch, no gui (default)

Cases:
EOF
    for case_name in "${CASES[@]}"; do
        printf '  %s\n' "$case_name"
    done
    cat <<EOF

Examples:
  ./$ME
  ./$ME --case=numeric_direct_pass
  ./$ME --jobs=2 --port_base=22000
  ./$ME --just_make --case=cache_config_pass
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
    POKE_MODE="host"
    POKE_BATCHES=()
    PASS_CONDITIONS=()
    REPORT_COLUMNS=()
    CACHE_POKES=()

    case "$CASE_NAME" in
        numeric_direct_pass)
            POKE_BATCHES=("POKE_NUM=42.5 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_NUM = 42.5")
            REPORT_COLUMNS=("poke_num=\$[POKE_NUM]")
            ;;
        string_forced_pass)
            POKE_BATCHES=("POKE_STR:=0012 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_STR = 0012")
            REPORT_COLUMNS=("poke_str=\$[POKE_STR]")
            ;;
        multiple_mixed_pass)
            POKE_BATCHES=("POKE_A=7 POKE_B:=bravo TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_A = 7" "POKE_B = bravo")
            REPORT_COLUMNS=("poke_a=\$[POKE_A]" "poke_b=\$[POKE_B]")
            ;;
        overwrite_existing_pass)
            POKE_BATCHES=("POKE_OVER:=old" "__SLEEP__=0.5" "POKE_OVER:=new TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_OVER = new")
            REPORT_COLUMNS=("poke_over=\$[POKE_OVER]")
            ;;
        cache_config_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_NUM=88" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_NUM = 88")
            REPORT_COLUMNS=("cache_num=\$[POKE_CACHE_NUM]")
            ;;
        mission_file_connection_pass)
            POKE_MODE="file"
            POKE_BATCHES=("POKE_FILE:=from_file TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_FILE = from_file")
            REPORT_COLUMNS=("poke_file=\$[POKE_FILE]")
            ;;
        quiet_mode_pass)
            POKE_BATCHES=("POKE_QUIET:=silent TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_QUIET = silent")
            REPORT_COLUMNS=("poke_quiet=\$[POKE_QUIET]")
            ;;
        negative_double_pass)
            POKE_BATCHES=("POKE_NEG=-13.25 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_NEG = -13.25")
            REPORT_COLUMNS=("poke_neg=\$[POKE_NEG]")
            ;;
        boolean_true_pass)
            POKE_BATCHES=("POKE_BOOL=true TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_BOOL = true")
            REPORT_COLUMNS=("poke_bool=\$[POKE_BOOL]")
            ;;
        repeated_batch_pass)
            POKE_BATCHES=("POKE_REPEAT:=first" "__SLEEP__=0.5" "POKE_REPEAT:=second TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_REPEAT = second")
            REPORT_COLUMNS=("poke_repeat=\$[POKE_REPEAT]")
            ;;
        mixed_numeric_string_batch_pass)
            POKE_BATCHES=("POKE_MIXED_NUM=31 POKE_MIXED_STR:=031 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_MIXED_NUM = 31" "POKE_MIXED_STR = 031")
            REPORT_COLUMNS=("mixed_num=\$[POKE_MIXED_NUM]" "mixed_str=\$[POKE_MIXED_STR]")
            ;;
        cache_string_numeric_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_NUM=77" "POKE_CACHE_STR:=seventy_seven" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_NUM = 77" "POKE_CACHE_STR = seventy_seven")
            REPORT_COLUMNS=("cache_num=\$[POKE_CACHE_NUM]" "cache_str=\$[POKE_CACHE_STR]")
            ;;
        host_alias_args_pass)
            POKE_MODE="host_alias"
            POKE_BATCHES=("POKE_ALIAS:=alias_host TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_ALIAS = alias_host")
            REPORT_COLUMNS=("poke_alias=\$[POKE_ALIAS]")
            ;;
        server_alias_args_pass)
            POKE_MODE="server_alias"
            POKE_BATCHES=("POKE_SERVER_ALIAS:=server_alias TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_SERVER_ALIAS = server_alias")
            REPORT_COLUMNS=("server_alias=\$[POKE_SERVER_ALIAS]")
            ;;
        server_underscore_args_pass)
            POKE_MODE="server_underscore"
            POKE_BATCHES=("POKE_SERVER_UNDERSCORE:=server_under TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_SERVER_UNDERSCORE = server_under")
            REPORT_COLUMNS=("server_under=\$[POKE_SERVER_UNDERSCORE]")
            ;;
        quiet_short_alias_pass)
            POKE_MODE="quiet_short"
            POKE_BATCHES=("POKE_QSHORT:=quiet_short TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_QSHORT = quiet_short")
            REPORT_COLUMNS=("qshort=\$[POKE_QSHORT]")
            ;;
        cache_short_alias_pass)
            POKE_MODE="cache_short"
            CACHE_POKES=("POKE_CACHE_SHORT:=cache_short" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_SHORT = cache_short")
            REPORT_COLUMNS=("cache_short=\$[POKE_CACHE_SHORT]")
            ;;
        zero_double_pass)
            POKE_BATCHES=("POKE_ZERO=0 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_ZERO = 0")
            REPORT_COLUMNS=("poke_zero=\$[POKE_ZERO]")
            ;;
        scientific_literal_pass)
            POKE_BATCHES=("POKE_SCI=1e-3 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_SCI = 1e-3")
            REPORT_COLUMNS=("poke_sci=\$[POKE_SCI]")
            ;;
        large_double_pass)
            POKE_BATCHES=("POKE_BIG=123456.75 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_BIG = 123456.75")
            REPORT_COLUMNS=("poke_big=\$[POKE_BIG]")
            ;;
        cache_forced_string_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_FORCED:=0007" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_FORCED = 0007")
            REPORT_COLUMNS=("cache_forced=\$[POKE_CACHE_FORCED]")
            ;;
        cache_boolean_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_BOOL=true" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_BOOL = true")
            REPORT_COLUMNS=("cache_bool=\$[POKE_CACHE_BOOL]")
            ;;
        utc_macro_pass)
            POKE_BATCHES=("POKE_UTC=@UTC TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_UTC > 0")
            REPORT_COLUMNS=("poke_utc=\$[POKE_UTC]")
            ;;
        moostime_macro_pass)
            POKE_BATCHES=("POKE_MOOSTIME=@MOOSTIME TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_MOOSTIME > 0")
            REPORT_COLUMNS=("poke_moostime=\$[POKE_MOOSTIME]")
            ;;
        forced_utc_string_pass)
            POKE_BATCHES=("POKE_UTC_STRING:=@UTC TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_UTC_STRING = @UTC")
            REPORT_COLUMNS=("utc_string=\$[POKE_UTC_STRING]")
            ;;
        duplicate_same_invocation_pass)
            POKE_BATCHES=("POKE_DUP:=first POKE_DUP:=second TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_DUP = second")
            REPORT_COLUMNS=("poke_dup=\$[POKE_DUP]")
            ;;
        cache_duplicate_first_wins_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_DUP:=first" "POKE_CACHE_DUP:=second" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_DUP = first")
            REPORT_COLUMNS=("cache_dup=\$[POKE_CACHE_DUP]")
            ;;
        cache_cli_mixed_pass)
            POKE_MODE="cache_cli"
            CACHE_POKES=("POKE_CACHE_MIX:=from_cache" "TEST_EVAL_READY=true")
            POKE_BATCHES=("POKE_CLI_MIX:=from_cli")
            PASS_CONDITIONS=("POKE_CACHE_MIX = from_cache" "POKE_CLI_MIX = from_cli")
            REPORT_COLUMNS=("cache_mix=\$[POKE_CACHE_MIX]" "cli_mix=\$[POKE_CLI_MIX]")
            ;;
    esac
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
        echo "  mission_form = upokedb_unit"
        echo "  mission_mod  = $CASE_NAME"
        echo ""
        echo "  prereport_column = form=\$[MISSION_FORM]"
        echo "  prereport_column = mmod=\$[MMOD]"
        echo "  report_column    = grade=\$[GRADE]"
        echo "  report_column    = eval=\$[TEST_EVAL_READY]"
        for col in "${REPORT_COLUMNS[@]}"; do
            echo "  report_column    = $col"
        done
        echo "  report_column    = mhash=\$[MHASH_SHORT]"
        echo "  report_file      = results.txt"
        echo "}"
    } > "$eval_file"
}

write_cache_config() {
    local case_dir="$1"
    local port="$2"
    local cache_file="$case_dir/poke_cache.moos"
    {
        echo "ServerHost = localhost"
        echo "ServerPort = $port"
        echo "Community  = cache"
        echo ""
        echo "ProcessConfig = uPokeDB"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        for poke in "${CACHE_POKES[@]}"; do
            echo "  poke = $poke"
        done
        echo "}"
    } > "$cache_file"
}

run_bounded_command() {
    local command_pid
    local watchdog_pid
    local command_rc=0

    "$@" &
    command_pid=$!
    (
        sleep "$MAX_TIME"
        kill -TERM "$command_pid" 2>/dev/null || true
    ) &
    watchdog_pid=$!

    wait "$command_pid" || command_rc=$?
    kill "$watchdog_pid" 2>/dev/null || true
    wait "$watchdog_pid" 2>/dev/null || true
    return "$command_rc"
}

run_pokes() {
    local case_dir="$1"
    local port="$2"
    local batch
    local -a batch_args=()

    if [ "$POKE_MODE" = "cache" ] || [ "$POKE_MODE" = "cache_short" ] || [ "$POKE_MODE" = "cache_cli" ]; then
        write_cache_config "$case_dir" "$port"
        if [ "$POKE_MODE" = "cache_short" ]; then
            run_bounded_command uPokeDB "$case_dir/poke_cache.moos" -c -q > "$case_dir/upokedb.out" 2>&1
        elif [ "$POKE_MODE" = "cache_cli" ]; then
            run_bounded_command uPokeDB "$case_dir/poke_cache.moos" --cache --quiet "${POKE_BATCHES[@]}" > "$case_dir/upokedb.out" 2>&1
        else
            run_bounded_command uPokeDB "$case_dir/poke_cache.moos" --cache --quiet > "$case_dir/upokedb.out" 2>&1
        fi
        return $?
    fi

    for batch in "${POKE_BATCHES[@]}"; do
        if echo "$batch" | grep -q '^__SLEEP__='; then
            sleep "${batch#__SLEEP__=}"
            continue
        fi
        read -r -a batch_args <<< "$batch"
        if [ "$POKE_MODE" = "file" ]; then
            run_bounded_command uPokeDB "$case_dir/targ_shoreside.moos" --quiet "${batch_args[@]}" >> "$case_dir/upokedb.out" 2>&1 || return 1
        elif [ "$POKE_MODE" = "host_alias" ]; then
            run_bounded_command uPokeDB --quiet host=localhost port="$port" "${batch_args[@]}" >> "$case_dir/upokedb.out" 2>&1 || return 1
        elif [ "$POKE_MODE" = "server_alias" ]; then
            run_bounded_command uPokeDB --quiet serverhost=localhost serverport="$port" "${batch_args[@]}" >> "$case_dir/upokedb.out" 2>&1 || return 1
        elif [ "$POKE_MODE" = "server_underscore" ]; then
            run_bounded_command uPokeDB --quiet server_host=localhost server_port="$port" "${batch_args[@]}" >> "$case_dir/upokedb.out" 2>&1 || return 1
        elif [ "$POKE_MODE" = "quiet_short" ]; then
            run_bounded_command uPokeDB -q --host=localhost --port="$port" "${batch_args[@]}" >> "$case_dir/upokedb.out" 2>&1 || return 1
        else
            run_bounded_command uPokeDB --quiet --host=localhost --port="$port" "${batch_args[@]}" >> "$case_dir/upokedb.out" 2>&1 || return 1
        fi
    done
}


prepare_case() {
    local case_name="$1"
    local workdir="$2"

    rm -rf "$workdir"
    mkdir -p "$workdir" || return 1
    cp -R "$MISSION_DIR"/. "$workdir"/ || return 1
    (
        cd "$workdir" || exit 1
        ./clean.sh >/dev/null 2>&1
    ) || return 1
    get_case_config "$case_name" || return 1
    write_eval_config "$workdir"
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
    local stem_pid=0
    local launch_args
    local result_line
    local grade

    prepare_case "$case_name" "$workdir" || {
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
                --mmod="$case_name"
                "$TIME_WARP"
            )
            ./zlaunch.sh "${launch_args[@]}"
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
                --mmod="$case_name"
                "$TIME_WARP"
            )
            ./zlaunch.sh "${launch_args[@]}"
        ) &
        stem_pid=$!
        sleep 0.75
        run_pokes "$workdir" "$((case_base + 0))" || stimulus_rc=$?
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

printf 'results=%s failures=%s total=%s jobs=%s elapsed_seconds=%s bash=%s workdirs=%s\n' \
    "$RESULTS_FILE" "$FINAL_FAILURES" "$total" "$JOBS" "$elapsed_seconds" "$BASH_VERSION" "$kept_root"

[ "$FINAL_FAILURES" -eq 0 ]
