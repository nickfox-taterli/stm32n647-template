# STM32N647 工程

STM32N647X0HxQ 嵌入式工程,包含 FSBL(First Stage Boot Loader)和 APP 两个子项目.

## 目录结构

```
.
├── app/                  # 应用程序(运行在外部 Flash)
│   ├── Core/             # 应用源代码
│   ├── Drivers/          # HAL/LL 驱动和 CMSIS
│   ├── ld/               # 链接脚本
│   ├── Secure_nsclib/    # TrustZone NSC 接口
│   └── Startup/          # 启动文件
├── fsbl/                 # 第一阶段引导加载程序
│   ├── Core/             # FSBL 源代码
│   ├── Drivers/          # HAL/LL 驱动,BSP 和 CMSIS
│   ├── ld/               # 链接脚本
│   ├── Middlewares/      # 外部存储器管理中间件
│   └── Startup/          # 启动文件
├── loader/               # OpenOCD 外部 Flash 加载器
│   └── ExtMemLoader.stldr
├── scripts/              # CubeCLT 构建和烧录脚本
└── build/                # 本地构建输出（被 Git 忽略）
```

## VENC 录像

串口命令触发的 10 秒 H.264 录像、SD NAND 裸盘布局和主机导出方法见
[`docs/venc-recording.md`](docs/venc-recording.md)。该流程会覆盖 SD NAND 的
FAT/MBR，执行前请确认盘内数据可以丢弃。
