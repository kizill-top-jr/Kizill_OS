#include <kernel/fb.h>
#include <kernel/types.h>

static const char *names[] = {
    "Divide error", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound range", "Invalid opcode", "Device n/a",
    "Double fault", "Coprocessor", "Invalid TSS", "Segment not present",
    "Stack fault", "General protection", "Page fault", "Reserved",
    "x87 FPU", "Alignment", "Machine check", "SIMD FPU",
    "Virtualization", "Control protection",
};

void exception_handler(u64 vec) {
    fb_clear(0x00C00000);

    printk_color("KERNEL PANIC: ", FB_WHITE);

    if (vec < 22)
        printk(names[vec]);
    else
        printk("Unknown");

    printk("  (vector "); printk_dec(vec); printk(")\n");

    if (vec == 14) {
        u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        printk("fault addr: "); printk_hex(cr2); printk("\n");
    }

    for (;;) asm volatile("cli; hlt");
}
