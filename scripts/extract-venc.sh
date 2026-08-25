#!/usr/bin/env bash
set -euo pipefail

device="${1:-/dev/disk/by-id/usb-STMicro_STM32N6_SD_NAND_N647SDNAND-0:0}"
output="${2:-venc_capture.h264}"

if [[ ! -b "$device" ]]; then
  echo "SD NAND block device not found: $device" >&2
  exit 1
fi

meta="$(mktemp)"
trap 'rm -f "$meta"' EXIT
dd if="$device" of="$meta" bs=512 count=1 iflag=direct status=none

magic="$(dd if="$meta" bs=1 count=7 status=none)"
if [[ "$magic" != "N6VENC1" ]]; then
  echo "No completed VENC recording metadata in LBA0" >&2
  exit 1
fi

read -r version data_lba data_bytes frames width height fps_num fps_den elapsed_ms bitrate \
  < <(od -An -v -tu4 -w40 -j 8 -N 40 "$meta")

if [[ "$version" -ne 1 || "$data_lba" -lt 1 || "$data_bytes" -le 0 ]]; then
  echo "Invalid VENC metadata" >&2
  exit 1
fi

data_blocks=$(((data_bytes + 511) / 512))
dd if="$device" of="$output" bs=512 iflag=direct \
  skip="$data_lba" count="$data_blocks" status=none
truncate -s "$data_bytes" "$output"

echo "Extracted $data_bytes bytes ($frames frames, ${width}x${height}, ${fps_num}/${fps_den} fps, logical $((frames * fps_den / fps_num)) s, encoder ${elapsed_ms} ms, target ${bitrate} bit/s) to $output"
ffprobe -v error -f h264 -framerate "${fps_num}/${fps_den}" -count_frames \
  -show_entries stream=codec_name,profile,width,height,pix_fmt,r_frame_rate,avg_frame_rate,nb_read_frames,duration,bit_rate \
  -of default=noprint_wrappers=1 "$output"
