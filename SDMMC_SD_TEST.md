# STM32N6 SDMMC SD Test

This document covers the local SDMMC smoke/regression tests for:

- SD Card on SDMMC1
- SD NAND on SDMMC2
- polling, interrupt, and SDMMC internal DMA/IDMA transfer paths

## Scope

The app intentionally uses only these HAL/LL SD-related driver sources:

- `stm32n6xx_hal_sd.c`
- `stm32n6xx_ll_sdmmc.c`
- `stm32n6xx_ll_dlyb.c`

GPIO, RCC, NVIC, RIF, PWR/VDDIO, and interrupt priority setup stay in local LL/register code.

## Build And Flash

From repo root:

```sh
cmake --build app/build
/root/code/openocd/src/openocd \
  -s /root/code/openocd/tcl \
  -s /root/code/st_n6/.vscode \
  -f /root/code/st_n6/.vscode/openocd/flash_app.cfg
```

If the board does not boot the app automatically after flashing, start the FSBL from RAM for test:

```sh
/root/code/openocd/src/openocd \
  -s /root/code/openocd/tcl \
  -s /root/code/st_n6/.vscode \
  -f /root/code/st_n6/.vscode/openocd/boot_app_via_fsbl.cfg
```

Open the serial shell:

```sh
picocom -b 115200 /dev/ttyUSB0
```

## Destructive Blocks

These tests write one 512-byte block per path:

- polling: block `0x1000`
- interrupt: block `0x1001`
- DMA/IDMA: block `0x1002`

Run only on media where these blocks may be overwritten.

## Commands

Run all tests:

```text
sd_all
```

Or run each path separately:

```text
sd_card
sd_card_it
sd_card_dma
sd_nand
sd_nand_it
sd_nand_dma
```

Optional register probe:

```text
sd_probe
```

## Expected Results

Each command should end with `Return: 0, 0x00000000` and a PASS line:

```text
[PASS] SD-Card blk 0x1000 OK
[PASS] SD-Card IT blk 0x1001 OK
[PASS] SD-Card DMA blk 0x1002 OK
[PASS] SD-NAND blk 0x1000 OK
[PASS] SD-NAND IT blk 0x1001 OK
[PASS] SD-NAND DMA blk 0x1002 OK
```

The DMA path is SDMMC internal DMA/IDMA. No external DMA HAL module is required.

## VSCode APP Debug Note

`Debug APP (External Flash)` builds FSBL and APP, flashes APP, then starts the RAM FSBL from OpenOCD before halting the target. It intentionally does not set an early temporary breakpoint at `main`, because APP code in external Flash is not safely readable until FSBL has configured the external memory mapping.

If the previous debug session used the old launch sequence and left OpenOCD failing at `Failed to read memory at 0x70011cd4`, power-cycle the board once before using the updated launch config.

## Known Local Result

Verified on `/dev/ttyUSB0` at 115200 baud after flashing `app/build/app.bin`:

- SDMMC1 SD Card polling: PASS
- SDMMC1 SD Card interrupt: PASS
- SDMMC1 SD Card IDMA: PASS
- SDMMC2 SD NAND polling: PASS
- SDMMC2 SD NAND interrupt: PASS
- SDMMC2 SD NAND IDMA: PASS
