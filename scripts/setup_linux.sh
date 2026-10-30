#!/usr/bin/env bash
#
# setup_linux.sh: one-shot environment setup for partition-learning on Linux.
#
# What it does:
#   1. Checks for git, a C++20 compiler, make, cmake (>= 3.16), python3 + venv.
#   2. Installs anything missing via the system package manager (apt, dnf, yum,
#      pacman, zypper) if you have root/sudo. Without root (e.g. a cluster
#      login node) it skips this and tells you what is missing.
#   3. Verifies the compiler really supports C++20 and (optionally) OpenMP.
#   4. Creates a Python venv (.venv) and installs requirements.txt.
#      If the system cmake is too old, installs a recent one into the venv.
#   5. Optionally builds the project and runs the tests (--build).
#
# This script only sets up the environment for a checkout of the repo. It does
# not create or modify any tracked files or directories.
#
# Usage (from anywhere inside the repo):
#   bash scripts/setup_linux.sh [--build] [--no-system-install] [--skip-python]
#
# Environment:
#   CXX   compiler to use/check (default: g++). Example: CXX=clang++
#
# Safe to re-run: every step is skipped if already satisfied.

set -euo pipefail

MIN_CMAKE="3.16"
DO_BUILD=0
SYSTEM_INSTALL=1
SKIP_PYTHON=0

usage() { sed -n '2,26p' "$0" | sed 's/^# \{0,1\}//'; }

for arg in "$@"; do
  case "$arg" in
  --build) DO_BUILD=1 ;;
  --no-system-install) SYSTEM_INSTALL=0 ;;
  --skip-python) SKIP_PYTHON=1 ;;
  -h | --help)
    usage
    exit 0
    ;;
  *)
    echo "Unknown option: $arg" >&2
    usage
    exit 1
    ;;
  esac
done

# ---------- helpers ----------
if [ -t 1 ]; then
  B=$'\033[1m'
  G=$'\033[32m'
  Y=$'\033[33m'
  R=$'\033[31m'
  N=$'\033[0m'
else
  B=""
  G=""
  Y=""
  R=""
  N=""
fi
step() { echo "${B}==> $*${N}"; }
ok() { echo "  ${G}ok${N}   $*"; }
warn() { echo "  ${Y}warn${N} $*"; }
die() {
  echo "  ${R}fail${N} $*" >&2
  exit 1
}
have() { command -v "$1" >/dev/null 2>&1; }

# true if $1 >= $2 (version strings, e.g. ver_ge 3.22.1 3.16)
ver_ge() { [ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n1)" = "$2" ]; }

# ---------- locate repo root ----------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"
step "Repository root: $REPO_ROOT"

# ---------- OS check ----------
if [ "$(uname -s)" != "Linux" ]; then
  die "This script is for Linux. On macOS use: xcode-select --install && brew install cmake libomp (see README)."
fi

CXX_BIN="${CXX:-g++}"
# $CXX can be inherited from a module/spack environment but point at a path that
# does not exist here (e.g. inside a container). Fall back to a compiler on PATH.
if ! have "$CXX_BIN"; then
  for fallback in g++ c++ clang++; do
    if have "$fallback"; then
      warn "CXX='$CXX_BIN' not found; using '$fallback' from PATH instead."
      CXX_BIN="$fallback"
      break
    fi
  done
fi
TMPDIR_SETUP="$(mktemp -d)"
trap 'rm -rf "$TMPDIR_SETUP"' EXIT

# ---------- detect package manager / privileges ----------
PM=""
for candidate in apt-get dnf yum pacman zypper; do
  if have "$candidate"; then
    PM="$candidate"
    break
  fi
done

SUDO=""
CAN_INSTALL=0
if [ "$SYSTEM_INSTALL" -eq 1 ] && [ -n "$PM" ]; then
  if [ "$(id -u)" -eq 0 ]; then
    CAN_INSTALL=1
  elif have sudo; then
    SUDO="sudo"
    CAN_INSTALL=1
  fi
fi

# ---------- figure out what is missing ----------
step "Checking system tools"
MISSING=() # logical names: git compiler make cmake python venv

have git || MISSING+=(git)
have "$CXX_BIN" || MISSING+=(compiler)
have make || MISSING+=(make)
have python3 || MISSING+=(python)
if have python3 && ! python3 -c "import venv, ensurepip" >/dev/null 2>&1; then
  MISSING+=(venv)
fi
if have cmake; then
  CM_VER="$(cmake --version | head -n1 | awk '{print $3}')"
  if ! ver_ge "$CM_VER" "$MIN_CMAKE"; then MISSING+=(cmake); fi
else
  MISSING+=(cmake)
fi

# map logical names to package names for each package manager
pkgs_for() {
  local name="$1"
  case "$PM:$name" in
  apt-get:git) echo git ;;
  apt-get:compiler | apt-get:make) echo build-essential ;;
  apt-get:cmake) echo cmake ;;
  apt-get:python) echo "python3 python3-pip python3-venv" ;;
  apt-get:venv) echo "python3-venv" ;;

  dnf:git | yum:git) echo git ;;
  dnf:compiler | yum:compiler) echo gcc-c++ ;;
  dnf:make | yum:make) echo make ;;
  dnf:cmake | yum:cmake) echo cmake ;;
  dnf:python | yum:python) echo "python3 python3-pip" ;;
  dnf:venv | yum:venv) echo "python3" ;;

  pacman:git) echo git ;;
  pacman:compiler | pacman:make) echo base-devel ;;
  pacman:cmake) echo cmake ;;
  pacman:python | pacman:venv) echo "python python-pip" ;;

  zypper:git) echo git ;;
  zypper:compiler) echo gcc-c++ ;;
  zypper:make) echo make ;;
  zypper:cmake) echo cmake ;;
  zypper:python) echo "python3 python3-pip" ;;
  zypper:venv) echo "python3-virtualenv" ;;
  esac
}

if [ "${#MISSING[@]}" -eq 0 ]; then
  ok "git, $CXX_BIN, make, cmake, python3 all present"
else
  warn "missing or too old: ${MISSING[*]}"
  if [ "$CAN_INSTALL" -eq 1 ]; then
    PKGS=()
    for m in "${MISSING[@]}"; do
      for p in $(pkgs_for "$m"); do PKGS+=("$p"); done
    done
    # de-duplicate
    mapfile -t PKGS < <(printf '%s\n' "${PKGS[@]}" | awk '!seen[$0]++')
    step "Installing via $PM: ${PKGS[*]}"
    case "$PM" in
    apt-get) $SUDO apt-get update && $SUDO apt-get install -y "${PKGS[@]}" ;;
    dnf) $SUDO dnf install -y "${PKGS[@]}" ;;
    yum) $SUDO yum install -y "${PKGS[@]}" ;;
    pacman) $SUDO pacman -Sy --needed --noconfirm "${PKGS[@]}" ;;
    zypper) $SUDO zypper --non-interactive install "${PKGS[@]}" ;;
    esac
  else
    warn "cannot install system packages (no root/sudo, no supported package manager, or --no-system-install)."
    warn "On a cluster such as MIT Engaging, load modules first, e.g.:"
    warn "    module avail gcc cmake python   # check exact names"
    warn "    module load gcc cmake           # then re-run this script"
  fi
fi

# ---------- hard requirements ----------
have git || die "git not found."
have make || die "make not found."
have "$CXX_BIN" || die "$CXX_BIN not found. Install a C++ compiler (or 'module load gcc')."
have python3 || die "python3 not found."

# ---------- verify C++20 actually works ----------
step "Verifying C++20 support ($CXX_BIN)"
cat >"$TMPDIR_SETUP/cxx20.cpp" <<'EOF'
#include <bit>
#include <concepts>
#include <span>
#include <vector>
template <std::integral T> int f(T x) { return std::popcount(static_cast<unsigned>(x)); }
int main() { std::vector<int> v{1, 2, 3}; std::span<int> s(v); return f(7) == 3 && s.size() == 3 ? 0 : 1; }
EOF
if "$CXX_BIN" -std=c++20 "$TMPDIR_SETUP/cxx20.cpp" -o "$TMPDIR_SETUP/cxx20" 2>"$TMPDIR_SETUP/cxx20.err" &&
  "$TMPDIR_SETUP/cxx20"; then
  ok "$("$CXX_BIN" --version | head -n1)"
else
  cat "$TMPDIR_SETUP/cxx20.err" >&2 || true
  die "$CXX_BIN does not support C++20. Need GCC >= 10 or Clang >= 12 (try 'module load gcc' on a cluster)."
fi

# ---------- verify OpenMP (optional) ----------
step "Checking OpenMP (optional)"
cat >"$TMPDIR_SETUP/omp.cpp" <<'EOF'
#include <omp.h>
int main() { int n = 0;
#pragma omp parallel
  {
#pragma omp atomic
    n++;
  }
  return n >= 1 ? 0 : 1; }
EOF
if "$CXX_BIN" -std=c++20 -fopenmp "$TMPDIR_SETUP/omp.cpp" -o "$TMPDIR_SETUP/omp" >/dev/null 2>&1 &&
  "$TMPDIR_SETUP/omp"; then
  ok "OpenMP works"
else
  warn "OpenMP not available; the project will build single-threaded."
  warn "Fix: install libgomp (comes with GCC) or libomp-dev for Clang."
fi

# ---------- Python environment ----------
if [ "$SKIP_PYTHON" -eq 0 ]; then
  step "Setting up Python virtual environment (.venv)"
  if [ ! -d .venv ]; then
    python3 -m venv .venv || die "venv creation failed (on Debian/Ubuntu: sudo apt install python3-venv)."
  fi
  # shellcheck disable=SC1091
  source .venv/bin/activate
  python -m pip install --quiet --upgrade pip
  [ -f requirements.txt ] || die "requirements.txt not found in $REPO_ROOT (did you pull the latest commit?)."
  python -m pip install --quiet -r requirements.txt
  ok "installed: $(python -m pip list --format=freeze | grep -iE '^(numpy|pandas|matplotlib|scipy)==' | tr '\n' ' ')"
else
  warn "skipping Python setup (--skip-python)"
fi

# ---------- CMake fallback via pip ----------
CM_OK=0
if have cmake; then
  CM_VER="$(cmake --version | head -n1 | awk '{print $3}')"
  ver_ge "$CM_VER" "$MIN_CMAKE" && CM_OK=1
fi
if [ "$CM_OK" -eq 0 ]; then
  if [ "$SKIP_PYTHON" -eq 0 ]; then
    step "System cmake missing or older than $MIN_CMAKE; installing cmake into .venv"
    python -m pip install --quiet cmake
    hash -r
    ok "cmake $(cmake --version | head -n1 | awk '{print $3}') (from .venv)"
    warn "Run 'source .venv/bin/activate' before building so this cmake is used."
  else
    die "cmake >= $MIN_CMAKE required (rerun without --skip-python to get one via pip)."
  fi
else
  ok "cmake $CM_VER"
fi

# ---------- optional build + test ----------
if [ "$DO_BUILD" -eq 1 ]; then
  step "Building release configuration"
  # Catch2 is downloaded by CMake FetchContent, so this step needs internet access.
  # On clusters, do this on the login node, not inside a compute job.
  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$(command -v "$CXX_BIN")"
  cmake --build build-release -j "$(nproc 2>/dev/null || echo 2)"
  if [ -x build-release/unit_tests ]; then
    step "Running unit tests"
    ./build-release/unit_tests
  fi
  if [ -x build-release/partition_experiment ]; then
    step "Running smoke executable"
    ./build-release/partition_experiment
  fi
fi

step "Setup complete"
echo "Next steps:"
[ "$SKIP_PYTHON" -eq 0 ] && echo "  source .venv/bin/activate"
if [ "$DO_BUILD" -eq 0 ]; then
  echo "  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release"
  echo "  cmake --build build-release -j"
  echo "  ./build-release/unit_tests"
fi
