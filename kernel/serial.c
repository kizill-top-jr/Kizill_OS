#include <kernel/serial.h>
#include <kernel/types.h>

#define COM1 0x3F8

static inline void outb(u16 port, u8 val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00);   // no irqs
    outb(COM1 + 3, 0x80);   // DLAB
    outb(COM1 + 0, 0x03);   // 38400 baud
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   // 8N1
    outb(COM1 + 2, 0xC7);   // FIFO
    outb(COM1 + 4, 0x0B);   // RTS/DSR
}

void serial_putc(char c) {
    while (!(inb(COM1 + 5) & 0x20));
    outb(COM1, (u8)c);
}

void serial_puts(const char *s) {
    while (*s) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s++);
    }
}

void serial_hex(u64 v) {
    const char *h = "0123456789ABCDEF";
    serial_puts("0x");
    for (int i = 60; i >= 0; i -= 4)
        serial_putc(h[(v >> i) & 0xF]);
}
