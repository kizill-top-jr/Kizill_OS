#ifndef KERNEL_GDT_H
#define KERNEL_GDT_H

#include <kernel/types.h>

void gdt_init(void);

/* TSS setter — useful later for interrupt stacks */
void tss_set_rsp0(u64 rsp0);

#endif
