// shell.c - interactive command line
#include <stdint.h>
#include <stddef.h>
#include "shell.h"

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
static void clear_screen();
static void newline();

void shell_init() {
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

    if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r') {
        clear_screen();
        cursor_row = SHELL_ROW;
        cursor_col = 0;
        print_string("Kizill_OS Shell v1.0\n", 0x1F, SHELL_ROW, 0);
        return;
    }
    if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p') {
        print_string("Commands:\n  clear  - clear screen\n  echo   - print text\n  help   - this\n  reboot - reboot (stub)\n  version - OS version\n", 0x0F, cursor_row, cursor_col);
        cursor_row += 6;
        return;
    }
    if (cmd[0] == 'v' && cmd[1] == 'e' && cmd[2] == 'r' && cmd[3] == 's' && cmd[4] == 'i' && cmd[5] == 'o' && cmd[6] == 'n') {
        print_string("Kizill_OS 0.2 (32-bit) - Sep 2026\n", 0x0F, cursor_row, cursor_col);
        cursor_row++;
        return;
    }
    if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o') {
        char *p = cmd + 4;
        while (*p == ' ') p++;
        if (*p) {
            print_string(p, 0x0F, cursor_row, cursor_col);
            cursor_row++;
        }
        return;
    }
    if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'b' && cmd[3] == 'o' && cmd[4] == 'o' && cmd[5] == 't') {
        print_string("Rebooting...\n", 0x0F, cursor_row, cursor_col);
        while (1) {}
    }
    print_string("Unknown. Type 'help'.\n", 0x0F, cursor_row, cursor_col);
    cursor_row++;
}

// ---- helpers ----
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

static void clear_screen() {
    char *video = (char *)VIDEO;
    for (int i = 0; i < ROWS * COLS * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }
    cursor_row = 0;
    cursor_col = 0;
}

static void newline() {
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
