# SD 线程安全与并发注意事项

## 句柄所有权

`hsd1`（SDMMC1/SD Card）和 `hsd2`（SDMMC2/SD NAND）是 `sd_test.c` 中的 static 变量。

- **初始化阶段**：由 `SD_InitTask`（FreeRTOS task, priority 2）完成 `HAL_SD_Init`。
  初始化结束后 task 自删除，`sd*_ready` 标志置 1。
- **运行阶段**：由 shell task（priority 3）通过 `sd read` / `sd write` 访问。
  当前只有一个 task 操作 SD，无需加锁。

## ISR 并发

| 项目 | 值 |
|------|-----|
| SDMMC1 IRQ Priority | 6 |
| SDMMC2 IRQ Priority | 6 |
| configMAX_SYSCALL_INTERRUPT_PRIORITY | 5 |
| configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY | 5 |

SDMMC IRQ priority（6）低于 configMAX_SYSCALL_INTERRUPT_PRIORITY（5），因此：
- ISR 中可以安全调用 `FromISR` 系列 FreeRTOS API。
- ISR 不会抢占 FreeRTOS 内核临界区。

### ISR 执行路径

```
SDMMC1_IRQHandler()
  → HAL_SD_IRQHandler(&hsd1)
    → 检查 DATAEND/DCRCFAIL/DTIMEOUT 等 flag
    → 调用 HAL_SD_RxCpltCallback(&hsd1) 或 HAL_SD_TxCpltCallback(&hsd1)
      → 设置 volatile sd1_rx_done 或 sd1_tx_done = 1
```

回调中只做一件事：设置 volatile flag。这是 ISR 安全的。

### volatile flag 的内存序

在单核 Cortex-M33 上，volatile 写对同一优先级的代码可见性由硬件保证。
不需要额外的 `__DSB()`。如果未来迁移到多核，需要加 DMB/DSB。

## DCache 与 DMA 线程安全

### 规则

1. **Clean/Invalidate 必须由发起 DMA 传输的同一 task 执行**。
   - 如果 task A 发起 DMA 写，task A 负责在发起前 Clean 缓冲区。
   - 如果 task A 发起 DMA 读，task A 负责在回调后 Invalidate 缓冲区。
   - 其他 task 不应同时操作同一缓冲区。

2. **栈上的 ALIGN_32BYTES 缓冲区天然是 task 私有的**。
   - 每个 task 有独立栈，不会与其他 task 共享。

3. **不要在中断中做 DCache 操作**。
   - 回调在 ISR 上下文执行，做 DCache 操作可能干扰被中断 task 的 cache 状态。
   - DCache 操作应在 task 侧完成（回调仅设 flag，task 轮询到 flag 后 Invalidate）。

### 当前实现的 DMA 流程（安全）

```
[task 侧]                          [ISR 侧]
dcache_clean(tx_buf, size)
HAL_SD_WriteBlocks_DMA(...)
    ...                             SDMMC IRQ
                                      HAL_SD_IRQHandler()
                                        TxCpltCallback()
                                          tx_done = 1
wait for tx_done == 1
wait_card_transfer()
```

```
[task 侧]                          [ISR 侧]
HAL_SD_ReadBlocks_DMA(...)
    ...                             SDMMC IRQ
                                      HAL_SD_IRQHandler()
                                        RxCpltCallback()
                                          rx_done = 1
wait for rx_done == 1
dcache_invalidate(rx_buf, size)   ← 在 task 侧做，不在 ISR 中
wait_card_transfer()
```

## 当前并发模型

```
┌─────────────┐  priority 3
│  shell task  │─── sd read/write (polling/IT/DMA)
└─────────────┘
┌─────────────┐  priority 2
│  sd_init (一次性) │─── HAL_SD_Init × 2, then self-delete
└─────────────┘
┌─────────────┐  priority 1
│  led task    │─── GPIO toggle only
└─────────────┘
┌─────────────┐  ISR priority 6
│  SDMMC1/2 IRQ│─── HAL_SD_IRQHandler → callback → set flag
└─────────────┘
```

**无并发冲突**：sd_init 先运行完再自删除，之后只有 shell task 访问 SD。
ISR 只读写 volatile flag。Led task 不涉及 SD。

## 未来改进建议

### 1. volatile flag → Task Notification

当前 IT/DMA 模式用 `vTaskDelay(1ms)` 轮询 flag。替换方案：

```c
// 回调中（ISR context）
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(sd_rx_wait_task, 1, eSetValueWithOverwrite,
                       &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// task 侧
uint32_t notified;
xTaskNotifyWait(0, ULONG_MAX, &notified, pdMS_TO_TICKS(5000));
```

优点：零延迟唤醒，无轮询开销。

### 2. Per-device Mutex

如果多个 task 需要访问同一个 SD 设备：

```c
static SemaphoreHandle_t sd1_mutex;
// sd1_mutex = xSemaphoreCreateMutex();
// 每次操作前 xSemaphoreTake(sd1_mutex, portMAX_DELAY);
// 操作完成后 xSemaphoreGive(sd1_mutex);
```

### 3. sd_curr_dev 保护

当前 `sd_curr_dev` 只在 shell task 中修改和读取。如果未来需要从多个 task
访问，应使用 critical section：

```c
taskENTER_CRITICAL();
int dev = sd_curr_dev;
taskEXIT_CRITICAL();
```
