// memory.h - Paging and physical memory allocator interface
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void init_paging(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
