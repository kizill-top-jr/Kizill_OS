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

#define SYS_WRITE  1
#define SYS_READ   0
#define SYS_YIELD  24
#define SYS_GETPID 39
#define SYS_EXIT   60
#define SYS_WAIT   61
#define SYS_CLEAR  99
#define SYS_EXEC   100
#define SYS_UPTIME 101
#define SYS_TASKS  102
#define SYS_REBOOT 103

#define KEY_UP     0x80
#define KEY_DOWN   0x81
#define KEY_LEFT   0x82
#define KEY_RIGHT  0x83
#define KEY_HOME   0x84
#define KEY_END    0x85
#define KEY_DELETE 0x89

struct task_info {
    unsigned long pid;
    unsigned long parent_pid;
    int state;
    char name[32];
};

#define MAX_HIST 8
static char history[MAX_HIST][128];
static int  hist_count = 0;

static char line[128];
static int  pos = 0;
static int  len = 0;
static int  cursor_on = 1;
static long last_blink = 0;

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
static long wait_(long pid) {
    return sys_call(SYS_WAIT, pid, 0, 0);
}
static long yield_(void) {
    return sys_call(SYS_YIELD, 0, 0, 0);
}
static long getpid_(void) {
    return sys_call(SYS_GETPID, 0, 0, 0);
}
static long uptime_(void) {
    return sys_call(SYS_UPTIME, 0, 0, 0);
}
static long tasks_(struct task_info *buf, long n) {
    return sys_call(SYS_TASKS, (long)buf, n, 0);
}
static long reboot_(void) {
    return sys_call(SYS_REBOOT, 0, 0, 0);
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
static void print(const char *s) {
   write(1, s, strlen_(s));
}
static void str_copy(char *dst, const char *src) {
    while (*src) *dst++ = *src++;
    *dst = 0;
}
static void strip(char *s) {
    long n = strlen_(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || s[n-1] == ' ')) {
        s[n-1] = 0; n--;
    }
}
static void print_dec(long v) {
    char buf[24];
    int n = 0;
    if (v == 0) { write(1, "0", 1); return; }
    if (v < 0) { write(1, "-", 1); v = -v; }
    while (v > 0) { buf[n++] = '0' + (v % 10); v /= 10; }
    for (int i = n - 1; i >= 0; i--) write(1, &buf[i], 1);
}
static const char *state_name(int s) {
    if (s == 0) return "READY";
    if (s == 1) return "RUN";
    if (s == 2) return "BLOCK";
    if (s == 3) return "ZOMB";
    if (s == 4) return "DEAD";
    return "?";
}

static void redraw(void) {
    write(1, "\r", 1);

    char render[96];
    for (int i = 0; i < 96; i++) render[i] = ' ';

    render[0] = '>';
    render[1] = ' ';

    int idx = 2;
    int cursor_visible = 0;

    for (int i = 0; i < len; i++) {
        render[idx++] = line[i];
        if (i == pos - 1 && pos < len && cursor_on) {
            render[idx++] = '_';
            cursor_visible = 1;
        }
    }

    if (pos == len && cursor_on) {
        render[idx++] = '_';
        cursor_visible = 1;
    }

    while (idx < 80) render[idx++] = ' ';
    write(1, render, 80);

    int want_col = 2 + pos;
    if (cursor_visible) want_col++;

    int cur_col = 80;
    while (cur_col > want_col) { write(1, "\b", 1); cur_col--; }
}

static void load_history(int idx) {
    str_copy(line, history[idx]);
    len = strlen_(line);
    pos = len;
}

int main(void) {
    char buf[8];

    clear_();
    write(1, "Kizill_OS Shell v0.4\n", 21);

    while (1) {
        len = 0;
        pos = 0;
        cursor_on = 1;
        last_blink = uptime_();

        redraw();

        int hist_idx = hist_count;

        while (1) {
            long n = read_(0, buf, 1);
            if (n <= 0) {
                long t = uptime_();
                if (t - last_blink >= 8) {
                    cursor_on = !cursor_on;
                    last_blink = t;
                    redraw();
                }
                yield_();
                continue;
            }

            unsigned char c = (unsigned char)buf[0];

            if (c == '\n') {
                write(1, "\n", 1);
                break;
            }

            if (c == '\b') {
                if (pos > 0) {
                    for (int i = pos - 1; i < len - 1; i++) line[i] = line[i + 1];
                    len--;
                    pos--;
                    redraw();
                }
                continue;
            }

            if (c == KEY_DELETE) {
                if (pos < len) {
                    for (int i = pos; i < len - 1; i++) line[i] = line[i + 1];
                    len--;
                    redraw();
                }
                continue;
            }

            if (c == KEY_LEFT) {
                if (pos > 0) { pos--; redraw(); }
                continue;
            }
            if (c == KEY_RIGHT) {
                if (pos < len) { pos++; redraw(); }
                continue;
            }
            if (c == KEY_HOME) {
                pos = 0; redraw();
                continue;
            }
            if (c == KEY_END) {
                pos = len; redraw();
                continue;
            }

            if (c == KEY_UP) {
                if (hist_idx > 0) {
                    hist_idx--;
                    load_history(hist_idx);
                    redraw();
                }
                continue;
            }
            if (c == KEY_DOWN) {
                if (hist_idx < hist_count) {
                    hist_idx++;
                    if (hist_idx < hist_count) load_history(hist_idx);
                    else { len = 0; pos = 0; }
                    redraw();
                }
                continue;
            }

            if (c >= 32 && c <= 126) {
                if (len < 127) {
                    for (int i = len; i > pos; i--) line[i] = line[i-1];
                    line[pos] = (char)c;
                    len++;
                    pos++;
                    redraw();
                }
                continue;
            }
        }

        line[len] = 0;

        char cmd[128];
        str_copy(cmd, line);
        strip(cmd);

        if (cmd[0] == 0) continue;

        if (hist_count < MAX_HIST) {
            str_copy(history[hist_count++], cmd);
        } else {
            for (int i = 1; i < MAX_HIST; i++) str_copy(history[i-1], history[i]);
            str_copy(history[MAX_HIST-1], cmd);
        }

       if (str_eq(cmd, "help")) {
           print("commands:\n");
           print("  help     - this\n");
           print("  clear    - clear screen\n");
           print("  echo X   - print X\n");
           print("  uname    - OS name\n");
           print("  pid      - current pid\n");
           print("  uptime   - seconds since boot\n");
           print("  ps       - list tasks\n");
           print("  hello    - run hello.elf\n");
           print("  reboot   - reboot system\n");
           print("  crash    - crash shell (test respawn)\n");
           print("  exit     - leave shell\n");
       } else if (str_eq(cmd, "clear")) {
           clear_();
       } else if (str_eq(cmd, "exit")) {
           exit_(0);
       } else if (str_eq(cmd, "uname")) {
           print("Kizill_OS x86_64\n");
       } else if (str_eq(cmd, "pid")) {
           print("pid: "); print_dec(getpid_()); print("\n");
       } else if (str_eq(cmd, "uptime")) {
           long t = uptime_();
           print("up "); print_dec(t / 100);
           print("."); print_dec(t % 100);
           print(" seconds\n");
       } else if (str_eq(cmd, "ps")) {
           struct task_info list[8];
           long n = tasks_(list, 8);
           print("pid  ppid state  name\n");
           for (long i = 0; i < n; i++) {
               print_dec(list[i].pid);  print("    ");
               print_dec(list[i].parent_pid); print("    ");
               print(state_name(list[i].state)); print("    ");
               print(list[i].name);
               print("\n");
           }
       } else if (str_eq(cmd, "crash")) {
           print("crashing...\n");
           asm volatile("xor %%rax, %%rax\n\tdiv %%rax" ::: "rax");
       } else if (str_eq(cmd, "reboot")) {
           print("rebooting...\n");
           yield_();
           reboot_();
       } else if (str_pref(cmd, "echo ")) {
           print(cmd + 5); print("\n");
       } else if (str_eq(cmd, "hello")) {
           long r = exec_("hello.elf");
           if (r != 0) print("exec failed\n");
           else { while (wait_(-1) < 0) yield_(); }
       } else {
           print("unknown: "); print(cmd); print("\n");
       }
    }
}

void _start(void) {
    asm volatile("andq $-16, %rsp");
    int code = main();
    exit_(code);
    for (;;) {}
}
