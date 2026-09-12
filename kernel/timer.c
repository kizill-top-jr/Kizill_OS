// timer.c - PIT initialization and timer IRQ handler
#include <stdint.h>
#include "timer.h"

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void timer_init(void) {
    uint32_t freq = 100;                  // 100 Hz
    uint32_t divisor = 1193180 / freq;
    outb(0x43, 0x36);                     // channel 0, mode 3, binary
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void timer_handler(void) {
    // Acknowledge only. Scheduling tick will be added later.
}
