# STM32N6 烧录、启动与 FSBL → APP 交接

当前镜像布局：

```text
签名 FSBL        0x70000000
APP 向量表       0x70010000
模型权重         0x71C00000（地址已写入 network-data.hex）
RAM FSBL 向量表  0x34180400（仅 JTAG BOOT）
```

FSBL 先调用 `NORFlash_EnableMemoryMappedMode()` 把 XSPI2 映射到
`0x70000000`，再从 `0x70010000` 读取 APP 的 MSP 和 Reset_Handler，设置
`VTOR` 后跳转。因此，XSPI2 尚未映射时不能直接把 PC 设置到 APP。

## 构建和烧录

唯一依赖是 STM32CubeCLT，默认位置为
`/opt/st/stm32cubeclt_1.22.0`，可用 `CUBECLT_ROOT` 覆盖。

```sh
./scripts/build.sh all
./scripts/flash.sh
```

`flash.sh` 依次烧录并校验模型、APP、签名 FSBL，整个过程保持 CPU 停止，
并将签名 FSBL 放在最后写入。模型代码或权重变化时必须使用这个完整任务，
避免 APP 与权重版本不匹配。也可以单独执行：

```sh
./scripts/flash-app.sh
./scripts/flash-fsbl.sh
./scripts/flash-model.sh
```

烧录使用 CubeProgrammer、`loader/ExtMemLoader.stldr`、under-reset 连接和
写后校验；瞬时连接失败默认重试三次。可设置 `STLINK_SN` 选择探针、
`SWD_FREQ` 修改频率、`PROGRAM_RETRIES` 修改重试次数。

## 两种运行方式

### JTAG BOOT：RAM FSBL → Flash APP

BOOT 拨码为 JTAG BOOT 时执行：

```sh
./scripts/boot-app-via-ram-fsbl.sh
```

脚本从 `build/fsbl/fsbl.bin` 动态读取并校验 MSP/Reset_Handler，在一次
CubeProgrammer under-reset 会话内完成 RAM 下载与校验、设置 VTOR/MSP/PC，
最后放行内核。FSBL 随后映射 XSPI2 并自行跳到 `0x70010000`。这个运行路径
不创建 GDB Server 会话，也不使用固定延时或硬编码的 Reset_Handler 地址。
若此前的调试会话把目标留在 `Rev Z` / `DEV_AP_ACCESS_ERROR` 状态，应先让
板卡完全断电再上电；仅重复 SWD 复位不能保证恢复该状态。

### Normal BOOT：直接复位运行

BOOT 拨码为正常外部 Flash 启动时执行：

```sh
./scripts/run-normal.sh
```

这个路径只脉冲硬件复位并放行 CPU，不向 RAM 重装 FSBL。ROM 从外部
Flash 启动签名 FSBL，FSBL 再映射并跳转 APP。这也是验证真实冷启动交接
的路径；最终仍应做一次完全断电再上电测试。

## VS Code Tasks

`Terminal` → `Run Task` 中提供：

- `Flash APP`、`Flash FSBL`、`Flash Model`
- `Flash All (FSBL + APP + Model)`
- `Run APP (JTAG BOOT: RAM FSBL -> Flash)`
- `Run APP (Normal BOOT: Reset Only)`

烧录任务不会自动运行，运行任务也不会隐式擦写 Flash，可根据板上 BOOT
模式明确选择。所有构建产物统一位于 `build/app` 和 `build/fsbl`。
