#!/usr/bin/env bash
#------------------------------------------------------------
#  Script: clean_harness_workdirs.sh
# Purpose: Report or remove retained harness-owned run roots
#------------------------------------------------------------

set -u

ME=$(basename "$0")
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
REPO_DIR=$(cd "$SCRIPT_DIR/.." && pwd -P)
HARNESS_ROOT="$REPO_DIR/harnesses"
TARGETS_FILE="$REPO_DIR/config/harness_targets.json"
TEARDOWN_HELPER="$SCRIPT_DIR/moos_scoped_teardown.sh"

DELETE=no
ALL_AGES=no
OLDER_THAN_DAYS=""
TARGET_TOKEN=""
EXCLUDE_TOKENS=()
TARGET_PATH=""
EXCLUDE_PATHS=()

CANDIDATES=()
CANDIDATE_HARNESSES=()
CANDIDATE_CONTAINERS=()
EMPTY_CONTAINERS=()

FOUND_COUNT=0
FOUND_KB=0
DELETED_COUNT=0
DELETED_KB=0
SKIPPED_ACTIVE=0
FAILED_COUNT=0

usage() {
    cat <<EOF
$ME [OPTIONS]

Report retained harness workdirs by default. Deletion requires --delete and
an explicit age selection.

Options:
  --delete             Remove selected run roots after safety checks
  --all                Select retained roots of every age
  --older-than=<days>  Select roots at least this many 24-hour periods old
  --target=<key>       Limit selection to one config/harness_targets.json key
  --exclude=<key>      Exclude one configured target; may be repeated
  --help, -h           Show this help message

Examples:
  $ME
  $ME --older-than=7
  $ME --delete --older-than=7
  $ME --delete --all --exclude=cmgr_h01 --exclude=cmgr_h02
EOF
}

die() {
    echo "$ME: $*" >&2
    exit 2
}

is_uint() {
    case "$1" in
        ""|*[!0-9]*) return 1 ;;
    esac
    return 0
}

resolve_target_path() {
    local token="$1"

    [ -f "$TARGETS_FILE" ] || die "missing harness target registry: $TARGETS_FILE"
    command -v python3 >/dev/null 2>&1 || die "python3 is required to resolve harness target keys"

    python3 - "$TARGETS_FILE" "$token" <<'PY'
import json
from pathlib import Path
import sys

targets_file = Path(sys.argv[1])
token = sys.argv[2]
with targets_file.open(encoding="utf-8") as stream:
    targets = json.load(stream)

matches = [entry["path"] for entry in targets if entry.get("key") == token]
if len(matches) != 1:
    raise SystemExit(1)
print(matches[0])
PY
}

canonical_dir() {
    (cd "$1" 2>/dev/null && pwd -P)
}

path_is_excluded() {
    local harness_dir="$1"
    local excluded

    for excluded in "${EXCLUDE_PATHS[@]}"; do
        [ "$harness_dir" != "$excluded" ] || return 0
    done
    return 1
}

harness_is_selected() {
    local harness_dir="$1"

    if [ -n "$TARGET_PATH" ] && [ "$harness_dir" != "$TARGET_PATH" ]; then
        return 1
    fi
    path_is_excluded "$harness_dir" && return 1
    return 0
}

mtime_epoch() {
    local path="$1"
    local value

    value=$(stat -f '%m' "$path" 2>/dev/null) && {
        printf '%s\n' "$value"
        return 0
    }
    value=$(stat -c '%Y' "$path" 2>/dev/null) && {
        printf '%s\n' "$value"
        return 0
    }
    return 1
}

size_kb() {
    du -sk "$1" 2>/dev/null | awk 'NR == 1 {print $1}'
}

human_size() {
    du -sh "$1" 2>/dev/null | awk 'NR == 1 {print $1}'
}

safe_candidate_shape() {
    local candidate="$1"
    local harness_dir="$2"
    local container="$3"
    local resolved_candidate

    [ -d "$candidate" ] || return 1
    [ ! -L "$candidate" ] || return 1
    [ -f "$harness_dir/zlaunch.sh" ] || return 1
    resolved_candidate=$(canonical_dir "$candidate") || return 1
    case "$resolved_candidate" in
        "$HARNESS_ROOT"/*) ;;
        *) return 1 ;;
    esac

    case "${container##*/}" in
        .harness_runs)
            case "$candidate" in
                "$container"/run_*|"$container"/scratch_*|"$container"/readiness_scratch_*) return 0 ;;
            esac
            ;;
        workdirs)
            case "$candidate" in
                "$container"/run_*) return 0 ;;
            esac
            ;;
    esac
    case "$candidate" in
        "$harness_dir"/.parallel_*) return 0 ;;
    esac
    return 1
}

add_candidate() {
    local candidate="$1"
    local harness_dir="$2"
    local container="$3"

    safe_candidate_shape "$candidate" "$harness_dir" "$container" || {
        echo "$ME: refusing unrecognized candidate shape: $candidate" >&2
        FAILED_COUNT=$((FAILED_COUNT + 1))
        return
    }
    CANDIDATES+=("$candidate")
    CANDIDATE_HARNESSES+=("$harness_dir")
    CANDIDATE_CONTAINERS+=("$container")
}

add_container_candidates() {
    local container="$1"
    local harness_dir
    local candidate

    harness_dir=$(canonical_dir "${container%/*}") || return
    [ -f "$harness_dir/zlaunch.sh" ] || return
    harness_is_selected "$harness_dir" || return
    EMPTY_CONTAINERS+=("$container")

    while IFS= read -r -d '' candidate; do
        add_candidate "$candidate" "$harness_dir" "$container"
    done < <(find "$container" -mindepth 1 -maxdepth 1 -type d \
        \( -name 'run_*' -o -name 'scratch_*' -o -name 'readiness_scratch_*' \) -print0)
}

discover_candidates() {
    local container
    local candidate
    local harness_dir

    while IFS= read -r -d '' container; do
        add_container_candidates "$container"
    done < <(find "$HARNESS_ROOT" -type d -name '.harness_runs' -prune -print0)

    while IFS= read -r -d '' container; do
        add_container_candidates "$container"
    done < <(find "$HARNESS_ROOT" -type d -name 'workdirs' -prune -print0)

    while IFS= read -r -d '' candidate; do
        harness_dir=$(canonical_dir "${candidate%/*}") || continue
        [ -f "$harness_dir/zlaunch.sh" ] || continue
        harness_is_selected "$harness_dir" || continue
        add_candidate "$candidate" "$harness_dir" "$harness_dir"
    done < <(find "$HARNESS_ROOT" \
        -type d \( -name '.harness_runs' -o -name 'workdirs' \) -prune -o \
        -type d -name '.parallel_*' -print0)
}

candidate_is_old_enough() {
    local candidate="$1"
    local now_epoch="$2"
    local modified_epoch
    local age_seconds
    local minimum_seconds

    [ "$ALL_AGES" = no ] || return 0
    modified_epoch=$(mtime_epoch "$candidate") || return 1
    age_seconds=$((now_epoch - modified_epoch))
    minimum_seconds=$((OLDER_THAN_DAYS * 86400))
    [ "$age_seconds" -ge "$minimum_seconds" ]
}

report_candidate() {
    local candidate="$1"
    local now_epoch="$2"
    local modified_epoch="$3"
    local kb="$4"
    local readable_size="$5"
    local age_days

    age_days=$(((now_epoch - modified_epoch) / 86400))
    [ "$age_days" -ge 0 ] || age_days=0
    printf 'RETAINED age_days=%s size=%s path=%s\n' "$age_days" "$readable_size" "$candidate"
    FOUND_COUNT=$((FOUND_COUNT + 1))
    FOUND_KB=$((FOUND_KB + kb))
}

remove_candidate() {
    local candidate="$1"
    local harness_dir="$2"
    local kb="$3"
    local pids

    pids=$(moos_scoped_teardown_pids_for_root_checked "$candidate") || {
        echo "$ME: unable to verify process state; preserving: $candidate" >&2
        FAILED_COUNT=$((FAILED_COUNT + 1))
        return
    }
    if [ -n "$pids" ]; then
        echo "SKIP active pids=$(printf '%s' "$pids" | tr '\n' ',') path=$candidate"
        SKIPPED_ACTIVE=$((SKIPPED_ACTIVE + 1))
        return
    fi

    if ! moos_scoped_teardown_stop_root "$candidate"; then
        echo "$ME: scoped teardown failed; preserving: $candidate" >&2
        FAILED_COUNT=$((FAILED_COUNT + 1))
        return
    fi

    if ! rm -rf -- "$candidate"; then
        echo "$ME: unable to remove: $candidate" >&2
        FAILED_COUNT=$((FAILED_COUNT + 1))
        return
    fi
    printf 'DELETED size_kb=%s path=%s\n' "$kb" "$candidate"
    DELETED_COUNT=$((DELETED_COUNT + 1))
    DELETED_KB=$((DELETED_KB + kb))
}

for arg in "$@"; do
    case "$arg" in
        --delete) DELETE=yes ;;
        --all) ALL_AGES=yes ;;
        --older-than=*) OLDER_THAN_DAYS="${arg#--older-than=}" ;;
        --target=*) TARGET_TOKEN="${arg#--target=}" ;;
        --exclude=*) EXCLUDE_TOKENS+=("${arg#--exclude=}") ;;
        --help|-h) usage; exit 0 ;;
        *) die "unknown argument: $arg" ;;
    esac
done

if [ "$ALL_AGES" = yes ] && [ -n "$OLDER_THAN_DAYS" ]; then
    die "--all and --older-than cannot be combined"
fi
if [ -n "$OLDER_THAN_DAYS" ]; then
    is_uint "$OLDER_THAN_DAYS" || die "--older-than must be a nonnegative integer"
fi
if [ "$DELETE" = yes ] && [ "$ALL_AGES" = no ] && [ -z "$OLDER_THAN_DAYS" ]; then
    die "--delete requires --all or --older-than=<days>"
fi
if [ "$ALL_AGES" = no ] && [ -z "$OLDER_THAN_DAYS" ]; then
    ALL_AGES=yes
fi
[ -d "$HARNESS_ROOT" ] || die "missing harness root: $HARNESS_ROOT"

if [ -n "$TARGET_TOKEN" ]; then
    target_relative=$(resolve_target_path "$TARGET_TOKEN") || die "unknown harness target: $TARGET_TOKEN"
    TARGET_PATH=$(canonical_dir "$REPO_DIR/$target_relative") || die "missing harness target path: $target_relative"
fi

for exclude_token in "${EXCLUDE_TOKENS[@]}"; do
    exclude_relative=$(resolve_target_path "$exclude_token") || die "unknown harness target: $exclude_token"
    exclude_path=$(canonical_dir "$REPO_DIR/$exclude_relative") || die "missing harness target path: $exclude_relative"
    EXCLUDE_PATHS+=("$exclude_path")
done

if [ "$DELETE" = yes ]; then
    [ -f "$TEARDOWN_HELPER" ] || die "missing scoped teardown helper: $TEARDOWN_HELPER"
    # shellcheck source=/dev/null
    . "$TEARDOWN_HELPER"
    command -v moos_scoped_teardown_pids_for_root_checked >/dev/null 2>&1 || \
        die "scoped teardown helper lacks process inspection"
    command -v moos_scoped_teardown_stop_root >/dev/null 2>&1 || \
        die "scoped teardown helper lacks stop_root"
fi

discover_candidates
now_epoch=$(date +%s)

for ((idx = 0; idx < ${#CANDIDATES[@]}; idx++)); do
    candidate=${CANDIDATES[$idx]}
    harness_dir=${CANDIDATE_HARNESSES[$idx]}
    candidate_is_old_enough "$candidate" "$now_epoch" || continue
    modified_epoch=$(mtime_epoch "$candidate") || {
        echo "$ME: unable to read modification time: $candidate" >&2
        FAILED_COUNT=$((FAILED_COUNT + 1))
        continue
    }
    kb=$(size_kb "$candidate")
    is_uint "$kb" || {
        echo "$ME: unable to measure candidate: $candidate" >&2
        FAILED_COUNT=$((FAILED_COUNT + 1))
        continue
    }
    readable_size=$(human_size "$candidate")
    report_candidate "$candidate" "$now_epoch" "$modified_epoch" "$kb" "${readable_size:-unknown}"
    if [ "$DELETE" = yes ]; then
        remove_candidate "$candidate" "$harness_dir" "$kb"
    fi
done

if [ "$DELETE" = yes ]; then
    for container in "${EMPTY_CONTAINERS[@]}"; do
        rmdir "$container" 2>/dev/null || true
    done
fi

printf 'SUMMARY mode=%s retained=%s retained_kb=%s deleted=%s deleted_kb=%s skipped_active=%s failed=%s\n' \
    "$([ "$DELETE" = yes ] && printf delete || printf report)" \
    "$FOUND_COUNT" "$FOUND_KB" "$DELETED_COUNT" "$DELETED_KB" \
    "$SKIPPED_ACTIVE" "$FAILED_COUNT"

if [ "$FAILED_COUNT" -ne 0 ] || [ "$SKIPPED_ACTIVE" -ne 0 ]; then
    exit 1
fi
exit 0
