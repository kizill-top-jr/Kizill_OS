// kernel.c - main entry point
#include "task.h"
#include "timer.h"
#include "memory.h"

extern void idt_init(void);
extern void shell_init(void);

static void task1(void) {
    char *video = (char *)0xB8000;
    while (1) {
        video[(10 * 80 + 20) * 2]     = '1';
        video[(10 * 80 + 20) * 2 + 1] = 0x1F;
        schedule();
    }
}

static void task2(void) {
    char *video = (char *)0xB8000;
    while (1) {
        video[(12 * 80 + 50) * 2]     = '2';
        video[(12 * 80 + 50) * 2 + 1] = 0x1F;
        schedule();
    }
}

void kmain(void) {
    char *video = (char *)0xB8000;

    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }

    const char *msg = "Kizill_OS with IRQ!";
    int len = 19;
    int start = (80 - len) / 2;
    for (int i = 0; msg[i]; i++) {
        int pos = (0 * 80 + start + i) * 2;
        video[pos] = msg[i];
        video[pos + 1] = 0x1F;
    }

    init_paging();
    idt_init();
    timer_init();

    init_scheduler();
    add_task(task1, "task1");
    add_task(task2, "task2");

    shell_init();
    asm volatile("sti");

    while (1) {
        schedule();
    }
}
