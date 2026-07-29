#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
# shellcheck source=scripts/cubeclt-common.sh
source "$SCRIPT_DIR/cubeclt-common.sh"

MODEL_HEX="${MODEL_HEX:-$ROOT/app/AI/Binary/network-data.hex}"

require_file "$LOADER"
require_file "$MODEL_HEX"

echo "Flashing model (address is embedded in HEX): $MODEL_HEX"
retry_programmer UR -halt -el "$LOADER" -d "$MODEL_HEX" -v
