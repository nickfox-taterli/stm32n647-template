# VENC 录像与导出

当前首通配置直接编码 DCMIPP Pipe1 的 RGB565 预览缓冲区，不启用 Pipe0，
也不改变 Pipe2/NPU 路径：

- H.264 Annex-B byte stream，Constrained Baseline
- 512x300，5 fps，50 帧（逻辑时长 10 秒）
- 目标码率 2 Mbit/s，GOP 5
- VENC 通过 LL/寄存器和轮询驱动；SDMMC 复用工程现有的最小 HAL 数据通路

## 串口触发

CH340 `1a86:7523` 是 `/dev/ttyUSB0` 串口，不是存储设备。串口参数为
115200/8N1。在 shell 中执行：

```text
venc status
venc record
venc status
```

`venc record` 会覆盖 SD NAND 的 LBA0，从而破坏已有 MBR/FAT。码流从 LBA1
开始连续写入；只有编码和落盘全部成功后，LBA0 才写入 `N6VENC1` 完成标记
及码流长度、帧数、分辨率、帧率和编码耗时等元数据。

## USB 导出与检查

APP 的 TinyUSB MSC 设备 ID 为 `0483:5721`，默认稳定设备路径是：

```text
/dev/disk/by-id/usb-STMicro_STM32N6_SD_NAND_N647SDNAND-0:0
```

导出脚本按元数据读取精确字节数，并自动运行 `ffprobe`：

```sh
./scripts/extract-venc.sh \
  /dev/disk/by-id/usb-STMicro_STM32N6_SD_NAND_N647SDNAND-0:0 \
  venc_capture.h264
```

裸 H.264 不携带容器时间戳，因此脚本显式把元数据帧率传给 `ffprobe`。
需要额外做完整解码检查时执行：

```sh
ffmpeg -v error -framerate 5 -i venc_capture.h264 -f null -
```

若要重新使用 FAT，需要在主机上重新分区并格式化整块 SD NAND；该操作会
删除当前裸码流和元数据。
