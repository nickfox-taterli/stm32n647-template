#ifndef SD_TEST_H
#define SD_TEST_H

/* 开机自动初始化任务（由 main.c 创建为 FreeRTOS task） */
void SD_InitTask(void *argument);

/* 检查设备是否已初始化就绪（dev: 1=SDMMC1, 2=SDMMC2） */
int  sd_is_ready(int dev);

/* sd shell 命令入口（已通过 SHELL_EXPORT_CMD 注册） */
int  sd_cmd(int argc, char **argv);

#endif /* SD_TEST_H */
