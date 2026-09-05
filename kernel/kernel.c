// kernel.c - main entry point, center header, shell init
extern void idt_init();
extern void timer_init();
extern void shell_init();

void kmain() {
    char *video = (char *)0xB8000;
    // clear screen
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }

    // print centered header (19 chars)
    const char *msg = "Kizill_OS with IRQ!";
    int len = 19;
    int start_col = (80 - len) / 2;
    for (int i = 0; msg[i]; i++) {
        int pos = (0 * 80 + start_col + i) * 2;
        video[pos] = msg[i];
        video[pos + 1] = 0x1F;   // blue background, white text
    }

    idt_init();
    timer_init();
    shell_init();

    asm volatile("sti");
    while (1) {}
}
