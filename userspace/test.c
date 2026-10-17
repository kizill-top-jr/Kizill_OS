// Kizill_OS basic syscall test

static long sys_call(long n, long a, long b, long c) {
    long ret;
    asm volatile(
        "mov %1, %%rax\n"
        "mov %2, %%rdi\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r"(ret)
        : "r"(n), "r"(a), "r"(b), "r"(c)
        : "rax", "rdi", "rsi", "rdx", "memory"
    );
    return ret;
}

#define SYS_WRITE   1
#define SYS_FORK    57
#define SYS_EXIT    60
#define SYS_WAIT    61

static void print(const char *s) {
    long n = 0;
    while (s[n]) n++;
    sys_call(SYS_WRITE, 1, (long)s, n);
}

static void print_num(long v) {
    char buf[24];
    int n = 0;
    if (v == 0) { buf[n++] = '0'; }
    while (v > 0) { buf[n++] = '0' + (v % 10); v /= 10; }
    for (int i = n - 1; i >= 0; i--) {
        sys_call(SYS_WRITE, 1, (long)&buf[i], 1);
    }
}

int main(void) {
    print("=== test.elf started ===\n");

    // test 1: valid write
    print("[test 1] valid write: OK\n");

    // test 2: invalid write (kernel pointer)
    // should return -1, not crash
    long r = sys_call(SYS_WRITE, 1, 0xFFFF800000000000, 16);
    if (r < 0) {
        print("[test 2] invalid write rejected: OK\n");
    } else {
        print("[test 2] FAIL: invalid write was allowed!\n");
    }

    // test 3: fork and wait
    long pid = sys_call(SYS_FORK, 0, 0, 0);
    if (pid == 0) {
        // child
        print("[test 3] child running: OK\n");
        sys_call(SYS_EXIT, 42, 0, 0);
    } else if (pid > 0) {
        // parent
        long code = -1;
        while (code < 0) {
            code = sys_call(SYS_WAIT, pid, 0, 0);
        }
        print("[test 3] child exited with code: ");
        print_num(code);
        if (code == 42) print(" (OK)\n");
        else print(" (FAIL)\n");
    } else {
        print("[test 3] fork FAILED\n");
    }

    print("=== all tests passed ===\n");
    return 0;
}

void _start(void) {
    asm volatile("andq $-16, %rsp");
    int code = main();
    sys_call(SYS_EXIT, code, 0, 0);
    for (;;) {}
}
