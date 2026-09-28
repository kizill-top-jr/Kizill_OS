#include <kernel/keyboard.h>

#define KBD_BUF_SIZE 256

static volatile char kbd_buf[KBD_BUF_SIZE];
static volatile u64  kbd_head = 0;
static volatile u64  kbd_tail = 0;

// modifier state
static volatile int shift_l = 0;
static volatile int shift_r = 0;
static volatile int caps    = 0;
static volatile int ctrl    = 0;
static volatile int alt     = 0;

// E0 prefix pending
static volatile int e0_pending = 0;

// ---- normal scancode map (no shift) ----
static const char map_normal[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
    [0x0E] = '\b',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
    [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
    [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\', [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c',
    [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n', [0x32] = 'm',
    [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x39] = ' ',
};

// ---- shifted map ----
static const char map_shift[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$',
    [0x06] = '%', [0x07] = '^', [0x08] = '&', [0x09] = '*',
    [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+',
    [0x0E] = '\b',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R',
    [0x14] = 'T', [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I',
    [0x18] = 'O', [0x19] = 'P', [0x1A] = '{', [0x1B] = '}',
    [0x1C] = '\n',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F',
    [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K',
    [0x26] = 'L', [0x27] = ':', [0x28] = '"', [0x29] = '~',
    [0x2B] = '|', [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C',
    [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N', [0x32] = 'M',
    [0x33] = '<', [0x34] = '>', [0x35] = '?',
    [0x39] = ' ',
};

static inline u8 inb(u16 port) {
    u8 v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline int is_alpha(char c) {
    return c >= 'a' && c <= 'z';
}

static void push_char(char c) {
    u64 next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next != kbd_tail) {
        kbd_buf[kbd_head] = c;
        asm volatile("" ::: "memory");
        kbd_head = next;
    }
}

void keyboard_init(void) {
    kbd_head = 0;
    kbd_tail = 0;
    shift_l = shift_r = caps = ctrl = alt = 0;
    e0_pending = 0;
}

void keyboard_handler(void) {
    u8 sc = inb(0x60);

    // ---- handle E0 prefix ----
    if (sc == 0xE0) {
        e0_pending = 1;
        return;
    }

    // ---- handle E0-extended keys ----
    if (e0_pending) {
        e0_pending = 0;

        int released = sc & 0x80;
        u8 code = sc & 0x7F;

        if (released) return;   // ignore all E0 releases for now

        switch (code) {
        case 0x48: push_char(KEY_UP);    return;   // Up
        case 0x50: push_char(KEY_DOWN);  return;   // Down
        case 0x4B: push_char(KEY_LEFT);  return;   // Left
        case 0x4D: push_char(KEY_RIGHT); return;   // Right
        case 0x47: push_char(KEY_HOME);  return;   // Home
        case 0x4F: push_char(KEY_END);   return;   // End
        case 0x49: push_char(KEY_PGUP);  return;   // Page Up
        case 0x51: push_char(KEY_PGDN);  return;   // Page Down
        case 0x52: push_char(KEY_INSERT);return;   // Insert
        case 0x53: push_char(KEY_DELETE);return;   // Delete
        case 0x5B: push_char(KEY_WIN_L); return;   // Left Win
        case 0x5C: push_char(KEY_WIN_R); return;   // Right Win
        case 0x5D: push_char(KEY_MENU);  return;   // Menu
        // ---- multimedia keys (Wired Keyboard 600) ----
        case 0x20: push_char(KEY_MUTE);   return;  // Mute
        case 0x2E: push_char(KEY_VOL_DOWN);return; // Volume Down
        case 0x30: push_char(KEY_VOL_UP);  return; // Volume Up
        case 0x22: push_char(KEY_PLAY);    return; // Play/Pause
        case 0x19: push_char(KEY_NEXT);    return; // Next Track
        case 0x10: push_char(KEY_PREV);    return; // Previous Track
        case 0x24: push_char(KEY_STOP);    return; // Stop
        case 0x21: push_char(KEY_CALC);    return; // Calculator
        default:
            return;
        }
    }

    // ---- normal (non-E0) scancodes ----
    int released = sc & 0x80;
    u8 code = sc & 0x7F;

    // modifiers
    if (code == 0x2A) { shift_l = !released; return; }   // LShift
    if (code == 0x36) { shift_r = !released; return; }   // RShift
    if (code == 0x3A) { if (!released) caps = !caps; return; }  // CapsLock
    if (code == 0x1D) { ctrl = !released; return; }      // LCtrl
    if (code == 0x38) { alt  = !released; return; }      // LAlt

    if (released) return;

    int shifted = (shift_l || shift_r);
    char c;

    if (is_alpha(map_normal[code])) {
        int upper = shifted ^ caps;
        c = upper ? map_shift[code] : map_normal[code];
    } else {
        c = shifted ? map_shift[code] : map_normal[code];
    }

    if (c != 0) push_char(c);
}

int keyboard_getchar(void) {
    if (kbd_head == kbd_tail) return -1;
    char c = kbd_buf[kbd_tail];
    asm volatile("" ::: "memory");
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return (int)(unsigned char)c;
}

int keyboard_has_data(void) {
    return kbd_head != kbd_tail;
}
