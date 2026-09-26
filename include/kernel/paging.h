#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <kernel/types.h>

// flags
#define PTE_PRESENT   (1ULL << 0)
#define PTE_WRITE     (1ULL << 1)
#define PTE_USER      (1ULL << 2)
#define PTE_HUGE      (1ULL << 7)

// map one 4 KiB page: virt -> phys with flags
// returns 0 on success, -1 on failure
int map_page(u64 virt, u64 phys, u64 flags);

// get physical address of a given virt, or 0 if not mapped
u64 virt_to_phys(u64 virt);

#endif
