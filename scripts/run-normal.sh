#!/usr/bin/env bash
# Normal boot mode: pulse NRST and let the ROM/FSBL boot path run unchanged.
set -euo pipefail

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
# shellcheck source=scripts/cubeclt-common.sh
source "$SCRIPT_DIR/cubeclt-common.sh"

echo "Normal BOOT: pulse the hardware reset; the target runs on release."
echo "The board BOOT switches must select the normal external-Flash boot path."
retry_programmer UR -hardRst
