#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
# shellcheck source=scripts/cubeclt-common.sh
source "$SCRIPT_DIR/cubeclt-common.sh"

APP_BIN="${APP_BIN:-$ROOT/build/app/app.bin}"

require_file "$LOADER"
require_file "$APP_BIN"

echo "Flashing APP: $APP_BIN -> 0x70010000"
retry_programmer UR -halt -el "$LOADER" -d "$APP_BIN" 0x70010000 -v
