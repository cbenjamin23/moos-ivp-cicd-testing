#!/bin/bash
#------------------------------------------------------------
#   Script: benchmark_parallel.sh
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------

set -euo pipefail

ME=$(basename "$0")
HARNESS=""
JOBS_LIST=""
TIME_WARP="10"
OUT_ROOT=""
MAX_TIME=""

for ARGI in "$@"; do
    if [ "$ARGI" = "--help" ] || [ "$ARGI" = "-h" ]; then
        echo "$ME --harness=<abs-path> --jobs=1,2,4 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --harness=<path>   Absolute path to harness directory"
        echo "  --jobs=<csv>       Comma-separated jobs list"
        echo "  --time_warp=<n>    Time warp passed to zlaunch (default 10)"
        echo "  --out=<dir>        Output directory root"
        echo "  --max_time=<secs>  Optional harness max_time override"
        echo ""
        echo "Example:"
        echo "  $ME --harness=/abs/H01-cmgr_unit --jobs=1,2,4,8,16"
        exit 0
    elif [[ "$ARGI" == --harness=* ]]; then
        HARNESS="${ARGI#--harness=}"
    elif [[ "$ARGI" == --jobs=* ]]; then
        JOBS_LIST="${ARGI#--jobs=}"
    elif [[ "$ARGI" == --time_warp=* ]]; then
        TIME_WARP="${ARGI#--time_warp=}"
    elif [[ "$ARGI" == --out=* ]]; then
        OUT_ROOT="${ARGI#--out=}"
    elif [[ "$ARGI" == --max_time=* ]]; then
        MAX_TIME="${ARGI#--max_time=}"
    else
        echo "$ME: Bad arg: $ARGI"
        exit 1
    fi
done

if [ -z "$HARNESS" ] || [ -z "$JOBS_LIST" ]; then
    echo "$ME: --harness and --jobs are required"
    exit 1
fi

if [ ! -d "$HARNESS" ] || [ ! -x "$HARNESS/zlaunch.sh" ]; then
    echo "$ME: Harness dir or zlaunch.sh missing: $HARNESS"
    exit 1
fi

if [ -z "$OUT_ROOT" ]; then
    OUT_ROOT="$(cd "$HARNESS/../../.." && pwd)/time_benchmarking"
fi

STAMP="$(date +%Y-%m-%d_%H%M%S)"
HARNESS_NAME="$(basename "$HARNESS")"
OUT_DIR="$OUT_ROOT/${HARNESS_NAME}_parallel_$STAMP"
RAW_DIR="$OUT_DIR/raw"
mkdir -p "$RAW_DIR"

read -r -a JOBS_ARRAY <<< "$(echo "$JOBS_LIST" | tr ',' ' ')"

SUMMARY="$OUT_DIR/README.md"
{
    echo "# Parallel Benchmark"
    echo
    echo "Date: $(date '+%B %d, %Y %H:%M:%S %Z')"
    echo
    echo "Harness:"
    echo
    echo "- \`$HARNESS\`"
    echo
    echo "Time warp:"
    echo
    echo "- \`$TIME_WARP\`"
    if [ -n "$MAX_TIME" ]; then
        echo "- max_time override: \`$MAX_TIME\`"
    fi
    echo
    echo "Results:"
    echo
} > "$SUMMARY"

for JOBS in "${JOBS_ARRAY[@]}"; do
    if ! echo "$JOBS" | grep -Eq '^[0-9]+$'; then
        echo "$ME: Bad jobs value: $JOBS"
        exit 1
    fi

    LOG_FILE="$RAW_DIR/jobs${JOBS}.log"
    TIME_FILE="$RAW_DIR/jobs${JOBS}.time"

    echo "$ME: Running jobs=$JOBS"
    ktm >/dev/null 2>&1 || true

    CMD=(./zlaunch.sh "--jobs=$JOBS")
    if [ -n "$MAX_TIME" ]; then
        CMD+=("--max_time=$MAX_TIME")
    fi
    CMD+=("$TIME_WARP")

    (
        cd "$HARNESS"
        /usr/bin/time -p "${CMD[@]}" >"$LOG_FILE" 2>"$TIME_FILE"
    )
    RC=$?

    REAL_TIME="$(awk '/^real /{print $2}' "$TIME_FILE")"
    STATUS="ok"
    if [ "$RC" -ne 0 ]; then
        STATUS="nonzero_exit"
    fi

    {
        echo "- \`jobs=$JOBS\`: \`${REAL_TIME}s\` (\`$STATUS\`)"
    } >> "$SUMMARY"
done

echo "$ME: Wrote $SUMMARY"
