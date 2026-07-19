#!/bin/bash
# Prepare direct-stem or post-overlay logging for the selected public mode.

set -u

ME=$(basename "$0")
LOG_MODE="${1:-minimal}"
MINIMAL_POLICY="${2:-none}"
SHORE_TARGET="${3:-}"
VEHICLE_TARGET="${4:-}"

case "$LOG_MODE" in
    minimal|full) ;;
    *) echo "$ME: --log must be minimal or full" >&2; exit 2 ;;
esac
case "$MINIMAL_POLICY" in
    none|grading) ;;
    *) echo "$ME: minimal policy must be none or grading" >&2; exit 2 ;;
esac

if [ -z "$SHORE_TARGET" ] && [ -z "$VEHICLE_TARGET" ]; then
    rm -f meta_shoreside.moosx meta_vehicle.moosx
    if [ "$LOG_MODE" = full ]; then
        nspatch --stem=meta_shoreside.moos full-logging-shoreside.xmoos \
            --targ=meta_shoreside.moosx || exit 1
        nspatch --stem=meta_vehicle.moos full-logging-vehicle.xmoos \
            --targ=meta_vehicle.moosx || exit 1
    fi
    exit 0
fi

[ -n "$SHORE_TARGET" ] && [ -n "$VEHICLE_TARGET" ] || {
    echo "$ME: both post-overlay targets are required" >&2
    exit 1
}

prepare_target() {
    local target_file="$1"
    local config_patch="$2"
    local logger_runs
    local stem_file
    local tmp_file

    [ -f "$target_file" ] || {
        echo "$ME: missing post-overlay target: $target_file" >&2
        return 1
    }
    logger_runs=$(awk '
      /^[[:space:]]*Run[[:space:]]*=[[:space:]]*pLogger[[:space:]]*@/ {count++}
      END {print count+0}
    ' "$target_file")

    if [ "$MINIMAL_POLICY" = none ]; then
        [ "$logger_runs" -le 1 ] || {
            echo "$ME: expected at most one post-overlay pLogger run; found $logger_runs" >&2
            return 1
        }
        [ "$logger_runs" -eq 1 ] || return 0
        tmp_file="${target_file%.moosx}.logging.$$.moosx"
        awk '
          /^[[:space:]]*Run[[:space:]]*=[[:space:]]*pLogger[[:space:]]*@/ {
            removed++
            next
          }
          {print}
          END {if (removed != 1) exit 42}
        ' "$target_file" > "$tmp_file" || return 1
        mv "$tmp_file" "$target_file" || return 1
        return 0
    fi

    [ "$logger_runs" -eq 1 ] || {
        echo "$ME: expected one grading pLogger run; found $logger_runs" >&2
        return 1
    }
    stem_file="${target_file%.moosx}.logging.$$.moos"
    tmp_file="${target_file%.moosx}.logging.$$.moosx"
    cp "$target_file" "$stem_file" || return 1
    nspatch --stem="$stem_file" "$config_patch" --targ="$tmp_file" || return 1
    [ -s "$tmp_file" ] || return 1
    mv "$tmp_file" "$target_file" || return 1
    rm -f "$stem_file"
}

[ "$LOG_MODE" = minimal ] || exit 0
trap 'rm -f ./*.logging."$$".moos ./*.logging."$$".moosx' EXIT INT TERM
prepare_target "$SHORE_TARGET" minimal-grading-shoreside.xmoos || exit 1
prepare_target "$VEHICLE_TARGET" minimal-grading-vehicle.xmoos || exit 1
trap - EXIT INT TERM
