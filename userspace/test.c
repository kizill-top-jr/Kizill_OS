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

#include <uapi/syscall.h>

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
    // test 4: getpid
    long my_pid = sys_call(39, 0, 0, 0); // SYS_GETPID
    if (my_pid > 0) {
        print("[test 4] getpid: OK (pid=");
        print_num(my_pid);
        print(")\n");
    } else {
        print("[test 4] getpid FAILED\n");
    }

    // test 5: uptime
    long up = sys_call(101, 0, 0, 0); // SYS_UPTIME
    if (up > 0) {
        print("[test 5] uptime: OK\n");
    } else {
        print("[test 5] uptime FAILED (returns 0)\n");
    }

    // test 6: tasks
    struct {
        unsigned long pid;
        unsigned long ppid;
        int state;
        char name[32];
    } tasks[8];

    long n = sys_call(102, (long)tasks, 8, 0); // SYS_TASKS
    if (n > 0) {
        print("[test 6] tasks list: OK (found ");
        print_num(n);
        print(" tasks)\n");
    } else {
        print("[test 6] tasks list FAILED\n");
    }

    // test 7: invalid spawn pointer
    long r_exec = sys_call(100, 0xFFFF800000000000, 0, 0); // SYS_SPAWN
    if (r_exec < 0) {
        print("[test 7] invalid spawn rejected: OK\n");
    } else {
        print("[test 7] FAIL: invalid spawn was allowed!\n");
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
