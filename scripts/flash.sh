#!/usr/bin/env bash
# Program the complete bootable image. The model is deliberately included:
# generated model code and weights must always be updated together.
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"

# Keep the target halted and install the signed boot image last. This avoids
# activating a new FSBL before its matching APP and model are in place.
"$SCRIPT_DIR/flash-model.sh"
"$SCRIPT_DIR/flash-app.sh"
"$SCRIPT_DIR/flash-fsbl.sh"

echo "FSBL, APP and model were programmed and verified."
