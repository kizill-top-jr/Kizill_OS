#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H

#include <kernel/types.h>

// extended key codes (returned by keyboard_getchar when a special key is pressed)
#define KEY_UP       0x80
#define KEY_DOWN     0x81
#define KEY_LEFT     0x82
#define KEY_RIGHT    0x83
#define KEY_HOME     0x84
#define KEY_END      0x85
#define KEY_PGUP     0x86
#define KEY_PGDN     0x87
#define KEY_INSERT   0x88
#define KEY_DELETE   0x89
#define KEY_WIN_L    0x8A
#define KEY_WIN_R    0x8B
#define KEY_MENU     0x8C

#define KEY_VOL_UP   0x90
#define KEY_VOL_DOWN 0x91
#define KEY_MUTE     0x92
#define KEY_PLAY     0x93
#define KEY_NEXT     0x94
#define KEY_PREV     0x95
#define KEY_STOP     0x96
#define KEY_CALC     0x97

void  keyboard_init(void);
void  keyboard_handler(void);
int   keyboard_getchar(void);        // returns ASCII or KEY_* code, -1 if empty
int   keyboard_has_data(void);

#endif
