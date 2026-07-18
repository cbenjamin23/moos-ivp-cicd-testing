#!/usr/bin/env bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: mission_utility_unit
#   Author: Charles Benjamin
#   LastEd: Jul 2026
#------------------------------------------------------------

set -u

ME=$(basename "$0")
TIME_WARP=10
MAX_TIME=80
VERBOSE=no
JUST_MAKE=no
MOOS_PORT=9000
PSHARE_PORT=9015
DISPLAY_ARGS=(--nogui)
FLOW_ARGS=()

usage() {
    cat <<EOF
$ME [OPTIONS] [time_warp]

Options:
  --help, -h            Show this help message
  --verbose, -v         Verbose launch output
  --just_make, -j       Only create target files
  --nogui, -ng          Headless launch, no gui (default)
  --gui                 Accepted for wrapper parity
  --max_time=<secs>     Maximum time passed to xlaunch
  --port_base=<n>       Compatibility alias for the MOOSDB port
  --shore_mport=<n>     Shoreside MOOSDB port
  --shore_pshare=<n>    Reserved shoreside pShare port
  --mmod=<name>         Optional standalone mission modification label
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

for arg in "$@"; do
    case "$arg" in
        --help|-h) usage; exit 0 ;;
        --verbose|-v) VERBOSE=yes; FLOW_ARGS+=(--verbose) ;;
        --just_make|-j) JUST_MAKE=yes ;;
        --nogui|-ng) DISPLAY_ARGS=(--nogui) ;;
        --gui) DISPLAY_ARGS=() ;;
        --max_time=*) MAX_TIME="${arg#--max_time=}" ;;
        --port_base=*)
            MOOS_PORT="${arg#--port_base=}"
            is_uint "$MOOS_PORT" || die "--port_base must be an integer"
            PSHARE_PORT=$((10#$MOOS_PORT + 15))
            ;;
        --shore_mport=*) MOOS_PORT="${arg#--shore_mport=}" ;;
        --shore_pshare=*) PSHARE_PORT="${arg#--shore_pshare=}" ;;
        --mmod=*) FLOW_ARGS+=("$arg") ;;
        *)
            is_uint "$arg" || die "bad argument: $arg"
            TIME_WARP="$arg"
            ;;
    esac
done

is_uint "$TIME_WARP" && [ "$TIME_WARP" -gt 0 ] || die "time warp must be a positive integer"
is_uint "$MAX_TIME" && [ "$MAX_TIME" -gt 0 ] || die "--max_time must be a positive integer"
is_uint "$MOOS_PORT" && [ "$MOOS_PORT" -gt 0 ] && [ "$MOOS_PORT" -le 65535 ] || \
    die "MOOSDB port must be an integer from 1 through 65535"
is_uint "$PSHARE_PORT" && [ "$PSHARE_PORT" -gt 0 ] && [ "$PSHARE_PORT" -le 65535 ] || \
    die "pShare port must be an integer from 1 through 65535"

REPO_DIR=$(git -C "$PWD" rev-parse --show-toplevel 2>/dev/null) || \
    die "unable to locate repository root from $PWD"
TEARDOWN_HELPER="$REPO_DIR/scripts/moos_scoped_teardown.sh"
[ -f "$TEARDOWN_HELPER" ] || die "missing scoped teardown helper: $TEARDOWN_HELPER"
# shellcheck source=/dev/null
. "$TEARDOWN_HELPER"

# shellcheck disable=SC2329  # Invoked through the EXIT-trap handler.
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

: > results.txt
[ "$VERBOSE" = yes ] && echo "$ME: launching with max_time=$MAX_TIME warp=$TIME_WARP"
[ "$JUST_MAKE" = yes ] && rm -f targ_shoreside.moos

launch_args=(
    --max_time="$MAX_TIME"
    --shore_mport="$MOOS_PORT"
    --shore_pshare="$PSHARE_PORT"
)
[ "${#DISPLAY_ARGS[@]}" -gt 0 ] && launch_args+=("${DISPLAY_ARGS[@]}")
[ "${#FLOW_ARGS[@]}" -gt 0 ] && launch_args+=("${FLOW_ARGS[@]}")
launch_args+=("$TIME_WARP")
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

for ((attempt = 0; attempt < 60; attempt++)); do
    grep -q 'grade=' results.txt 2>/dev/null && break
    sleep 0.05
done

result_rows=$(awk 'NF {count++} END {print count+0}' results.txt 2>/dev/null)
if [ "$result_rows" -ne 1 ]; then
    echo "$ME: expected exactly one pMissionEval result row; found $result_rows" >&2
    exit 1
fi

result_line=$(awk 'NF {print; exit}' results.txt 2>/dev/null)
result_grade=""
grade_count=0
result_fields=()
read -r -a result_fields <<< "$result_line"
for field in "${result_fields[@]}"; do
    case "$field" in
        grade=*)
            grade_count=$((grade_count + 1))
            result_grade="${field#grade=}"
            result_grade="${result_grade%,}"
            ;;
    esac
done

if [ "$grade_count" -ne 1 ] || \
   { [ "$result_grade" != pass ] && [ "$result_grade" != fail ]; }; then
    echo "$ME: pMissionEval did not write exactly one grade=pass|fail result" >&2
    exit 1
fi

exit "$launch_rc"
