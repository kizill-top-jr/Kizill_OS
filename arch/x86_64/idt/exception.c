#include <kernel/fb.h>
#include <kernel/types.h>
#include <kernel/exception.h>
#include <kernel/task.h>

static const char *names[] = {
    "Divide error", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound range", "Invalid opcode", "Device n/a",
    "Double fault", "Coprocessor", "Invalid TSS", "Segment not present",
    "Stack fault", "General protection", "Page fault", "Reserved",
    "x87 FPU", "Alignment", "Machine check", "SIMD FPU",
    "Virtualization", "Control protection",
};

// kernel-mode panic -- never returns
static void kernel_panic(struct exception_frame *f) {
    fb_clear(0x00C00000);

    printk_color("KERNEL PANIC: ", FB_WHITE);
    u64 vec = f->vector;
    if (vec < 22) printk(names[vec]);
    else          printk("Unknown");
    printk("  (vector "); printk_dec(vec); printk(")\n");

    printk("rip: "); printk_hex(f->rip);
    printk("  cs: ");  printk_hex(f->cs);
    printk("  err: "); printk_hex(f->errcode);
    printk("\n");

    if (vec == 14) {
        u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        printk("fault addr: "); printk_hex(cr2); printk("\n");
    }

    for (;;) asm volatile("cli; hlt");
}

u64 exception_dispatch(struct exception_frame *f) {
    u64 vec = f->vector;

    // rpl bits of CS: 0 = kernel, 3 = user
    int from_user = (f->cs & 3) == 3;

    // #DF is always fatal -- can't recover
    if (vec == 8) {
        fb_clear(0x00C00000);
        printk_color("DOUBLE FAULT -- kernel bug or bad IDT\n", FB_WHITE);
        printk("rip: "); printk_hex(f->rip); printk("\n");
        printk("err: "); printk_hex(f->errcode); printk("\n");
        for (;;) asm volatile("cli; hlt");
    }

    if (from_user) {
        // log and kill the task, don't panic
        printk_color("[fault] killed user task: ", FB_YELLOW);
        if (vec < 22) printk(names[vec]); else printk("unknown");
        printk("  vec="); printk_dec(vec);
        printk("  rip="); printk_hex(f->rip);
        printk("\n");

        u64 rsp = task_exit_current(128 + (int)vec);
        return rsp;
    }

    // kernel-mode exception -> unrecoverable
    kernel_panic(f);
    return 0;  // unreachable
}
