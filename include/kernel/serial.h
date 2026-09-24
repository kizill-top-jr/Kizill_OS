#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

#include <kernel/types.h>

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);
void serial_hex(u64 v);

#endif
