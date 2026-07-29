#ifndef CONSOLE_LOG_H
#define CONSOLE_LOG_H

#include <stdint.h>

/* Level severity increases with value; messages below the current threshold
 * are dropped. Vocabulary mirrors STEdgeAI core_log.h (DEBUG/INFO/WARN/ERROR)
 * so the two layers stay consistent. */
typedef enum
{
  CONSOLE_LOG_DEBUG = 0,
  CONSOLE_LOG_INFO = 1,
  CONSOLE_LOG_WARN = 2,
  CONSOLE_LOG_ERROR = 3,
} console_log_level_t;

void console_log_set_level(console_log_level_t level);
console_log_level_t console_log_get_level(void);

/* printf-style, level-filtered. Formats into an internal buffer and routes to
 * serial_console_write(); output is byte-identical to writing the formatted
 * string directly (no level prefix is emitted). */
void console_log_write(console_log_level_t level, const char *fmt, ...);

/* Call sites use these. No per-module aliases. */
#define LOG_DEBUG(...) console_log_write(CONSOLE_LOG_DEBUG, ##__VA_ARGS__)
#define LOG_INFO(...)  console_log_write(CONSOLE_LOG_INFO,  ##__VA_ARGS__)
#define LOG_WARN(...)  console_log_write(CONSOLE_LOG_WARN,  ##__VA_ARGS__)
#define LOG_ERROR(...) console_log_write(CONSOLE_LOG_ERROR, ##__VA_ARGS__)

#endif /* CONSOLE_LOG_H */
