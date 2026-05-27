# SD HAL API 注意事项

STM32N6 的 SDMMC 外设支持三种数据传输模式。本项目使用精简 HAL shim
（`hal_shim.c`），HAL_GetTick 基于 FreeRTOS tick，HAL_Delay 基于 vTaskDelay。

## Polling 模式

```c
HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData,
    uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout);
HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, const uint8_t *pData,
    uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout);
```

- **阻塞**：调用任务会阻塞直到传输完成或超时。
- **安全上下文**：可在任何 FreeRTOS 任务中调用。不能在 ISR 中调用。
- **超时单位**：ms，由 HAL_GetTick() 计时。在 FreeRTOS 环境下分辨率受
  tick 频率限制（默认 1ms）。
- **后续操作**：写完成后需调用 `wait_card_transfer()` 等待卡回到 TRANSFER
  状态，否则下一次操作可能失败。
- **Timeout=0**：HAL 实现中 Timeout==0 会立即返回 TIMEOUT。永远不要传 0。

## IT（中断）模式

```c
HAL_StatusTypeDef HAL_SD_ReadBlocks_IT(SD_HandleTypeDef *hsd, uint8_t *pData,
    uint32_t BlockAdd, uint32_t NumberOfBlocks);
HAL_StatusTypeDef HAL_SD_WriteBlocks_IT(SD_HandleTypeDef *hsd, const uint8_t *pData,
    uint32_t BlockAdd, uint32_t NumberOfBlocks);
```

- **非阻塞**：立即返回 HAL_OK，传输在后台进行。
- **完成通知**：通过 `HAL_SD_RxCpltCallback` / `HAL_SD_TxCpltCallback` 回调。
- **ISR 上下文**：回调在 SDMMC ISR 中执行（当前 NVIC priority = 6）。
  - 不能在回调中调用阻塞型 FreeRTOS API（如 vTaskDelay、xQueueReceive）。
  - 可以调用 `...FromISR` 系列 API。
- **标志管理**：
  - 当前实现用 volatile `sd*_tx_done` / `sd*_rx_done` 标志。
  - **必须在发起传输前清零**：`*tx_done = 0;` 否则会误判上一次传输完成。
  - 任务侧轮询此标志并 `vTaskDelay(1)` 等待。
- **建议改进**：用 `xTaskNotifyFromISR` + `ulTaskNotifyTake` 替代 volatile flag
  轮询，可消除 1ms 延迟粒度。

### IT 模式使用模板

```c
volatile uint32_t *done = sd_rx_done_flag(hsd);
*done = 0;  // 清零！
ret = HAL_SD_ReadBlocks_IT(hsd, buf, blk, 1);
if (ret != HAL_OK) { /* 错误处理 */ }
// 等待完成
uint32_t t0 = HAL_GetTick();
while (*done == 0) {
    if (hsd->ErrorCode != HAL_SD_ERROR_NONE) break;
    if ((HAL_GetTick() - t0) >= 5000) break;
    vTaskDelay(pdMS_TO_TICKS(1));
}
wait_card_transfer(hsd, 5000);
```

## DMA/IDMA 模式

```c
HAL_StatusTypeDef HAL_SD_ReadBlocks_DMA(SD_HandleTypeDef *hsd, uint8_t *pData,
    uint32_t BlockAdd, uint32_t NumberOfBlocks);
HAL_StatusTypeDef HAL_SD_WriteBlocks_DMA(SD_HandleTypeDef *hsd, const uint8_t *pData,
    uint32_t BlockAdd, uint32_t NumberOfBlocks);
```

- **SDMMC 内部 DMA（IDMA）**：不使用外部 DMA 控制器（DMA1/DMA2）。
- **完成通知**：与 IT 模式相同的回调（TxCpltCallback / RxCpltCallback）。
- **DCache 管理是关键**：

### 写操作流程

```
1. 准备 tx 缓冲区数据
2. SCB_CleanDCache_by_Addr(tx_buf, size)   ← 把 CPU cache 推到 RAM
3. HAL_SD_WriteBlocks_DMA(hsd, tx_buf, blk, cnt)
4. 等待 TxCpltCallback（tx_done flag）
5. wait_card_transfer()
```

### 读操作流程

```
1. HAL_SD_ReadBlocks_DMA(hsd, rx_buf, blk, cnt)
2. 等待 RxCpltCallback（rx_done flag）
3. SCB_InvalidateDCache_by_Addr(rx_buf, size)  ← 丢弃 stale cache
4. 此时 CPU 读取 rx_buf 得到 DMA 写入的正确数据
5. wait_card_transfer()
```

### DCache 注意事项

| 规则 | 原因 |
|------|------|
| 缓冲区必须 `ALIGN_32BYTES` | SDMMC IDMA 和 cache line 对齐要求 |
| 写前 Clean，读后 Invalidate | CPU 和 DMA 看到一致的内存 |
| 不要在 WriteBlocks_DMA 和 TxCpltCallback 之间 Invalidate | 可能破坏正在进行的 DMA 传输 |
| 不要 Invalidate 正在写入的缓冲区 | 可能丢弃 DMA 刚写入的数据 |
| 缓冲区大小必须是 512 的倍数 | SD 块大小固定 512 字节 |

## 回调机制

```c
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd);
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd);
```

- HAL 中以 `__weak` 定义，项目中在 `sd_test.c` 覆盖。
- **两个 SDMMC 共享同一对回调函数**，内部通过 `hsd->Instance` 区分：
  - `SDMMC1` → `sd1_tx_done` / `sd1_rx_done`
  - `SDMMC2` → `sd2_tx_done` / `sd2_rx_done`
- 如需独立回调，可启用 `USE_HAL_SD_REGISTER_CALLBACKS` 并用
  `HAL_SD_RegisterCallback()` 注册。

## 错误处理

### 常见 ErrorCode

| 错误码 | 含义 | 常见原因 |
|--------|------|---------|
| `HAL_SD_ERROR_CMD_RSP_TIMEOUT` | 命令响应超时 | 卡未插入、时钟未开、线路错误 |
| `HAL_SD_ERROR_DATA_CRC_FAIL` | 数据 CRC 校验失败 | 信号完整性、速度过高 |
| `HAL_SD_ERROR_DATA_TIMEOUT` | 数据超时 | 总线忙、卡响应慢 |
| `HAL_SD_ERROR_TX_UNDERRUN` | 发送 FIFO 欠载 | CPU/DMA 供给不及时 |
| `HAL_SD_ERROR_RX_OVERRUN` | 接收 FIFO 溢出 | CPU/DMA 读取不及时 |
| `HAL_SD_ERROR_ADDR_OUT_OF_RANGE` | 地址越界 | 块号超出卡容量 |
| `HAL_SD_ERROR_UNSUPPORTED_FEATURE` | 不支持的操作 | 如尝试 8-bit 总线 |

### 错误后恢复

发生错误后 HAL 内部状态可能不一致，建议：

```
HAL_SD_DeInit(hsd);
HAL_SD_Init(hsd);  // 重新走完整初始化流程
```

## Block Size

- SD 卡块大小固定 512 字节（无论 SDSC 还是 SDHC/SDXC）。
- SDHC/SDXC：块寻址（地址 = 块号）。
- SDSC：字节寻址（地址 = 块号 × 512）。
- HAL 内部透明处理，调用者只需传块号。
