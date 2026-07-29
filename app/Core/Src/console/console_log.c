#include "console_log.h"
#include "serial_console.h"

#include <stdarg.h>
#include <stdio.h>

#define CONSOLE_LOG_BUF_SIZE 128U

static console_log_level_t s_level = CONSOLE_LOG_INFO;

void console_log_set_level(console_log_level_t level)
{
  s_level = level;
}

console_log_level_t console_log_get_level(void)
{
  return s_level;
}

void console_log_write(console_log_level_t level, const char *fmt, ...)
{
  char buf[CONSOLE_LOG_BUF_SIZE];
  va_list ap;
  int n;

  if (level < s_level) /* severity grows with enum value: drop below threshold */
  {
    return;
  }

  va_start(ap, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (n > 0)
  {
    unsigned short len;

    if (n > (int)sizeof(buf))
    {
      len = (unsigned short)sizeof(buf);
    }
    else
    {
      len = (unsigned short)n;
    }
    (void)serial_console_write(buf, len);
  }
}
