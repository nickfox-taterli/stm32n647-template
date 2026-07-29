#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
# shellcheck source=scripts/cubeclt-common.sh
source "$SCRIPT_DIR/cubeclt-common.sh"

IMAGE_BMP="${IMAGE_BMP:-$ROOT/assets/demo.bmp}"
IMAGE_FLASH_BASE="${IMAGE_FLASH_BASE:-0x71800000}"

require_file "$LOADER"
require_file "$IMAGE_BMP"

IMAGE_BIN="$(mktemp --suffix=.bin)"
trap 'rm -f "$IMAGE_BIN"' EXIT
cp "$IMAGE_BMP" "$IMAGE_BIN"

echo "Flashing static BMP: $IMAGE_BMP -> $IMAGE_FLASH_BASE"
retry_programmer UR -halt -el "$LOADER" -d "$IMAGE_BIN" "$IMAGE_FLASH_BASE" -v
