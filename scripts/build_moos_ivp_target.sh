#!/usr/bin/env bash
set -euo pipefail

MOOS_IVP_SOURCE="${MOOS_IVP_SOURCE:-}"
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

prepare_moos_ivp() {
  local checkout_dir
  local source_dir

  if [ -z "$WORK_DIR" ] || [ "$WORK_DIR" = "/" ]; then
    echo "Refusing unsafe WORK_DIR: '$WORK_DIR'" >&2
    exit 1
  fi

  mkdir -p "$WORK_DIR"
  WORK_DIR="$(cd "$WORK_DIR" && pwd -P)"
  checkout_dir="$WORK_DIR/moos-ivp"

  if [ -z "$MOOS_IVP_SOURCE" ] || [ ! -d "$MOOS_IVP_SOURCE" ] || [ ! -f "$MOOS_IVP_SOURCE/build.sh" ]; then
    echo "MOOS_IVP_SOURCE is not a MOOS-IvP checkout: $MOOS_IVP_SOURCE" >&2
    exit 1
  fi
  source_dir="$(cd "$MOOS_IVP_SOURCE" && pwd -P)"
  if [ "$source_dir" = "$checkout_dir" ]; then
    echo "MOOS_IVP_SOURCE must not be the disposable build directory: $checkout_dir" >&2
    exit 1
  fi

  rm -rf -- "$checkout_dir"
  echo "Copying local MOOS-IvP checkout: $source_dir"
  mkdir -p "$checkout_dir"
  cp -a "$source_dir/." "$checkout_dir/"
  rm -rf -- \
    "${checkout_dir:?}/build" \
    "${checkout_dir:?}/bin" \
    "${checkout_dir:?}/include" \
    "${checkout_dir:?}/lib"
}

main() {
  echo "Target: $TARGET_KEY"
  echo "Compiler: $TARGET_COMPILER"
  echo "Build profile: $BUILD_PROFILE"
  echo "MOOS-IvP source: local checkout $MOOS_IVP_SOURCE"
  echo "Work directory: $WORK_DIR"

  validate_build_profile
  install_dependencies
  configure_compiler
  prepare_moos_ivp

  local sha
  sha="$(git -C "$WORK_DIR/moos-ivp" rev-parse HEAD 2>/dev/null || echo local-tree)"
  echo "Resolved MOOS-IvP SHA: $sha"
  if [ -n "$(git -C "$WORK_DIR/moos-ivp" status --short 2>/dev/null || true)" ]; then
    echo "Local MOOS-IvP edits: present (included in this build)"
  fi
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
}

main "$@"
