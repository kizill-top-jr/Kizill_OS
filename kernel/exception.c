// exception.c - exception handler with readable output on red screen
#include <stdint.h>

static const char *exception_names[] = {
    "Divide by zero",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 FPU error",
    "Alignment check",
    "Machine check",
    "SIMD FPU error",
    "Virtualization error",
    "Control protection",
};

static void write_at(int pos, const char *s, char attr) {
    volatile char *video = (volatile char *)0xB8000;
    for (int i = 0; s[i]; i++) {
        video[pos] = s[i];
        video[pos + 1] = attr;
        pos += 2;
    }
}

void exception_handler(uint32_t num) {
    volatile char *video = (volatile char *)0xB8000;

    // fill screen with red background
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x4F;   // red bg, white fg
    }

    int pos = 0;
    write_at(pos, "KERNEL PANIC: ", 0x4F);
    pos += 14 * 2;

    if (num == 0xFFFFFFFF) {
        write_at(pos, "Unknown exception", 0x4F);
        pos += 17 * 2;
    } else if (num < 22) {
        write_at(pos, exception_names[num], 0x4F);
        pos += 30 * 2; // overshoot is fine, write_at stops at '\0'
    } else {
        write_at(pos, "Unknown", 0x4F);
    }

    // if page fault, show CR2 and error code
    if (num == 14) {
        uint32_t cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));

        int line2 = 2 * 80 * 2;
        write_at(line2, "Faulting address: 0x", 0x4F);
        int hp = line2 + 20 * 2;
        const char *hex = "0123456789ABCDEF";
        for (int i = 28; i >= 0; i -= 4) {
            video[hp] = hex[(cr2 >> i) & 0xF];
            video[hp + 1] = 0x4F;
            hp += 2;
        }
    }

    asm volatile("cli");
    while (1) {
        asm volatile("hlt");
    }
}
