#!/usr/bin/env bash
# JTAG boot mode: load an unsigned FSBL into RAM and run it. The FSBL enables
# XSPI2 memory-mapped mode and performs its normal jump to the Flash APP.
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
# shellcheck source=scripts/cubeclt-common.sh
source "$SCRIPT_DIR/cubeclt-common.sh"

FSBL_BIN="${FSBL_BIN:-$ROOT/build/fsbl/fsbl.bin}"
FSBL_VECTOR="${FSBL_VECTOR:-0x34180400}"
VTOR=0xE000ED08

require_file "$FSBL_BIN"

read -r fsbl_sp fsbl_pc < <(od -An -tx4 -N8 "$FSBL_BIN")
if test -z "$fsbl_sp" || test -z "$fsbl_pc"; then
  die "cannot read FSBL vector"
fi
fsbl_sp="0x$fsbl_sp"
fsbl_pc="0x$fsbl_pc"

((fsbl_sp >= 0x34000000 && fsbl_sp < 0x38000000)) || \
  die "invalid FSBL stack pointer: $fsbl_sp"
((fsbl_pc >= 0x34180401 && fsbl_pc < 0x34200000 && (fsbl_pc & 1) == 1)) || \
  die "invalid FSBL reset vector: $fsbl_pc"

echo "JTAG BOOT: loading RAM FSBL at $FSBL_VECTOR"
echo "FSBL vector verified: SP=$fsbl_sp PC=$fsbl_pc"

# Keep the complete hand-off in one CubeProgrammer session. A halt is required
# after both the RAM download and the VTOR write: otherwise STM32N6 may resume
# its ROM wait loop before MSP/PC are installed, and a later debugger attach
# cannot reliably halt it again.
retry_programmer UR \
  -halt \
  -d "$FSBL_BIN" "$FSBL_VECTOR" -v \
  -halt \
  -w32 "$VTOR" "$FSBL_VECTOR" \
  -halt \
  -coreReg "MSP=$fsbl_sp" "PC=$fsbl_pc" \
  -run

echo "RAM FSBL started; it will map XSPI2 and jump to the Flash APP."
