// kernel.c - main entry point
extern void init_paging(void);
extern void idt_init(void);
extern void timer_init(void);
extern void shell_init(void);

void kmain(void) {
    char *video = (char *)0xB8000;

    // clear screen
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }

    // centered header
    const char *msg = "Kizill_OS with IRQ!";
    int len = 19;
    int start_col = (80 - len) / 2;
    for (int i = 0; msg[i]; i++) {
        int pos = (0 * 80 + start_col + i) * 2;
        video[pos] = msg[i];
        video[pos + 1] = 0x1F;
    }

    // ENABLE PAGING FIRST (before IDT/timer/shell)
    init_paging();

    idt_init();
    timer_init();
    shell_init();

    asm volatile("sti");
    while (1) {}
}
