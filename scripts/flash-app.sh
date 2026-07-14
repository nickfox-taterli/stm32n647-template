#!/usr/bin/env bash
set -euo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
CUBECLT_ROOT="${CUBECLT_ROOT:-/opt/st/stm32cubeclt_1.22.0}"
APP_BIN="${APP_BIN:-$ROOT/build/app/app.bin}"
MODEL_HEX="${MODEL_HEX:-$ROOT/app/AI/Binary/network-data.hex}"
PROGRAMMER="$CUBECLT_ROOT/STM32CubeProgrammer/bin/STM32_Programmer_CLI"

"$PROGRAMMER" -c port=SWD mode=UR reset=HWrst freq=4000 \
  -el "$ROOT/loader/ExtMemLoader.stldr" \
  -d "$APP_BIN" 0x70010000 -v
"$PROGRAMMER" -c port=SWD mode=UR reset=HWrst freq=4000 \
  -el "$ROOT/loader/ExtMemLoader.stldr" \
  -d "$MODEL_HEX" -v