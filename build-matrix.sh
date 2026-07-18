#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
TARGET_SET="current"
TARGETS=""
BUILD_PROFILE="full"
MOOS_SOURCE="${MOOS_IVP_SOURCE:-${MOOSIVP_SOURCE_TREE_BASE:-}}"
DOCKER_PLATFORM=""
DRY_RUN="no"
RUN_NATIVE="yes"
RUN_DOCKER="yes"

usage() {
  cat <<'EOF'
Usage: ./build-matrix.sh [OPTIONS]

Build the local MOOS-IvP checkout natively, then in clean Linux containers.

With no options, runs the normal build on this Mac plus full builds on the
four "current" Linux targets: Ubuntu GCC, Ubuntu Clang, Debian GCC, and
Rocky Linux GCC.

Options:
  --target=NAME        Run one target.
  --targets=A,B        Run comma-separated targets.
  --set=NAME           Run a target set (default: current).
                       Sets: current, compat, modern, gcc, clang,
                       ubuntu, debian, rocky, all.
  --profile=PROFILE    full (default) or headless.
  --source=PATH        MOOS-IvP checkout to build. By default, use the
                       checkout recorded in build/CMakeCache.txt, then
                       fall back to ../moos-ivp.
  --platform=PLATFORM  Docker platform, for example linux/amd64.
  --skip-native        Skip the build on this computer.
  --native-only        Run only the build on this computer.
  --list               List Linux targets and exit.
  --dry-run            Show the selected builds without running them.
  -h, --help           Show this help.

Examples:
  ./build-matrix.sh
  ./build-matrix.sh --native-only
  ./build-matrix.sh --target=ubuntu_2404_gcc
  ./build-matrix.sh --set=compat --profile=headless
EOF
}

die() {
  echo "build-matrix.sh: $*" >&2
  exit 1
}

value_after_equals() {
  printf '%s\n' "${1#*=}"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --target=*)
      TARGET_SET="specific_targets"
      TARGETS="$(value_after_equals "$1")"
      ;;
    --target)
      [ "$#" -ge 2 ] || die "--target requires a value"
      TARGET_SET="specific_targets"
      TARGETS="$2"
      shift
      ;;
    --targets=*)
      TARGET_SET="specific_targets"
      TARGETS="$(value_after_equals "$1")"
      ;;
    --targets)
      [ "$#" -ge 2 ] || die "--targets requires a value"
      TARGET_SET="specific_targets"
      TARGETS="$2"
      shift
      ;;
    --set=*) TARGET_SET="$(value_after_equals "$1")" ;;
    --set)
      [ "$#" -ge 2 ] || die "--set requires a value"
      TARGET_SET="$2"
      shift
      ;;
    --profile=*) BUILD_PROFILE="$(value_after_equals "$1")" ;;
    --profile)
      [ "$#" -ge 2 ] || die "--profile requires a value"
      BUILD_PROFILE="$2"
      shift
      ;;
    --source=*) MOOS_SOURCE="$(value_after_equals "$1")" ;;
    --source)
      [ "$#" -ge 2 ] || die "--source requires a value"
      MOOS_SOURCE="$2"
      shift
      ;;
    --platform=*) DOCKER_PLATFORM="$(value_after_equals "$1")" ;;
    --platform)
      [ "$#" -ge 2 ] || die "--platform requires a value"
      DOCKER_PLATFORM="$2"
      shift
      ;;
    --skip-native) RUN_NATIVE="no" ;;
    --native-only)
      RUN_NATIVE="yes"
      RUN_DOCKER="no"
      ;;
    --list)
      exec python3 "$SCRIPT_DIR/scripts/moos_ivp_build_matrix.py" list
      ;;
    --dry-run) DRY_RUN="yes" ;;
    -h|--help)
      usage
      exit 0
      ;;
    *) die "unknown option: $1 (use --help)" ;;
  esac
  shift
done

if [ "$RUN_NATIVE" = "no" ] && [ "$RUN_DOCKER" = "no" ]; then
  die "no builds selected"
fi
case "$BUILD_PROFILE" in
  full|headless) ;;
  *) die "unknown build profile: $BUILD_PROFILE (use full or headless)" ;;
esac

if [ -z "$MOOS_SOURCE" ] && [ -f "$SCRIPT_DIR/build/CMakeCache.txt" ]; then
  MOOS_SOURCE="$(sed -n 's/^MOOSIVP_SOURCE_TREE_BASE:PATH=//p' "$SCRIPT_DIR/build/CMakeCache.txt" | tail -n 1)"
fi
if [ -z "$MOOS_SOURCE" ] && [ -d "$SCRIPT_DIR/../moos-ivp" ]; then
  MOOS_SOURCE="$SCRIPT_DIR/../moos-ivp"
fi
if [ -z "$MOOS_SOURCE" ]; then
  die "could not find MOOS-IvP; use --source=/path/to/moos-ivp"
fi
if [ ! -d "$MOOS_SOURCE" ] || [ ! -f "$MOOS_SOURCE/build.sh" ]; then
  die "not a MOOS-IvP checkout: $MOOS_SOURCE"
fi
MOOS_SOURCE="$(cd "$MOOS_SOURCE" && pwd -P)"

selection_file="$(mktemp "${TMPDIR:-/tmp}/moos-build-matrix.XXXXXX")" || exit 1
trap 'rm -f "$selection_file"' EXIT INT TERM

if [ "$RUN_DOCKER" = "yes" ]; then
  python3 "$SCRIPT_DIR/scripts/moos_ivp_build_matrix.py" select \
    --target-set "$TARGET_SET" \
    --targets "$TARGETS" \
    --build-profile "$BUILD_PROFILE" > "$selection_file" || exit 1
fi

native_key="native_host"
native_description="$(uname -s) $(uname -m)"
if [ "$(uname -s)" = "Darwin" ]; then
  native_key="macos_native"
  native_description="macOS $(sw_vers -productVersion) / $(uname -m)"
fi

echo "MOOS-IvP checkout: $MOOS_SOURCE"
echo "Build profile: $BUILD_PROFILE"
echo "Selected builds:"
if [ "$RUN_NATIVE" = "yes" ]; then
  printf '  %-22s %s\n' "$native_key" "$native_description"
fi
while IFS="$(printf '\t')" read -r target_key image compiler; do
  printf '  %-22s %-28s %s\n' "$target_key" "$image" "$compiler"
done < "$selection_file"

if [ "$DRY_RUN" = "yes" ]; then
  echo "Dry run only; no builds were started."
  exit 0
fi

if [ "$RUN_DOCKER" = "yes" ]; then
  command -v docker >/dev/null 2>&1 || die "Docker is not installed"
  docker info >/dev/null 2>&1 || die "Docker is installed but is not running"
fi

run_stamp="$(date '+%Y%m%d-%H%M%S')"
results_dir="$SCRIPT_DIR/build_matrix_results/$run_stamp"
mkdir -p "$results_dir"

passed=0
failed=0
failed_targets=""

if [ "$RUN_NATIVE" = "yes" ]; then
  native_results="$results_dir/$native_key"
  mkdir -p "$native_results"

  echo
  echo "=== $native_key: $native_description ==="
  if (
    cd "$MOOS_SOURCE"
    case "$BUILD_PROFILE" in
      full)
        ./build.sh
        ;;
      headless)
        ./build-moos.sh --minrobot --release
        ./build-ivp.sh --nogui
        ;;
    esac
  ) > >(tee "$native_results/build.log") 2>&1; then
    echo "PASS: $native_key"
    passed=$((passed + 1))
  else
    echo "FAIL: $native_key (see $native_results/build.log)" >&2
    failed=$((failed + 1))
    failed_targets="$failed_targets $native_key"
  fi
fi

while IFS="$(printf '\t')" read -r target_key image compiler; do
  target_results="$results_dir/$target_key"
  mkdir -p "$target_results"

  echo
  echo "=== $target_key: $image / $compiler ==="

  docker_args=(
    run --rm
    --mount "type=bind,src=$MOOS_SOURCE,dst=/input/moos-ivp,readonly"
    --mount "type=bind,src=$SCRIPT_DIR,dst=/workspace,readonly"
    --mount "type=bind,src=$target_results,dst=/output"
    --env "MOOS_IVP_SOURCE=/input/moos-ivp"
    --env "TARGET_KEY=$target_key"
    --env "TARGET_COMPILER=$compiler"
    --env "BUILD_PROFILE=$BUILD_PROFILE"
    --env "WORK_DIR=/tmp/moos-portability"
    --env "BUILD_LOG=/output/build.log"
  )
  if [ -n "$DOCKER_PLATFORM" ]; then
    docker_args+=(--platform "$DOCKER_PLATFORM")
  fi

  if docker "${docker_args[@]}" "$image" bash /workspace/scripts/build_moos_ivp_target.sh; then
    echo "PASS: $target_key"
    passed=$((passed + 1))
  else
    echo "FAIL: $target_key (see $target_results/build.log)" >&2
    failed=$((failed + 1))
    failed_targets="$failed_targets $target_key"
  fi
done < "$selection_file"

echo
echo "Build matrix result: $passed passed, $failed failed"
echo "Logs: $results_dir"
if [ "$failed" -ne 0 ]; then
  echo "Failed targets:$failed_targets" >&2
  exit 1
fi
