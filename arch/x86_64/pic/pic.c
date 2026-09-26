#include <kernel/pic.h>
#include <kernel/types.h>

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

static inline void outb(u16 port, u8 val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

// remap PIC so IRQ0 lands at vector 0x20 instead of 0x08 (which is #DF)
void pic_remap(void) {
    outb(PIC1_CMD, 0x11); io_wait();
    outb(PIC2_CMD, 0x11); io_wait();
    outb(PIC1_DATA, 0x20); io_wait();  // master -> 0x20..0x27
    outb(PIC2_DATA, 0x28); io_wait();  // slave  -> 0x28..0x2F
    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    // mask everything until IDT is live
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_unmask_timer(void) {
    // IRQ0 (timer) + IRQ1 (keyboard)
    // 0xFC = 1111 1100
    outb(PIC1_DATA, 0xFC);
    outb(PIC2_DATA, 0xFF);
}
