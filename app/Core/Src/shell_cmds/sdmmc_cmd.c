#include "sdmmc_cmd.h"
#include "sdmmc_drv.h"
#include "stm32n6xx_hal.h"
#include "shell.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define P(fmt, ...) shellPrint(shellGetCurrent(), fmt, ##__VA_ARGS__)

static int sd_curr_dev = 1;

/* ================================================================
 *  Hexdump
 * ================================================================ */

static void sd_hexdump(const uint8_t *data, uint32_t len)
{
    for (uint32_t off = 0; off < len; off += 16) {
        P("%04lX: ", (unsigned long)off);
        for (int i = 0; i < 16; i++) {
            if (i == 8) P(" ");
            P("%02X", data[off + i]);
        }
        P("  |");
        for (int i = 0; i < 16; i++) {
            uint8_t c = data[off + i];
            P("%c", (c >= 0x20 && c <= 0x7E) ? c : '.');
        }
        P("|\r\n");
    }
}

/* ================================================================
 *  Subcommand handlers
 * ================================================================ */

static int sd_cmd_info(int argc, char **argv)
{
    int dev = sd_curr_dev;
    if (argc >= 2) dev = (int)strtoul(argv[1], NULL, 0);

    if (dev != 1 && dev != 2) {
        P("Invalid device: use 1 or 2\r\n");
        return -1;
    }

    SD_HandleTypeDef *hsd = sd_get_handle(dev);
    if (!hsd) {
        P("SDMMC%d not initialized\r\n", dev);
        return -1;
    }

    sd_print_info(hsd, dev);
    return 0;
}

static int sd_cmd_read(int argc, char **argv)
{
    if (argc < 3) {
        P("Usage: sd read dev blk [cnt]\r\n");
        return -1;
    }

    int  dev = (int)strtoul(argv[1], NULL, 0);
    uint32_t blk = strtoul(argv[2], NULL, 0);
    uint32_t cnt = (argc >= 4) ? strtoul(argv[3], NULL, 0) : 1;

    if (dev != 1 && dev != 2) {
        P("Invalid device: use 1 or 2\r\n");
        return -1;
    }
    if (cnt == 0) cnt = 1;

    SD_HandleTypeDef *hsd = sd_get_handle(dev);
    if (!hsd) {
        P("SDMMC%d not initialized\r\n", dev);
        return -1;
    }

    ALIGN_32BYTES(uint8_t buf[512]);
    HAL_StatusTypeDef ret;
    uint32_t ok_cnt = 0;

    for (uint32_t i = 0; i < cnt; i++) {
        ret = HAL_SD_ReadBlocks(hsd, buf, blk + i, 1, 5000);
        if (ret != HAL_OK) {
            P("SD read: dev # %d, block # 0x%lX: FAIL ret=%d Err=0x%08lX\r\n",
              dev, (unsigned long)(blk + i), ret, (unsigned long)hsd->ErrorCode);
            break;
        }
        if (sd_wait_card_transfer(hsd, 5000) != 0) break;
        ok_cnt++;

        if (i == 0) {
            sd_hexdump(buf, 512);
        }
    }

    if (cnt > 1 && ok_cnt > 1)
        P("... %lu blocks not shown\r\n", (unsigned long)(ok_cnt - 1));

    P("SD read: dev # %d, block # 0x%lX, count %lu: %s\r\n",
      dev, (unsigned long)blk, (unsigned long)cnt,
      (ok_cnt == cnt) ? "OK" : "ERROR");

    return (ok_cnt == cnt) ? 0 : -1;
}

static int sd_cmd_write(int argc, char **argv)
{
    if (argc < 3) {
        P("Usage: sd write dev blk [cnt]\r\n");
        return -1;
    }

    int  dev = (int)strtoul(argv[1], NULL, 0);
    uint32_t blk = strtoul(argv[2], NULL, 0);
    uint32_t cnt = (argc >= 4) ? strtoul(argv[3], NULL, 0) : 1;

    if (dev != 1 && dev != 2) {
        P("Invalid device: use 1 or 2\r\n");
        return -1;
    }
    if (cnt == 0) cnt = 1;

    SD_HandleTypeDef *hsd = sd_get_handle(dev);
    if (!hsd) {
        P("SDMMC%d not initialized\r\n", dev);
        return -1;
    }

    ALIGN_32BYTES(uint8_t tx[512]);
    ALIGN_32BYTES(uint8_t rx[512]);
    HAL_StatusTypeDef ret;
    int pass = 0, fail = 0;

    for (uint32_t i = 0; i < cnt; i++) {
        uint32_t b = blk + i;
        for (int j = 0; j < 512; j++) tx[j] = (uint8_t)(j ^ (b & 0xFF));

        ret = HAL_SD_WriteBlocks(hsd, tx, b, 1, 5000);
        if (ret != HAL_OK) {
            P("  blk 0x%lX: WRITE FAIL ret=%d\r\n", (unsigned long)b, ret);
            fail++;
            continue;
        }
        if (sd_wait_card_transfer(hsd, 5000) != 0) { fail++; continue; }

        ret = HAL_SD_ReadBlocks(hsd, rx, b, 1, 5000);
        if (ret != HAL_OK) {
            P("  blk 0x%lX: READBACK FAIL ret=%d\r\n", (unsigned long)b, ret);
            fail++;
            continue;
        }
        if (sd_wait_card_transfer(hsd, 5000) != 0) { fail++; continue; }

        if (memcmp(tx, rx, 512) == 0) {
            P("  blk 0x%lX: PASS\r\n", (unsigned long)b);
            pass++;
        } else {
            P("  blk 0x%lX: FAIL (mismatch)\r\n", (unsigned long)b);
            fail++;
        }
    }

    P("SD write: dev # %d, block # 0x%lX, count %lu: %d pass, %d fail\r\n",
      dev, (unsigned long)blk, (unsigned long)cnt, pass, fail);

    return (fail == 0) ? 0 : -1;
}

static int sd_cmd_dev(int argc, char **argv)
{
    if (argc < 2) {
        P("Current device: SDMMC%d (%s)\r\n", sd_curr_dev,
          sd_curr_dev == 1 ? "SD Card" : "SD NAND");
        return 0;
    }

    int dev = (int)strtoul(argv[1], NULL, 0);
    if (dev != 1 && dev != 2) {
        P("Invalid device: use 1 or 2\r\n");
        return -1;
    }

    sd_curr_dev = dev;
    P("Switched to SDMMC%d (%s, ready=%d)\r\n", dev,
      dev == 1 ? "SD Card" : "SD NAND", sd_is_ready(dev));
    return 0;
}

/* ================================================================
 *  Subcommand dispatch
 * ================================================================ */

typedef struct {
    const char *name;
    int (*fn)(int argc, char **argv);
    const char *usage;
} sd_subcmd_t;

static const sd_subcmd_t sd_subcmds[] = {
    { "info",  sd_cmd_info,  "sd info [1|2]" },
    { "read",  sd_cmd_read,  "sd read dev blk [cnt]" },
    { "write", sd_cmd_write, "sd write dev blk [cnt]" },
    { "dev",   sd_cmd_dev,   "sd dev [1|2]" },
    { NULL,    NULL,         NULL },
};

int sd_cmd(int argc, char **argv)
{
    if (argc < 2) {
        P("Usage:\r\n");
        for (const sd_subcmd_t *c = sd_subcmds; c->name; c++)
            P("  %s\r\n", c->usage);
        return 0;
    }

    const char *sub = argv[1];

    for (const sd_subcmd_t *c = sd_subcmds; c->name; c++) {
        if (strcmp(sub, c->name) == 0)
            return c->fn(argc - 1, &argv[1]);
    }

    P("Unknown subcommand: %s\r\n", sub);
    P("Usage:\r\n");
    for (const sd_subcmd_t *c = sd_subcmds; c->name; c++)
        P("  %s\r\n", c->usage);
    return -1;
}

SHELL_EXPORT_CMD(SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), sd, sd_cmd, SD subsystem: info|read|write|dev);
