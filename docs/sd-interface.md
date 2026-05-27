# SD 子系统接口文档

## Shell 命令

所有 SD 操作通过单个 `sd` 命令完成，子命令风格与 U-Boot `mmc` 命令一致。

### sd info [1|2]

显示指定设备的卡信息。不传参数时显示当前默认设备。

输出包含：Manufacturer ID、OEM ID、产品名、版本、序列号、生产日期、
容量、块大小、块数量、RCA、总线宽度、Card Type。

```
> sd info
> sd info 2
```

### sd read dev blk [cnt]

从设备 `dev`（1=SDMMC1/SD Card, 2=SDMMC2/SD NAND）的块地址 `blk` 开始读取 `cnt` 个块（默认 1）。
十六进制地址需加 `0x` 前缀。只 hexdump 第一个块的内容。

```
> sd read 1 0x1000           # 从 SD Card 读 1 块
> sd read 2 0x1000 4         # 从 SD NAND 读 4 块
```

### sd write dev blk [cnt]

向设备 `dev` 的块地址 `blk` 写入 `cnt` 个块（默认 1）的测试模式，
然后读回校验。每个块打印 PASS/FAIL。

测试模式：每个字节为 `(块地址 ^ 字节索引)`。

```
> sd write 1 0x1000          # 写 1 块到 SD Card 并校验
> sd write 2 0x2000 2        # 写 2 块到 SD NAND 并校验
```

### sd dev [1|2]

设置或查看当前默认设备。不传参数时仅显示当前设备。

```
> sd dev                      # 查看当前设备
> sd dev 2                    # 切换到 SDMMC2
```

## 编程接口

```c
/* 检查设备是否已初始化就绪（参数: 1 或 2） */
int sd_is_ready(int dev);

/* 开机自动初始化任务（由 main.c 创建为 FreeRTOS task） */
void SD_InitTask(void *argument);

/* sd 命令入口（已通过 SHELL_EXPORT_CMD 注册） */
int sd_cmd(int argc, char **argv);
```

## 数值格式

所有数值参数支持：
- 十进制：`4096`
- 十六进制（`0x` 前缀）：`0x1000`

内部使用 `strtoul(str, NULL, 0)` 解析，与 U-Boot 行为一致。

## 错误处理

| 场景 | 输出 |
|------|------|
| 设备未初始化 | `SDMMCx not initialized` |
| 无效设备号 | `Invalid device: use 1 or 2` |
| 参数不足 | 打印 usage |
| HAL 错误 | `FAIL: ret=X Err=0xXXXXXXXX` |
