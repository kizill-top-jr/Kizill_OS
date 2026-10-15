#include <kernel/fb.h>
#include <kernel/types.h>

static inline void outb(u16 port, u8 val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void pit_init(void) {
    u32 div = 1193182 / 100;
    outb(0x43, 0x36);
    outb(0x40, (u8)(div & 0xFF));
    outb(0x40, (u8)((div >> 8) & 0xFF));
}

volatile u64 g_ticks = 0;

// handler is called from isr_stub_timer in asm, but actually
// the scheduler switch is done there. this function is just
// a placeholder for future tick accounting.
void timer_handler(void) {
    g_ticks++;
}
