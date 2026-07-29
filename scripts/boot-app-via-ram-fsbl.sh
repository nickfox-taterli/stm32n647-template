#!/usr/bin/env bash
# JTAG boot mode: load an unsigned FSBL into RAM and run it. The FSBL enables
# XSPI2 memory-mapped mode and performs its normal jump to the Flash APP.
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
# shellcheck source=scripts/cubeclt-common.sh
source "$SCRIPT_DIR/cubeclt-common.sh"

FSBL_BIN="${FSBL_BIN:-$ROOT/build/fsbl/fsbl.bin}"
FSBL_ELF="${FSBL_ELF:-$ROOT/build/fsbl/fsbl.elf}"
FSBL_VECTOR="${FSBL_VECTOR:-0x34180400}"
GDB_SERVER="$CUBECLT_ROOT/STLink-gdb-server/bin/ST-LINK_gdbserver"
GDB="$CUBECLT_ROOT/GNU-tools-for-STM32/bin/arm-none-eabi-gdb"
GDB_COMMANDS="$SCRIPT_DIR/boot-app-via-ram-fsbl.gdb"
GDB_PORT="${GDB_PORT:-61234}"

require_file "$FSBL_BIN"
require_file "$FSBL_ELF"
require_file "$GDB_COMMANDS"
require_executable "$GDB_SERVER"
require_executable "$GDB"

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

# CubeProgrammer is the reliable under-reset loader for this target. It leaves
# the core halted; the CubeCLT GDB server then attaches without another reset.
retry_programmer UR -halt -d "$FSBL_BIN" "$FSBL_VECTOR" -v

server_log="$(mktemp)"
server_pid=""
cleanup() {
  if test -n "$server_pid" && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
  rm -f "$server_log"
}
trap cleanup EXIT INT TERM

"$GDB_SERVER" \
  -p "$GDB_PORT" \
  -cp "$CUBECLT_ROOT/STM32CubeProgrammer/bin" \
  -g --halt -d --frequency "$SWD_FREQ" \
  -f "$server_log" &
server_pid=$!

# Give the attaching server a bounded startup window. If it exits early, show
# its diagnostic log instead of letting GDB fail with an opaque timeout.
for _ in 1 2 3 4 5; do
  kill -0 "$server_pid" 2>/dev/null || {
    test -s "$server_log" && sed -n '1,200p' "$server_log" >&2
    die "ST-LINK GDB server failed to attach"
  }
  sleep 0.2
done

"$GDB" --batch "$FSBL_ELF" \
  -ex "target remote 127.0.0.1:$GDB_PORT" \
  -x "$GDB_COMMANDS"

wait "$server_pid" || {
  test -s "$server_log" && sed -n '1,200p' "$server_log" >&2
  die "ST-LINK GDB server exited with an error"
}
server_pid=""

echo "RAM FSBL mapped XSPI2 and resumed its jump to the Flash APP."
