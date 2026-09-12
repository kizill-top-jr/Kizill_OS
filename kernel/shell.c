// shell.c - interactive command line
#include <stdint.h>
#include <stddef.h>
#include "shell.h"
#include "string.h"

#define VIDEO 0xB8000
#define ROWS 25
#define COLS 80
#define SHELL_ROW 3

static char buffer[64];
static int buf_pos = 0;
static int cursor_row = SHELL_ROW;
static int cursor_col = 0;

void shell_execute(char *cmd);

static void print_char(char c, uint8_t attr, int row, int col);
static void print_string(const char *s, uint8_t attr, int row, int col);
static void clear_screen(void);
static void newline(void);

// trigger real divide-by-zero (not optimizable)
__attribute__((noinline))
static void trigger_div0(void) {
    volatile int zero = 0;
    volatile int result;
    asm volatile(
        "movl $1, %%eax\n"
        "xorl %%edx, %%edx\n"
        "divl %1\n"
        "movl %%eax, %0\n"
        : "=m"(result)
        : "r"(zero)
        : "eax", "edx"
    );
    (void)result;
}

void shell_init(void) {
    print_string("Kizill_OS Shell v1.0\n", 0x1F, SHELL_ROW, 0);
    print_string("> ", 0x0F, cursor_row, cursor_col);
}

void shell_input(char c) {
    if (c == '\n' || c == '\r') {
        buffer[buf_pos] = '\0';
        shell_execute(buffer);
        buf_pos = 0;
        newline();
        print_string("> ", 0x0F, cursor_row, cursor_col);
        return;
    }
    if (c == '\b' || c == 0x7F) {
        if (buf_pos > 0) {
            buf_pos--;
            cursor_col--;
            print_char(' ', 0x07, cursor_row, cursor_col);
        }
        return;
    }
    if (buf_pos < 63 && c >= 0x20 && c <= 0x7E) {
        buffer[buf_pos++] = c;
        print_char(c, 0x0F, cursor_row, cursor_col);
        cursor_col++;
    }
}

void shell_execute(char *cmd) {
    newline();
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    if (strcmp(cmd, "clear") == 0) {
        clear_screen();
        cursor_row = SHELL_ROW;
        cursor_col = 0;
        print_string("Kizill_OS Shell v1.0\n", 0x1F, SHELL_ROW, 0);
        return;
    }

    if (strcmp(cmd, "help") == 0) {
        print_string("Commands:\n  clear   - clear screen\n  echo    - print text\n  help    - this\n  panic   - trigger exception\n  reboot  - reboot (stub)\n  version - OS version\n", 0x0F, cursor_row, cursor_col);
        cursor_row += 7;
        return;
    }

    if (strcmp(cmd, "version") == 0) {
        print_string("Kizill_OS 0.2 (32-bit) - Sep 2026\n", 0x0F, cursor_row, cursor_col);
        cursor_row++;
        return;
    }

    if (strncmp(cmd, "echo ", 5) == 0) {
        print_string(cmd + 5, 0x0F, cursor_row, cursor_col);
        cursor_row++;
        return;
    }

    if (strcmp(cmd, "panic") == 0) {
        trigger_div0();
        return;
    }

    if (strcmp(cmd, "reboot") == 0) {
        print_string("Rebooting...\n", 0x0F, cursor_row, cursor_col);
        while (1) {}
    }

    print_string("Unknown. Type 'help'.\n", 0x0F, cursor_row, cursor_col);
    cursor_row++;
}

static void print_char(char c, uint8_t attr, int row, int col) {
    char *video = (char *)VIDEO;
    int pos = (row * COLS + col) * 2;
    video[pos] = c;
    video[pos + 1] = attr;
}

static void print_string(const char *s, uint8_t attr, int row, int col) {
    while (*s) {
        if (*s == '\n') { row++; col = 0; }
        else {
            print_char(*s, attr, row, col);
            col++;
        }
        s++;
        if (col >= COLS) { row++; col = 0; }
    }
    cursor_row = row;
    cursor_col = col;
}

static void clear_screen(void) {
    char *video = (char *)VIDEO;
    for (int i = 0; i < ROWS * COLS * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }
    cursor_row = 0;
    cursor_col = 0;
}

static void newline(void) {
    cursor_row++;
    cursor_col = 0;
    if (cursor_row >= ROWS) {
        char *video = (char *)VIDEO;
        for (int row = 1; row < ROWS; row++) {
            for (int col = 0; col < COLS * 2; col++) {
                video[(row - 1) * COLS * 2 + col] = video[row * COLS * 2 + col];
            }
        }
        for (int col = 0; col < COLS * 2; col++) {
            video[(ROWS - 1) * COLS * 2 + col] = 0;
        }
        cursor_row = ROWS - 1;
        cursor_col = 0;
    }
}
