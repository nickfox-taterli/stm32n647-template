#!/usr/bin/env bash
set -euo pipefail

ROOT="$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)"
CUBECLT_ROOT="${CUBECLT_ROOT:-/opt/st/stm32cubeclt_1.22.0}"
CMAKE="$CUBECLT_ROOT/CMake/bin/cmake"
NINJA="$CUBECLT_ROOT/Ninja/bin/ninja"
TOOLCHAIN_BIN="$CUBECLT_ROOT/GNU-tools-for-STM32/bin"
TARGET="${1:-all}"

test -x "$CMAKE" || { echo "error: CMake not found: $CMAKE" >&2; exit 1; }
test -x "$NINJA" || { echo "error: Ninja not found: $NINJA" >&2; exit 1; }
test -x "$TOOLCHAIN_BIN/arm-none-eabi-gcc" || {
  echo "error: Arm GCC not found below $TOOLCHAIN_BIN" >&2
  exit 1
}

case "$TARGET" in
  app|fsbl|all) ;;
  *) echo "usage: $0 [app|fsbl|all]" >&2; exit 2 ;;
esac

export PATH="$TOOLCHAIN_BIN:$CUBECLT_ROOT/Ninja/bin:$PATH"

build_target() {
  local name="$1"
  local extra=()
  if test "$name" = fsbl; then
    extra+=("-DCUBECLT_ROOT=$CUBECLT_ROOT")
  fi

  "$CMAKE" -S "$ROOT/$name" -B "$ROOT/build/$name" -G Ninja \
    "-DCMAKE_MAKE_PROGRAM=$NINJA" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    "${extra[@]}"
  "$CMAKE" --build "$ROOT/build/$name" --parallel
}

if test "$TARGET" = app || test "$TARGET" = all; then
  build_target app
fi
if test "$TARGET" = fsbl || test "$TARGET" = all; then
  build_target fsbl
fi
