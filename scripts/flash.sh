#!/usr/bin/env bash
# Program the complete bootable image. The model and static test image are
# deliberately included so the APP never runs with stale external assets.
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"

# Keep the target halted and install the signed boot image last. This avoids
# activating a new FSBL before its matching APP and model are in place.
"$SCRIPT_DIR/flash-model.sh"
"$SCRIPT_DIR/flash-image.sh"
"$SCRIPT_DIR/flash-app.sh"
"$SCRIPT_DIR/flash-fsbl.sh"

echo "FSBL, APP, model and static image were programmed and verified."
