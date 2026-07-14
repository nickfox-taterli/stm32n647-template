# FSBL → APP 启动与调试记录

本文记录当前 `st_n6` 已验证过的外部 Flash XIP 调试方法。它区分两种路径：

* **正常启动**：由 BOOT 配置启动 FSBL，FSBL 初始化 XSPI2 外部 NOR 后跳转 APP。
* **调试启动**：用 ST-LINK GDB Server 连接 FSBL，在 RAM 中运行/断下 FSBL，确认外部 Flash 已映射后，将 CPU 向量切到 APP。

## 地址和跳转逻辑

当前镜像布局：

```text
FSBL             0x70000000
APP 向量表       0x70010000
模型权重         0x71C00000（NOR 最后 4 MiB）
```

`fsbl/Middlewares/STM32_ExtMem_Manager/boot/stm32_boot_xip.c` 的逻辑是：

1. `NORFlash_EnableMemoryMappedMode()` 将 XSPI2 NOR 映射到 `0x70000000`。
2. `XIP_IMAGE_OFFSET` 加到映射基址，得到 APP 向量表 `0x70010000`。
3. 从 `0x70010000` 读取初始 MSP。
4. 从 `0x70010004` 读取 APP Reset_Handler 地址。
5. 设置 `SCB->VTOR = 0x70010000`、MSP，然后调用 Reset_Handler。

对应源码中的关键操作为：

```c
Application_vector += XIP_IMAGE_OFFSET;
SCB->VTOR = Application_vector;
__set_MSP(*(__IO uint32_t *)Application_vector);
JumpToApp = (pFunction)(*(__IO uint32_t *)(Application_vector + 4u));
JumpToApp();
```

## CubeCLT 构建和烧录

```sh
cd /root/code/atk-n647/st_n6
./scripts/build.sh
./scripts/flash-model.sh
./scripts/flash-app.sh
```

FSBL 使用签名 HEX 烧录：

```sh
/opt/st/stm32cubeclt_1.22.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI \
  -c port=SWD mode=UR reset=HWrst freq=4000 \
  -el loader/ExtMemLoader.stldr \
  -d build/fsbl/fsbl-trusted.hex -v
```

## 正常冷启动

1. BOOT 拨码切到外部 XSPI Flash 启动。
2. 断开主电源/USB，等待完全掉电。
3. 重新上电。
4. 不启动 GDB，不手工写寄存器。

只有这种方式成功，才能证明 FSBL 的真实冷启动交接稳定。此前已经验证了 FSBL、APP、模型三段内容均可由 CubeProgrammer 校验，但正常冷启动交接仍应以板上实测为准。

## 已验证的调试交接方法

当直接对外部 XIP APP 下断点失败时，先调试 FSBL：

2026-07-13 已在本板执行并确认此链路：CubeProgrammer 先将 CPU halt；OpenOCD 装载 `build/fsbl/fsbl.elf` 到 RAM；FSBL 在 `JumpToApplication`（`0x34181ad8`）停止。此时 CPU 已能读取外部 Flash 的 APP 向量：`0x70010000 = 0x34100000`、`0x70010004 = 0x700136cd`。

推荐直接运行已固化的脚本：

```sh
cd /root/code/atk-n647/st_n6
./scripts/boot-app-via-ram-fsbl.sh
```

脚本完成的动作等同于以下调试器命令：

```gdb
VTOR = 0x70010000
MSP  = *(uint32_t *)0x70010000   # 0x34100000
PC   = *(uint32_t *)0x70010004   # 0x700136cd
continue
```

这不是绕过 FSBL：FSBL 先初始化 XSPI2 并完成 memory-map，脚本只在该事实已经由 APP 向量可读证实后，显式完成最后的向量交接。若脚本未打印 `APP vector verified`，不得直接设置 APP 的 PC。

## 当前自动化边界

目前已经自动化：

* CubeCLT CMake/Ninja 构建 APP 和 FSBL；
* CubeProgrammer 通过外部 Flash Loader 烧录 APP 和模型；
* FSBL 的签名镜像生成。

已自动化为“一键 RAM FSBL 调试交接”：`scripts/boot-app-via-ram-fsbl.sh` 会确认映射后的 APP 向量，再切换 VTOR、MSP、PC。

尚未闭环的是**无调试器的冷启动**：BOOT 拨码切到外部 XSPI Flash 后断电再上电，仍需在板上观察画面与模型结果。RAM FSBL 交接成功不能替代这项验收。
