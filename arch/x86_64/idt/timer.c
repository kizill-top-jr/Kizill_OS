#include <kernel/fb.h>
#include <kernel/types.h>

volatile u64 g_ticks = 0;

// handler is called from isr_stub_timer in asm, but actually
// the scheduler switch is done there. this function is just
// a placeholder for future tick accounting.
void timer_handler(void) {
    g_ticks++;
}
