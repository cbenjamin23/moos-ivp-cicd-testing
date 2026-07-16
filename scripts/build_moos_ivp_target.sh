#!/usr/bin/env bash
set -euo pipefail

MOOS_IVP_REF="${1:-${MOOS_IVP_REF:-main}}"
MOOS_IVP_REPO="${MOOS_IVP_REPO:-https://github.com/moos-ivp/moos-ivp.git}"
TARGET_KEY="${TARGET_KEY:-local}"
TARGET_COMPILER="${TARGET_COMPILER:-gcc}"
BUILD_PROFILE="${BUILD_PROFILE:-full}"
WORK_DIR="${WORK_DIR:-${PWD}/_moos_ivp_build}"
SKIP_DEP_INSTALL="${SKIP_DEP_INSTALL:-0}"
BUILD_LOG="${BUILD_LOG:-}"

if [ -n "$BUILD_LOG" ]; then
  mkdir -p "$(dirname "$BUILD_LOG")"
  exec > >(tee "$BUILD_LOG") 2>&1
fi

run_as_root() {
  if [ "$(id -u)" -eq 0 ]; then
    "$@"
  elif command -v sudo >/dev/null 2>&1; then
    sudo "$@"
  else
    echo "Root privileges or sudo are required to install build dependencies." >&2
    exit 1
  fi
}

install_apt_deps() {
  local compiler_packages
  local gui_packages=""
  if [ "$TARGET_COMPILER" = "clang" ]; then
    compiler_packages="g++ clang binutils"
  else
    compiler_packages="g++"
  fi
  if [ "$BUILD_PROFILE" = "full" ]; then
    gui_packages="fluid libfltk1.3-dev libgl1-mesa-dev libglu1-mesa-dev libtiff-dev libx11-dev libxft-dev libxinerama-dev"
  fi

  run_as_root apt-get update
  # shellcheck disable=SC2086
  run_as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    git \
    make \
    pkg-config \
    $compiler_packages \
    $gui_packages
}

install_dnf_deps() {
  local package_manager="$1"
  local compiler_packages
  local gui_packages=""
  if [ "$TARGET_COMPILER" = "clang" ]; then
    compiler_packages="gcc gcc-c++ clang"
  else
    compiler_packages="gcc gcc-c++"
  fi
  if [ "$BUILD_PROFILE" = "full" ]; then
    if [ "$package_manager" = "dnf" ]; then
      run_as_root dnf install -y dnf-plugins-core epel-release || true
      run_as_root dnf config-manager --set-enabled crb || true
    fi
    gui_packages="fltk-devel libtiff-devel mesa-libGL-devel mesa-libGLU-devel libX11-devel libXft-devel libXinerama-devel"
  fi

  # shellcheck disable=SC2086
  run_as_root "$package_manager" install -y \
    ca-certificates \
    cmake \
    git \
    make \
    pkgconf-pkg-config \
    $compiler_packages \
    $gui_packages

  if [ "$BUILD_PROFILE" = "full" ] && ! command -v fluid >/dev/null 2>&1; then
    if "$package_manager" repoquery --available fltk-fluid >/dev/null 2>&1; then
      run_as_root "$package_manager" install -y fltk-fluid
    fi
  fi

  if [ "$BUILD_PROFILE" = "full" ] && ! command -v fluid >/dev/null 2>&1; then
    echo "FLTK Fluid is required for BUILD_PROFILE=full but is not available on this target." >&2
    exit 1
  fi
}

install_dependencies() {
  if [ "$SKIP_DEP_INSTALL" = "1" ]; then
    echo "Skipping dependency installation because SKIP_DEP_INSTALL=1."
    return
  fi

  if command -v apt-get >/dev/null 2>&1; then
    install_apt_deps
  elif command -v dnf >/dev/null 2>&1; then
    install_dnf_deps dnf
  elif command -v yum >/dev/null 2>&1; then
    install_dnf_deps yum
  else
    echo "Unsupported package manager. Install git, cmake, make, and a C++ compiler first." >&2
    exit 1
  fi
}

configure_compiler() {
  case "$TARGET_COMPILER" in
    gcc)
      export CC="${CC:-gcc}"
      export CXX="${CXX:-g++}"
      ;;
    clang)
      export CC="${CC:-clang}"
      export CXX="${CXX:-clang++}"
      ;;
    *)
      echo "Unsupported TARGET_COMPILER: $TARGET_COMPILER" >&2
      exit 1
      ;;
  esac
}

validate_build_profile() {
  case "$BUILD_PROFILE" in
    full|headless)
      ;;
    *)
      echo "Unsupported BUILD_PROFILE: $BUILD_PROFILE" >&2
      echo "Supported profiles: full, headless" >&2
      exit 1
      ;;
  esac
}

checkout_moos_ivp() {
  mkdir -p "$WORK_DIR"
  rm -rf "$WORK_DIR/moos-ivp"
  git clone "$MOOS_IVP_REPO" "$WORK_DIR/moos-ivp"
  git -C "$WORK_DIR/moos-ivp" fetch --tags origin "$MOOS_IVP_REF" || true

  if git -C "$WORK_DIR/moos-ivp" rev-parse --verify --quiet "$MOOS_IVP_REF^{commit}" >/dev/null; then
    git -C "$WORK_DIR/moos-ivp" checkout --detach "$MOOS_IVP_REF"
  elif git -C "$WORK_DIR/moos-ivp" rev-parse --verify --quiet "origin/$MOOS_IVP_REF^{commit}" >/dev/null; then
    git -C "$WORK_DIR/moos-ivp" checkout --detach "origin/$MOOS_IVP_REF"
  else
    echo "Unable to resolve MOOS-IvP ref: $MOOS_IVP_REF" >&2
    exit 1
  fi
}

write_summary() {
  local sha="$1"
  if [ -n "${GITHUB_OUTPUT:-}" ]; then
    echo "moos_ivp_sha=$sha" >> "$GITHUB_OUTPUT"
  fi

  if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    {
      echo "### MOOS-IvP Build"
      echo ""
      echo "- Target: \`$TARGET_KEY\`"
      echo "- Compiler: \`$TARGET_COMPILER\`"
      echo "- Build profile: \`$BUILD_PROFILE\`"
      echo "- Repository: \`$MOOS_IVP_REPO\`"
      echo "- Requested ref: \`$MOOS_IVP_REF\`"
      echo "- Resolved SHA: \`$sha\`"
    } >> "$GITHUB_STEP_SUMMARY"
  fi
}

main() {
  echo "Target: $TARGET_KEY"
  echo "Compiler: $TARGET_COMPILER"
  echo "Build profile: $BUILD_PROFILE"
  echo "MOOS-IvP repository: $MOOS_IVP_REPO"
  echo "MOOS-IvP ref: $MOOS_IVP_REF"
  echo "Work directory: $WORK_DIR"

  validate_build_profile
  install_dependencies
  configure_compiler
  checkout_moos_ivp

  local sha
  sha="$(git -C "$WORK_DIR/moos-ivp" rev-parse HEAD)"
  echo "Resolved MOOS-IvP SHA: $sha"
  echo "CC=$CC"
  echo "CXX=$CXX"

  (
    cd "$WORK_DIR/moos-ivp"
    case "$BUILD_PROFILE" in
      full)
        ./build.sh -mx
        ;;
      headless)
        ./build-moos.sh --minrobot --release
        ./build-ivp.sh --nogui
        ;;
    esac
  )

  write_summary "$sha"
}

main "$@"
