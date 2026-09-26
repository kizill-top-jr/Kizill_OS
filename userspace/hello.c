// first real user program, compiled as ELF
// raw syscalls, no libc

static long sys_write(long fd, const char *buf, long len) {
    long ret;
    asm volatile(
        "mov $1, %%rax\n"
        "mov %1, %%rdi\n"
        "mov %2, %%rsi\n"
        "mov %3, %%rdx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r"(ret)
        : "r"(fd), "r"(buf), "r"(len)
        : "rax", "rdi", "rsi", "rdx", "memory"
    );
    return ret;
}

static long sys_exit(long code) {
    asm volatile(
        "mov $60, %%rax\n"
        "mov %0, %%rdi\n"
        "int $0x80\n"
        : : "r"(code) : "rax", "rdi", "memory"
    );
    return 0;
}

int main(void) {
    const char *msg = "hello from ELF!\n";
    sys_write(1, msg, 17);
    return 33;
}

// entry point for the ELF binary
void _start(void) {
    int code = main();
    sys_exit(code);
    // never reached
    for (;;) {}
}
