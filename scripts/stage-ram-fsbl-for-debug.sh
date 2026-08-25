#!/usr/bin/env bash
# Stage the RAM FSBL and leave the Cortex-M55 halted at its Reset_Handler.
# GDB will run the FSBL to its final hand-off instruction under a temporary
# hardware breakpoint, then switch PC to the external-flash APP.
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
# shellcheck source=scripts/cubeclt-common.sh
source "$SCRIPT_DIR/cubeclt-common.sh"

FSBL_BIN="${FSBL_BIN:-$ROOT/build/fsbl/fsbl.bin}"
FSBL_ELF="${FSBL_ELF:-$ROOT/build/fsbl/fsbl.elf}"
FSBL_VECTOR="${FSBL_VECTOR:-0x34180400}"
VTOR=0xE000ED08
NM="$CUBECLT_ROOT/GNU-tools-for-STM32/bin/arm-none-eabi-nm"

require_file "$FSBL_BIN"
require_file "$FSBL_ELF"
require_executable "$NM"

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

handoff_address="$($NM -n "$FSBL_ELF" | awk '$3 == "FSBL_DebugHandoff" { print "0x" $1; exit }')"
test -n "$handoff_address" || die "FSBL_DebugHandoff symbol not found in $FSBL_ELF"
entry_address="$(printf '0x%08x' "$((fsbl_pc & ~1))")"

echo "Staging RAM FSBL for GDB: SP=$fsbl_sp PC=$fsbl_pc"
echo "RAM-only entry hold: $entry_address; final hand-off: $handoff_address"

# Keep this in one under-reset CubeProgrammer session.  Patch only the RAM
# copy's Reset_Handler entry to Thumb `b .` (0xE7FE), then run.  This stable
# loop keeps debug access alive until GDB performs a real EXC_RETURN into
# Thread mode, restores the FSBL image and runs it to the final hand-off.
retry_programmer UR \
  -halt \
  -d "$FSBL_BIN" "$FSBL_VECTOR" -v \
  -halt \
  -w16 "$entry_address" 0xE7FE \
  -halt \
  -w32 "$VTOR" "$FSBL_VECTOR" \
  -halt \
  -coreReg "XPSR=0x01000000" "CONTROL=0" "PRIMASK=0" \
           "MSP=$fsbl_sp" "PC=$fsbl_pc" \
  -run

echo "RAM FSBL is waiting at its entry hold for GDB."
