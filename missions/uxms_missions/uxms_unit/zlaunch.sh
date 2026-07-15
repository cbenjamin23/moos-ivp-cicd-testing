#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: uxms_unit
#   Author: Charles Benjamin
#   LastEd: Jul 2026
#------------------------------------------------------------

set -u

ME=$(basename "$0")
TIME_WARP=10
MAX_TIME=30
VERBOSE=no
JUST_MAKE=no
EXTERNAL_CLIENT=no
MOOS_PORT=15200
DISPLAY_ARGS=(--nogui)
FLOW_ARGS=()

usage() {
    cat <<EOF
$ME [OPTIONS] [time_warp]

Options:
  --help, -h            Show this help message
  --verbose, -v         Verbose launch output
  --just_make, -j       Only create targ files
  --nogui, -ng          Headless launch, no gui (default)
  --gui                 Accepted for wrapper parity
  --max_time=<secs>     Maximum time passed to xlaunch
  --shore_mport=<n>     Shoreside MOOSDB port
  --shore_pshare=<n>    Reserved shoreside pShare port
  --mmod=<name>         Mission modification label
  --external_client     Let a harness capture uXMS before evaluation
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
        --external_client) EXTERNAL_CLIENT=yes ;;
        --nogui|-ng) DISPLAY_ARGS=(--nogui) ;;
        --gui) DISPLAY_ARGS=() ;;
        --max_time=*) MAX_TIME="${arg#--max_time=}" ;;
        --shore_mport=*) MOOS_PORT="${arg#--shore_mport=}"; FLOW_ARGS+=("$arg") ;;
        --shore_pshare=*|--mmod=*) FLOW_ARGS+=("$arg") ;;
        *)
            is_uint "$arg" || die "bad argument: $arg"
            TIME_WARP="$arg"
            ;;
    esac
done

is_uint "$TIME_WARP" && [ "$TIME_WARP" -gt 0 ] || die "time warp must be a positive integer"
is_uint "$MAX_TIME" && [ "$MAX_TIME" -gt 0 ] || die "--max_time must be a positive integer"

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

: > results.txt
[ "$VERBOSE" = yes ] && echo "$ME: launching with max_time=$MAX_TIME warp=$TIME_WARP"

if [ "$JUST_MAKE" = yes ]; then
    rm -f targ_shoreside.moos
fi

launch_args=(--max_time="$MAX_TIME")
if [ "${#DISPLAY_ARGS[@]}" -gt 0 ]; then
    launch_args+=("${DISPLAY_ARGS[@]}")
fi
if [ "${#FLOW_ARGS[@]}" -gt 0 ]; then
    launch_args+=("${FLOW_ARGS[@]}")
fi
launch_args+=("$TIME_WARP")
[ "$JUST_MAKE" = yes ] && launch_args+=(--just_make)

launch_rc=0
readiness_rc=0

if [ "$JUST_MAKE" = yes ]; then
    xlaunch.sh "${launch_args[@]}" || launch_rc=$?
else
    xlaunch.sh "${launch_args[@]}" &
    xlaunch_pid=$!
    if [ "$EXTERNAL_CLIENT" = no ]; then
        sleep 0.75
        uPokeDB --quiet --host=localhost --port="$MOOS_PORT" \
            TEST_EVAL_READY=true > readiness.out 2>&1 || readiness_rc=$?
    fi
    wait "$xlaunch_pid" || launch_rc=$?
fi

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
if [ -n "$result_line" ]; then
    read -r -a result_fields <<< "$result_line"
fi
for field in "${result_fields[@]}"; do
    case "$field" in
        grade=*)
            grade_count=$((grade_count + 1))
            result_grade="${field#grade=}"
            ;;
    esac
done

if [ "$grade_count" -ne 1 ] || \
   { [ "$result_grade" != pass ] && [ "$result_grade" != fail ]; }; then
    echo "$ME: pMissionEval did not write exactly one grade=pass|fail result" >&2
    exit 1
fi

[ "$readiness_rc" -eq 0 ] || exit "$readiness_rc"
exit "$launch_rc"
