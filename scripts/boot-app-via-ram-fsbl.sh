#!/usr/bin/env bash
# Run the FSBL from RAM, stop at JumpToApplication after XSPI2 is mapped,
# then hand off to the XIP application vector table.
set -euo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
CUBECLT_ROOT="${CUBECLT_ROOT:-/opt/st/stm32cubeclt_1.22.0}"
OPENOCD="${OPENOCD:-/root/code/archive/openocd/src/openocd}"
OPENOCD_TCL="${OPENOCD_TCL:-/root/code/archive/openocd/tcl}"
PROGRAMMER="$CUBECLT_ROOT/STM32CubeProgrammer/bin/STM32_Programmer_CLI"
FSBL_ELF="${FSBL_ELF:-$ROOT/build/fsbl/fsbl.elf}"

test -x "$OPENOCD"
test -f "$FSBL_ELF"

# OpenOCD cannot always halt this N647 directly after reset.  CubeProgrammer
# can; the halted state remains while OpenOCD attaches.
"$PROGRAMMER" -c port=SWD mode=UR reset=HWrst freq=4000 -halt

"$OPENOCD" -s "$OPENOCD_TCL" -s "$ROOT/.vscode" \
  -f openocd/stm32n6_target.cfg \
  -c 'init' -c 'halt' \
  -c "load_image $FSBL_ELF" \
  -c 'bp 0x34181ad8 2' \
  -c 'mww 0xE000ED08 0x34180400' \
  -c 'reg msp 0x34200000' -c 'reg pc 0x34180915' \
  -c 'resume' -c 'wait_halt 15000' \
  -c 'set app_sp [mrw 0x70010000]' \
  -c 'set app_pc [mrw 0x70010004]' \
  -c 'echo [format "APP vector verified: SP=0x%08x PC=0x%08x" $app_sp $app_pc]' \
  -c 'rbp 0x34181ad8' \
  -c 'mww 0xE000ED08 0x70010000' \
  -c 'reg msp $app_sp' -c 'reg pc $app_pc' \
  -c 'resume' -c 'shutdown'
