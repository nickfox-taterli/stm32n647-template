#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#include <stddef.h>
#include <stdint.h>

void serial_console_init(void);
int serial_console_read(char *data, unsigned short len);
int serial_console_write(const char *data, unsigned short len);
void serial_console_irq_handler(void);

#endif
