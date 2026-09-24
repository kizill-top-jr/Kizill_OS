#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#include <kernel/types.h>

void  heap_init(void);
void *kmalloc(u64 size);
void  kfree(void *ptr);

// diagnostics
u64 heap_total_bytes(void);
u64 heap_used_bytes(void);
u64 heap_free_bytes(void);

#endif
