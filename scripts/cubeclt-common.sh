#!/usr/bin/env bash

# Shared STM32CubeCLT discovery and CubeProgrammer retry helpers.
# This file is sourced by the scripts in this directory.

ROOT="$(CDPATH='' cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
CUBECLT_ROOT="${CUBECLT_ROOT:-/opt/st/stm32cubeclt_1.22.0}"
PROGRAMMER="$CUBECLT_ROOT/STM32CubeProgrammer/bin/STM32_Programmer_CLI"
LOADER="${LOADER:-$ROOT/loader/ExtMemLoader.stldr}"
SWD_FREQ="${SWD_FREQ:-4000}"
PROGRAM_RETRIES="${PROGRAM_RETRIES:-3}"

die() {
  echo "error: $*" >&2
  exit 1
}

require_file() {
  test -f "$1" || die "required file not found: $1"
}

require_executable() {
  test -x "$1" || die "required executable not found: $1"
}

run_programmer() {
  local mode="$1"
  shift
  local active_freq="${ACTIVE_SWD_FREQ:-$SWD_FREQ}"
  local connection=(-c port=SWD "mode=$mode" "freq=$active_freq")

  if test "$mode" = UR; then
    connection+=(reset=HWrst)
  fi

  if test -n "${STLINK_SN:-}"; then
    connection+=("sn=$STLINK_SN")
  fi

  "$PROGRAMMER" "${connection[@]}" "$@"
}

retry_programmer() {
  local mode="$1"
  shift
  local attempt
  local requested_freq="$SWD_FREQ"

  for ((attempt = 1; attempt <= PROGRAM_RETRIES; attempt++)); do
    ACTIVE_SWD_FREQ="$requested_freq"
    if run_programmer "$mode" "$@"; then
      unset ACTIVE_SWD_FREQ
      return 0
    fi
    if ((attempt < PROGRAM_RETRIES)); then
      echo "CubeProgrammer failed (attempt $attempt/$PROGRAM_RETRIES at ${ACTIVE_SWD_FREQ} kHz); retrying..." >&2
      sleep 1
    fi
  done

  unset ACTIVE_SWD_FREQ
  die "CubeProgrammer failed after $PROGRAM_RETRIES attempts"
}

require_executable "$PROGRAMMER"
