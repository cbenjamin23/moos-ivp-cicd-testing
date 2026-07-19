#!/usr/bin/env bash
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: mission_utility_unit
#   Author: Charles Benjamin
#   LastEd: Jul 2026
#------------------------------------------------------------

set -u

ME=$(basename "$0")
TIME_WARP=1
VERBOSE=no
JUST_MAKE=no
LOG_CLEAN=no
LOG_MODE=minimal
if [ "${LOG_MODE_PREPARED:-no}" = yes ] && [ -n "${LOG_MODE_PREPARED_VALUE:-}" ]; then
    LOG_MODE="$LOG_MODE_PREPARED_VALUE"
fi
MOOS_PORT=9000
PSHARE_PORT=9015
MMOD=""
XLAUNCHED=no
SHORE_MOOS=meta_shoreside.moos

usage() {
    cat <<EOF
$ME [OPTIONS] [time_warp]

Options:
  --help, -h            Show this help message
  --verbose, -v         Verbose launch output
  --just_make, -j       Only create target files
  --log=<mode>          minimal (default) or full
  --log_clean, -lc      Run clean.sh before launch
  --mport=<n>           MOOSDB port
  --pshare=<n>          Reserved pShare port
  --shore_mport=<n>     MOOSDB port alias for xlaunch
  --shore_pshare=<n>    Reserved pShare port alias for xlaunch
  --mmod=<name>         Optional standalone mission modification label
  --xlaunched, -x       Launched by xlaunch or a harness
  --nogui, -ng          Accepted for wrapper parity
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
        --verbose|-v) VERBOSE=yes ;;
        --just_make|-j) JUST_MAKE=yes ;;
        --log=*) LOG_MODE="${arg#--log=}" ;;
        --log_clean|-lc) LOG_CLEAN=yes ;;
        --mport=*) MOOS_PORT="${arg#--mport=}" ;;
        --pshare=*) PSHARE_PORT="${arg#--pshare=}" ;;
        --shore_mport=*) MOOS_PORT="${arg#--shore_mport=}" ;;
        --shore_pshare=*) PSHARE_PORT="${arg#--shore_pshare=}" ;;
        --veh_mport=*|--veh_pshare=*) ;;
        --mmod=*) MMOD="${arg#--mmod=}" ;;
        --xlaunched|-x) XLAUNCHED=yes ;;
        --nogui|-ng) ;;
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
if [ "${LOG_MODE_PREPARED:-no}" != yes ]; then
    ./prepare_logging_mode.sh "$LOG_MODE" || die "unable to prepare --log=$LOG_MODE"
fi
export LOG_MODE_PREPARED=yes
export LOG_MODE_PREPARED_VALUE="$LOG_MODE"

is_uint "$TIME_WARP" && [ "$TIME_WARP" -gt 0 ] || die "time warp must be a positive integer"
is_uint "$MOOS_PORT" && [ "$MOOS_PORT" -gt 0 ] && [ "$MOOS_PORT" -le 65535 ] || \
    die "MOOSDB port must be an integer from 1 through 65535"
is_uint "$PSHARE_PORT" && [ "$PSHARE_PORT" -gt 0 ] && [ "$PSHARE_PORT" -le 65535 ] || \
    die "pShare port must be an integer from 1 through 65535"

[ "$LOG_CLEAN" = no ] || ./clean.sh
[ -f meta_shoreside.moosx ] && SHORE_MOOS=meta_shoreside.moosx

nsplug_args=(
    "$SHORE_MOOS"
    targ_shoreside.moos
    --strict
    --force
    -x
    WARP="$TIME_WARP"
    MOOS_PORT="$MOOS_PORT"
    PSHARE_PORT="$PSHARE_PORT"
)
[ -n "$MMOD" ] && nsplug_args+=(MMOD="$MMOD")
nsplug "${nsplug_args[@]}"

if [ "$JUST_MAKE" = yes ]; then
    [ "$VERBOSE" = no ] || echo "$ME: target files made; exiting without launch"
    exit 0
fi

[ "$VERBOSE" = no ] || echo "$ME: launching mission utility unit community"
pAntler targ_shoreside.moos >/dev/null 2>&1 &

if [ "$XLAUNCHED" = no ]; then
    uMAC --paused targ_shoreside.moos || true
    REPO_DIR=$(git -C "$PWD" rev-parse --show-toplevel 2>/dev/null) || \
        die "unable to locate repository root from $PWD"
    # shellcheck source=/dev/null
    . "$REPO_DIR/scripts/moos_scoped_teardown.sh"
    moos_scoped_teardown_stop_root "$PWD"
fi

exit 0
