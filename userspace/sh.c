// Kizill_OS shell -- ring 3

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

#define SYS_WRITE 1
#define SYS_READ  0
#define SYS_EXIT  60
#define SYS_CLEAR 99
#define SYS_EXEC  100

static long write(long fd, const char *s, long n) {
    return sys_call(SYS_WRITE, fd, (long)s, n);
}

static long read_(long fd, char *buf, long n) {
    return sys_call(SYS_READ, fd, (long)buf, n);
}

static long exit_(long c) {
    return sys_call(SYS_EXIT, c, 0, 0);
}

static long clear_(void) {
    return sys_call(SYS_CLEAR, 0, 0, 0);
}

static long exec_(const char *name) {
    return sys_call(SYS_EXEC, (long)name, 0, 0);
}

static int str_eq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == 0 && *b == 0;
}

static int str_pref(const char *a, const char *p) {
    while (*p) { if (*a != *p) return 0; a++; p++; }
    return 1;
}

static long strlen_(const char *s) {
    long n = 0; while (s[n]) n++; return n;
}

// strip trailing newline/spaces
static void strip(char *s) {
    long n = strlen_(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || s[n-1] == ' ')) {
        s[n-1] = 0;
        n--;
    }
}

int main(void) {
    char buf[128];

    // clear screen so kernel messages and shell don't overlap
    sys_call(SYS_CLEAR, 0, 0, 0);

    write(1, "Kizill_OS Shell v0.1\n", 21);

    while (1) {
        write(1, "> ", 2);

        // read one line (poll in a loop)
        int pos = 0;
        while (1) {
            long n = read_(0, buf + pos, 1);
            if (n <= 0) continue;   // non-blocking, retry

            char c = buf[pos];
            if (c == '\n') {
                write(1, "\n", 1);
                pos++;
                break;
            }
            if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    write(1, "\b \b", 3);
                }
                continue;
            }
            // echo
            write(1, &c, 1);
            pos++;
            if (pos >= 127) {
                write(1, "\n", 1);
                break;
            }
        }
        buf[pos] = 0;

        char cmd[128];
        int i = 0;
        while (buf[i] && i < 127) { cmd[i] = buf[i]; i++; }
        cmd[i] = 0;
        strip(cmd);

        if (cmd[0] == 0) continue;

        if (str_eq(cmd, "help")) {
            write(1, "commands:\n  help    - this\n  clear   - clear screen\n  echo X  - print X\n  hello   - run hello.elf\n  exit    - leave shell\n", 119);
        } else if (str_eq(cmd, "clear")) {
            clear_();
        } else if (str_eq(cmd, "exit")) {
            exit_(0);
        } else if (str_pref(cmd, "echo ")) {
            const char *p = cmd + 5;
            write(1, p, strlen_(p));
            write(1, "\n", 1);
        } else if (str_eq(cmd, "hello")) {
            long r = exec_("hello.elf");
            if (r != 0) write(1, "exec failed\n", 12);
        } else {
            write(1, "unknown: ", 9);
            write(1, cmd, strlen_(cmd));
            write(1, "\n", 1);
        }
    }
}

void _start(void) {
    int code = main();
    exit_(code);
    for (;;) {}
}
