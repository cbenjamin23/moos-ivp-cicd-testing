#!/bin/bash

ME=$(basename "$0")
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
BUILD_DIR="$REPO_ROOT/build"
FAMILY_TOOL="$REPO_ROOT/scripts/ci_cpp_test_targets.py"

DO_BUILD=yes
EXPLICIT_ALL=no
JOBS=1
FAMILIES=()
SELECTED=()

family_text=$(python3 "$FAMILY_TOOL" list) || exit 1
while IFS= read -r family; do
    [ -n "$family" ] && FAMILIES+=("$family")
done <<< "$family_text"

is_family() {
    local wanted="$1"
    local family
    for family in "${FAMILIES[@]}"; do
        [ "$family" = "$wanted" ] && return 0
    done
    return 1
}

is_selected() {
    local wanted="$1"
    local family
    for family in "${SELECTED[@]}"; do
        [ "$family" = "$wanted" ] && return 0
    done
    return 1
}

focused_build_target() {
    # Keep public CTest labels case-sensitive while matching CMake's
    # filesystem-safe lowercase target key.
    printf 'ctest-family-%s' "$1" | tr '[:upper:]' '[:lower:]'
}

usage() {
    local family
    cat <<EOF
$ME [OPTIONS]

Build and run the repository's C++ tests. Family options build and run only
the selected families; with no family options, all tests run.

Options:
  --all             Run all CTest families (default)
  --no-build        Skip the incremental test-repository build
  --jobs=N          Run the build and CTest with N parallel jobs (default: 1)
  --help, -h        Show this help message

CTest families (options may be combined):
EOF
    for family in "${FAMILIES[@]}"; do
        printf '  --%-24s Run the %s family\n' "$family" "$family"
    done
}

for arg in "$@"; do
    case "$arg" in
        --all)
            EXPLICIT_ALL=yes
            ;;
        --no-build)
            DO_BUILD=no
            ;;
        --jobs=*)
            JOBS="${arg#--jobs=}"
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        --*)
            family="${arg#--}"
            if ! is_family "$family"; then
                echo "$ME: unknown option or CTest family: $arg" >&2
                echo "$ME: use --help to list valid families" >&2
                exit 2
            fi
            is_selected "$family" || SELECTED+=("$family")
            ;;
        *)
            echo "$ME: bad argument: $arg" >&2
            echo "$ME: family selectors use --family-name" >&2
            exit 2
            ;;
    esac
done

case "$JOBS" in
    ''|*[!0-9]*)
        echo "$ME: --jobs must be a positive integer" >&2
        exit 2
        ;;
esac
[ "$JOBS" -gt 0 ] 2>/dev/null || {
    echo "$ME: --jobs must be a positive integer" >&2
    exit 2
}

if [ "$EXPLICIT_ALL" = yes ] && [ "${#SELECTED[@]}" -gt 0 ]; then
    echo "$ME: --all cannot be combined with family options" >&2
    exit 2
fi

if [ "$DO_BUILD" = yes ]; then
    build_targets=()
    if [ "${#SELECTED[@]}" -gt 0 ]; then
        for family in "${SELECTED[@]}"; do
            build_targets+=("$(focused_build_target "$family")")
        done
    fi

    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        if [ "${#build_targets[@]}" -eq 0 ]; then
            echo "$ME: configuring and building all C++ tests"
            (cd "$REPO_ROOT" && ./build.sh "-j$JOBS") || exit $?
        else
            echo "$ME: configuring and building CTest families: ${SELECTED[*]}"
            (cd "$REPO_ROOT" && ./build.sh "-j$JOBS" "${build_targets[@]}") || exit $?
        fi
    elif [ "${#build_targets[@]}" -eq 0 ]; then
        echo "$ME: incrementally building all C++ tests"
        cmake --build "$BUILD_DIR" --parallel "$JOBS" || exit $?
    else
        echo "$ME: incrementally building CTest families: ${SELECTED[*]}"
        cmake --build "$BUILD_DIR" --parallel "$JOBS" --target "${build_targets[@]}" || exit $?
    fi
elif [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "$ME: test build is not configured; omit --no-build or run $REPO_ROOT/build.sh" >&2
    exit 2
fi

ctest_args=(
    --test-dir "$BUILD_DIR"
    --output-on-failure
    --no-tests=error
    --parallel "$JOBS"
)

if [ "${#SELECTED[@]}" -eq 0 ]; then
    echo "$ME: running all CTest families"
else
    label_regex=$(python3 "$FAMILY_TOOL" label-regex "${SELECTED[@]}") || exit $?
    ctest_args+=( -L "$label_regex" )
    echo "$ME: running CTest families: ${SELECTED[*]}"
fi

summary_file=$(mktemp "${TMPDIR:-/tmp}/${ME}.summary.XXXXXX") || exit 1
trap 'rm -f "$summary_file"' EXIT

ctest "${ctest_args[@]}" 2>&1 | awk -v summary_file="$summary_file" '
    /tests passed, [0-9]+ tests failed out of [0-9]+/ {
        print > summary_file
        close(summary_file)
        next
    }
    {
        print
        fflush()
    }
'
pipeline_status=("${PIPESTATUS[@]}")

if [ -s "$summary_file" ]; then
    printf '\n'
    cat "$summary_file"
fi

if [ "${pipeline_status[0]}" -ne 0 ]; then
    exit "${pipeline_status[0]}"
fi
exit "${pipeline_status[1]}"
