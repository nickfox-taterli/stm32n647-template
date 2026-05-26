#include "shell_port.h"

#include "FreeRTOS.h"
#include "task.h"

#include "serial_console.h"
#include "shell.h"

static Shell shell;
static char shell_buffer[256];

static signed short shell_read(char *data, unsigned short len)
{
  return (signed short)serial_console_read(data, len);
}

static signed short shell_write(char *data, unsigned short len)
{
  return (signed short)serial_console_write(data, len);
}

static void shell_task(void *argument)
{
  (void)argument;

  shell.read = shell_read;
  shell.write = shell_write;
  shellInit(&shell, shell_buffer, sizeof(shell_buffer));
  shellTask(&shell);
}

static int cmd_uptime(void)
{
  shellPrint(shellGetCurrent(), "uptime: %lu ms\r\n", (unsigned long)xTaskGetTickCount());
  return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
                 uptime,
                 cmd_uptime,
                 show FreeRTOS uptime);

static int cmd_echo(int argc, char *argv[])
{
  Shell *current = shellGetCurrent();

  for (int i = 1; i < argc; i++)
  {
    shellPrint(current, "%s%s", argv[i], (i == (argc - 1)) ? "" : " ");
  }
  shellPrint(current, "\r\n");
  return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),
                 echo,
                 cmd_echo,
                 echo arguments);

void shell_port_start(void)
{
  BaseType_t ok = xTaskCreate(shell_task,
                              "shell",
                              1024,
                              NULL,
                              3,
                              NULL);
  configASSERT(ok == pdPASS);
}
