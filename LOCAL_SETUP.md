# STM32N6 本机开发环境 (Local Dev Setup)

本机 (aarch64 Debian) 上编译 / 下载 / 调试 STM32N647 工程的完整流程。
工程目录：`/root/code/atk-n647/st_n6`（`.cfg` 里的绝对路径已直接指向该目录，不再需要软链）。

> 已验证全链路：编译 → 下载 APP 到外部 Flash → gdb-multiarch 调试 → 串口 Shell，
> 运行的就是本机编译的镜像（串口 Banner 的 Build 时间与本地编译一致）。

---

## 0. 硬件 / 调试器（重要）

- **调试器必须用 ST-LINK**。本芯片只能用 ST-LINK 下载/调试；
  CMSIS-DAP 探针（如 H7-TOOL, `c251:f00a`）连不上——SWD 会全程 `JUNK`。
  当前已连 **ST-LINK/V2-1**（`0483:374b`），目标电压 ~3.25V。
- 串口：外接 WCH USB-UART，波特率 **115200**（APP 的 Letter Shell）。设备名
  可能是 `/dev/ttyUSB*` 或 `/dev/ttyACM*`，以 by-id 中标记为 WCH/CH34x/CH910x
  的设备为准；`/dev/ttyACM0` 若显示为 STMicroelectronics ST-Link，则不是 WCH 串口。

## 1. 环境（一次性，已配好）

| 组件 | 位置 | 说明 |
|------|------|------|
| 交叉工具链 | `arm-none-eabi-gcc 14.2.1` (apt) | `gdb-multiarch`、`cmake`、`make`、`ninja` 同装 |
| OpenOCD | `/root/code/archive/openocd` | **ST 分支** `0.12.0+dev`（支持 STM32N6 / `stldr` 外部 Flash 加载器）。二进制 `src/openocd`，脚本 `tcl/`。依赖 `libcapstone5`(apt，已装) |
| 签名工具 | `/opt/st/stm32cubeclt_1.22.0/STM32CubeProgrammer/bin/STM32_SigningTool_CLI` | 随 CubeCLT 提供，x86-64，经 binfmt/qemu 透明运行。`fsbl/CMakeLists.txt` 用 `${CUBECLT_ROOT}` 定位，**仅生成 `fsbl-trusted.bin` 时需要** |
| 调试器 | ST-LINK/V2-1 | 见上 |

OpenOCD / 签名工具 / x86-64 运行库均从远程 `root@192.168.31.54` 同种 aarch64 环境直接拷贝，二进制兼容。

## 2. 编译

```sh
cd /root/code/atk-n647/st_n6
# FSBL（产物：fsbl/build/fsbl.elf, fsbl.bin, fsbl-trusted.bin, fsbl.hex ...）
cmake -S fsbl -B fsbl/build -G "Unix Makefiles"
cmake --build fsbl/build -j
# APP（产物：app/build/app.elf, app.bin）
cmake -S app  -B app/build  -G "Unix Makefiles"
cmake --build app/build -j
```

`CMakeLists.txt` 内已指定工具链与 `-mcpu=cortex-m55 -mfpu=fpv5-d16 -mfloat-abi=hard`，无需额外 toolchain 文件。

## 3. 下载（烧 APP 到外部 Flash @0x70010000）

```sh
/root/code/archive/openocd/src/openocd \
  -s /root/code/archive/openocd/tcl -s /root/code/atk-n647/st_n6/.vscode \
  -f /root/code/atk-n647/st_n6/.vscode/openocd/flash_app.cfg
```

经 `stldr` 外部 Flash 加载器（`loader/ExtMemLoader.stldr`）擦写。`flash_app.cfg` 默认 cwd=`app/`，故 `build/app.bin` 解析为 `app/build/app.bin`。

## 4. 运行 / 看串口

```sh
# 从 RAM 启动 FSBL 再跑 APP（resume 后 openocd 退出，APP 继续运行）
/root/code/archive/openocd/src/openocd \
  -s /root/code/archive/openocd/tcl -s /root/code/atk-n647/st_n6/.vscode \
  -f /root/code/atk-n647/st_n6/.vscode/openocd/boot_app_via_fsbl.cfg
ls -l /dev/serial/by-id/
WCH_UART="$(readlink -f /dev/serial/by-id/*WCH* 2>/dev/null | head -n 1)"
test -n "$WCH_UART" && picocom -b 115200 "$WCH_UART"
# 若设备没有 WCH 字样，用 udevadm info --query=all --name=/dev/ttyUSB0
# 或 --name=/dev/ttyACM0 找到实际的 CH34x/CH910x 端口。
# 预期可见 Banner + 提示符 stm32n6:/$
```

## 5. 调试（gdb-multiarch + OpenOCD gdbserver）

```sh
# 终端1：gdbserver（常驻）
/root/code/archive/openocd/src/openocd \
  -s /root/code/archive/openocd/tcl -s /root/code/atk-n647/st_n6/.vscode \
  -f /root/code/atk-n647/st_n6/.vscode/openocd/gdbserver_extflash.cfg

# 终端2：gdb
gdb-multiarch app/build/app.elf
(gdb) target remote :3333
(gdb) monitor stm32n6_boot_app_for_debug   # 从 RAM 跑 FSBL，映射外部 Flash，halt
(gdb) maintenance flush register-cache
# 此时 APP 已在外部 Flash XIP 执行，可正常下断点 / 单步 / 看变量。
# 在外部 Flash 映射完成前不要在 main 提前下断点（见 launch.json 说明）。
```

VSCode：用 `cortex-debug` 扩展，`launch.json` 已配 `Debug FSBL` / `Debug APP (External Flash)`
（`serverpath=/root/code/archive/openocd/src/openocd`）。

> 若上一次调试残留导致 `Failed to read memory at 0x70011cd4`，给板子断电上电一次即可。

## 6. 从远程同步

```sh
ssh-keyscan 192.168.31.54 >> ~/.ssh/known_hosts   # 首次/换主机时
rsync -aH --exclude='/build/' --exclude='/app/build/' --exclude='/fsbl/build/' \
  root@192.168.31.54:/root/code/st_n6/ /root/code/atk-n647/st_n6/
# 远端源路径以远端实际布局为准（远端若也迁移到 atk-n647/st_n6，需同步修改）
```

排除 `build/`（CMake 缓存含绝对路径，迁过来需重新 configure）。

## 7. 代码跳转 / 补全 (VSCode IntelliSense)

跳转/悬停/补全基于 `compile_commands.json`(CMake 导出的**每个文件真实编译参数**),
不是手写 include 列表。已配好:

- `.vscode/c_cpp_properties.json`:`compileCommands` 指向 `${workspaceFolder}/compile_commands.json`
  (app+fsbl 合并,53 条;`compilerPath=/usr/bin/arm-none-eabi-gcc`,`intelliSenseMode=gcc-arm`,`cStandard=c11`)。
- `.vscode/settings.json`:`cmake.configureSettings` 设了 `CMAKE_EXPORT_COMPILE_COMMANDS=ON`。
- `.vscode/merge_compile_commands.py`:把 `app/build` + `fsbl/build` 的 compile_commands.json 合并到根目录。
- tasks.json 两个 Configure 任务都带 `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`,并新增任务 **`Refresh IntelliSense DB`**。

重新 configure 或增删源文件后,跑一次 `Refresh IntelliSense DB`
(或 `python3 .vscode/merge_compile_commands.py`)刷新;改已有文件内容无需刷新。
改完配置后在 VSCode 里 `Ctrl+Shift+P` → "Reload Window" 让 cpptools 重建索引。

> 旧的 `C_Cpp.default.includePath/defines`(手写、app/fsbl 混在一起,还误带 `USE_HAL_DRIVER`
> —— 实际代码只用 `USE_FULL_LL_DRIVER`)已移除,那正是跳转/提示不准的根因。

## 8. 注意事项

- **必须 ST-LINK**；CMSIS-DAP 不可用。
- `.vscode/` 被 `.gitignore` 忽略（本地编辑器/调试配置，改它不影响仓库）。
- 仓库被追踪文件与远程 HEAD 完全一致（`git status` 干净）。
- `qemu` + `STM32_SigningTool_CLI` 仅在生成 `fsbl-trusted.bin`（`flash.cfg` 生产烧 FSBL）时用到；
  日常调试/烧 APP 用不到。
- APP 是 FreeRTOS + Letter Shell，XIP 于外部 Flash（`.text@0x7001034c`，`main@0x70011a9c`），
  `.data@0x34000000`。
