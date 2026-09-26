#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H

#include <kernel/types.h>

void  keyboard_init(void);
void  keyboard_handler(void);
int   keyboard_getchar(void);        // non-blocking, -1 if empty
int   keyboard_has_data(void);

#endif
