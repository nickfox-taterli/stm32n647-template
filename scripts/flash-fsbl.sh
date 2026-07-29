#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
# shellcheck source=scripts/cubeclt-common.sh
source "$SCRIPT_DIR/cubeclt-common.sh"

FSBL_HEX="${FSBL_HEX:-$ROOT/build/fsbl/fsbl-trusted.hex}"

require_file "$LOADER"
require_file "$FSBL_HEX"

echo "Flashing signed FSBL: $FSBL_HEX"
retry_programmer UR -halt -el "$LOADER" -d "$FSBL_HEX" -v
