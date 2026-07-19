#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: ufield_app_unit
#   Author: Charles Benjamin
#   LastEd: Jul 2026
#------------------------------------------------------------

set -u

ME=$(basename "$0")
TIME_WARP=10
MAX_TIME=80
VERBOSE=no
JUST_MAKE=no
LOG_MODE=minimal
DISPLAY_ARGS=(--nogui)
FLOW_ARGS=()
SHORE_MPORT=""
VEH_MPORT=""
SHORE_PSHARE=""
VEH_PSHARE=""

usage() {
    cat <<EOF
$ME [OPTIONS] [time_warp]

Options:
  --help, -h           Show this help message
  --verbose, -v        Verbose launch output
  --just_make, -j      Only create targ files
  --log=minimal|full   Select logging mode (default: minimal)
  --nogui, -ng         Headless launch, no gui (default)
  --max_time=<secs>    Maximum time passed to xlaunch
  --mmod=<mod>         Mission variation/modifier
  --shore_mport=<n>    Shoreside MOOSDB port
  --veh_mport=<n>      Accepted for single-community parity
  --shore_pshare=<n>   Shoreside pShare port
  --veh_pshare=<n>     Accepted for single-community parity
EOF
}

die() {
    echo "$ME: $*" >&2
    exit 2
}

is_uint() {
    case "$1" in
        ''|*[!0-9]*) return 1 ;;
        *) return 0 ;;
    esac
}

validate_port() {
    local value="$1"
    local option="$2"
    local numeric

    is_uint "$value" && [ "${#value}" -le 5 ] || \
        die "$option must be an integer from 1 through 65535"
    numeric=$((10#$value))
    [ "$numeric" -gt 0 ] && [ "$numeric" -le 65535 ] || \
        die "$option must be an integer from 1 through 65535"
}

for arg in "$@"; do
    case "$arg" in
        --help|-h) usage; exit 0 ;;
        --verbose|-v) VERBOSE=yes; FLOW_ARGS+=(--verbose) ;;
        --just_make|-j) JUST_MAKE=yes ;;
        --log=*) LOG_MODE="${arg#--log=}" ;;
        --nogui|-ng) DISPLAY_ARGS=(--nogui) ;;
        --max_time=*) MAX_TIME="${arg#--max_time=}" ;;
        --mmod=*) FLOW_ARGS+=("$arg") ;;
        --shore_mport=*)
            SHORE_MPORT="${arg#--shore_mport=}"
            validate_port "$SHORE_MPORT" --shore_mport
            FLOW_ARGS+=("$arg")
            ;;
        --veh_mport=*)
            VEH_MPORT="${arg#--veh_mport=}"
            validate_port "$VEH_MPORT" --veh_mport
            FLOW_ARGS+=("$arg")
            ;;
        --shore_pshare=*)
            SHORE_PSHARE="${arg#--shore_pshare=}"
            validate_port "$SHORE_PSHARE" --shore_pshare
            FLOW_ARGS+=("$arg")
            ;;
        --veh_pshare=*)
            VEH_PSHARE="${arg#--veh_pshare=}"
            validate_port "$VEH_PSHARE" --veh_pshare
            FLOW_ARGS+=("$arg")
            ;;
        *)
            is_uint "$arg" || die "bad argument: $arg"
            TIME_WARP="$arg"
            ;;
    esac
done

case "$LOG_MODE" in
    minimal|full) ;;
    *) die "--log must be minimal or full" ;;
esac

is_uint "$TIME_WARP" && [ "${#TIME_WARP}" -le 9 ] || \
    die "time warp must be a bounded positive integer"
is_uint "$MAX_TIME" && [ "${#MAX_TIME}" -le 9 ] || \
    die "--max_time must be a bounded positive integer"
TIME_WARP=$((10#$TIME_WARP))
MAX_TIME=$((10#$MAX_TIME))
[ "$TIME_WARP" -gt 0 ] || die "time warp must be a positive integer"
[ "$MAX_TIME" -gt 0 ] || die "--max_time must be a positive integer"

REPO_DIR=$(git -C "$PWD" rev-parse --show-toplevel 2>/dev/null) || \
    die "unable to locate repository root from $PWD"
TEARDOWN_HELPER="$REPO_DIR/scripts/moos_scoped_teardown.sh"
[ -f "$TEARDOWN_HELPER" ] || die "missing scoped teardown helper: $TEARDOWN_HELPER"
# shellcheck source=/dev/null
. "$TEARDOWN_HELPER"

# shellcheck disable=SC2329  # Reached from the EXIT-trap handler.
stop_here() {
    if ! moos_scoped_teardown_stop_root "$PWD"; then
        echo "$ME: scoped teardown failed for $PWD" >&2
        return 1
    fi
}

# shellcheck disable=SC2329  # Invoked through the EXIT trap.
on_exit() {
    local exit_rc=$?
    trap - EXIT
    if ! stop_here && [ "$exit_rc" -eq 0 ]; then
        exit_rc=1
    fi
    exit "$exit_rc"
}

# shellcheck disable=SC2329  # Invoked through the signal traps.
on_signal() {
    exit 130
}

trap on_exit EXIT
trap on_signal INT TERM

if [ "${LOG_MODE_PREPARED:-no}" != yes ]; then
    ./prepare_logging_mode.sh "$LOG_MODE" || die "unable to prepare --log=$LOG_MODE"
fi
export LOG_MODE_PREPARED=yes
export LOG_MODE_PREPARED_VALUE="$LOG_MODE"

: > results.txt
[ "$VERBOSE" = yes ] && echo "$ME: launching with max_time=$MAX_TIME warp=$TIME_WARP"

if [ "$JUST_MAKE" = yes ]; then
    rm -f targ_shoreside.moos
fi

launch_args=(--max_time="$MAX_TIME" "${DISPLAY_ARGS[@]}" "${FLOW_ARGS[@]}" "$TIME_WARP")
[ "$JUST_MAKE" = yes ] && launch_args+=(--just_make)

launch_rc=0
xlaunch.sh "${launch_args[@]}" || launch_rc=$?

if [ "$JUST_MAKE" = yes ]; then
    if [ "$launch_rc" -ne 0 ] || [ ! -s targ_shoreside.moos ]; then
        echo "$ME: target generation did not produce targ_shoreside.moos" >&2
        exit 1
    fi
    exit 0
fi

result_rows=$(awk 'NF {count++} END {print count+0}' results.txt 2>/dev/null)
[ "$result_rows" -eq 1 ] || {
    echo "$ME: expected exactly one pMissionEval result row; found $result_rows" >&2
    exit 1
}

result_line=$(awk 'NF {print; exit}' results.txt 2>/dev/null)
result_grade=""
grade_count=0
result_fields=()
if [ -n "$result_line" ]; then
    read -r -a result_fields <<< "$result_line"
    for field in "${result_fields[@]}"; do
        case "$field" in
            grade=*)
                grade_count=$((grade_count + 1))
                result_grade="${field#grade=}"
                ;;
        esac
    done
fi

if [ "$grade_count" -ne 1 ] ||
   { [ "$result_grade" != pass ] && [ "$result_grade" != fail ]; }; then
    echo "$ME: pMissionEval result must contain exactly one grade=pass|fail" >&2
    exit 1
fi

exit "$launch_rc"
