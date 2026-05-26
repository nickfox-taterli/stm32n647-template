#ifndef SHELL_PORT_CFG_H
#define SHELL_PORT_CFG_H

#include "FreeRTOS.h"
#include "task.h"

#define SHELL_TASK_WHILE       1
#define SHELL_USING_LOCK       0
#define SHELL_GET_TICK()       ((int)xTaskGetTickCount())
#define SHELL_HISTORY_MAX_NUMBER 5
#define SHELL_PRINT_BUFFER     160
#define SHELL_MAX_NUMBER       1
#define SHELL_DEFAULT_USER     "stm32n6"
#define SHELL_DEFAULT_USER_PASSWORD ""

#endif
