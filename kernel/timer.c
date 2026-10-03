volatile u64 g_ticks = 0;

void timer_handler(void) {
    g_ticks++;
}
