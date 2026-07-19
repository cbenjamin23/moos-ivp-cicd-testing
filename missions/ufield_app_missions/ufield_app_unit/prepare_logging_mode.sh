#!/bin/bash
# Prepare direct-stem or post-overlay logging for the selected public mode.

set -u

ME=$(basename "$0")
LOG_MODE="${1:-minimal}"
MINIMAL_POLICY="${2:-none}"
TARGET_FILE="${3:-}"

case "$LOG_MODE" in
    minimal|full) ;;
    *) echo "$ME: --log must be minimal or full" >&2; exit 2 ;;
esac
case "$MINIMAL_POLICY" in
    none|appcast) ;;
    *) echo "$ME: minimal policy must be none or appcast" >&2; exit 2 ;;
esac

if [ -z "$TARGET_FILE" ]; then
    rm -f meta_shoreside.moosx
    if [ "$LOG_MODE" = full ]; then
        nspatch --stem=meta_shoreside.moos full-logging-shoreside.xmoos \
            --targ=meta_shoreside.moosx || exit 1
    fi
    exit 0
fi

[ -f "$TARGET_FILE" ] || {
    echo "$ME: missing post-overlay target: $TARGET_FILE" >&2
    exit 1
}

logger_runs=$(awk '
  /^[[:space:]]*Run[[:space:]]*=[[:space:]]*pLogger[[:space:]]*@/ {count++}
  END {print count+0}
' "$TARGET_FILE")
[ "$logger_runs" -eq 1 ] || {
    echo "$ME: expected one post-overlay pLogger run; found $logger_runs" >&2
    exit 1
}

# Case overlays already inherit the dormant pre-migration configuration from
# the stem. Full mode only needs the overlay's existing pLogger launch line.
if [ "$LOG_MODE" = full ]; then
    exit 0
fi

stem_file="${TARGET_FILE%.moosx}.logging.$$.moos"
tmp_file="${TARGET_FILE%.moosx}.logging.$$.moosx"
trap 'rm -f "$stem_file" "$tmp_file"' EXIT INT TERM

if [ "$MINIMAL_POLICY" = appcast ]; then
    cp "$TARGET_FILE" "$stem_file" || exit 1
    nspatch --stem="$stem_file" minimal-appcast-logging-config.xmoos \
        --targ="$tmp_file" || exit 1
else
    awk '
      /^[[:space:]]*Run[[:space:]]*=[[:space:]]*pLogger[[:space:]]*@/ {
        removed++
        next
      }
      {print}
      END {if (removed != 1) exit 42}
    ' "$TARGET_FILE" > "$tmp_file" || {
        echo "$ME: unable to remove the post-overlay pLogger run" >&2
        exit 1
    }
fi

[ -s "$tmp_file" ] || {
    echo "$ME: logging preparation produced no target" >&2
    exit 1
}
mv "$tmp_file" "$TARGET_FILE" || exit 1
rm -f "$stem_file"
trap - EXIT INT TERM
