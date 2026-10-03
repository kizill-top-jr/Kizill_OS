#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <kernel/types.h>

// flags
#define PTE_PRESENT  (1ULL << 0)
#define PTE_WRITE    (1ULL << 1)
#define PTE_USER     (1ULL << 2)
#define PTE_HUGE     (1ULL << 7)
#define PTE_NX       (1ULL << 63)

// create new PML4 -- copies kernel entries 256..511 from master.
// returns physical address of PML4, or 0 on failure.
u64 pml4_create(void);

// free user entries of a PML4, then the PML4 page itself.
// never touches entries 256..511.
void pml4_destroy(u64 pml4_phys);

// get physical address of master PML4 (current CR3)
u64 pml4_master(void);

// map one page into a specific PML4.
// pml4_phys == 0 -> use current CR3
int map_page_in(u64 pml4_phys, u64 virt, u64 phys, u64 flags);

// map into current PML4 (wrapper)
int map_page(u64 virt, u64 phys, u64 flags);

// get physical address of virt in the given PML4 (0 = current)
u64 virt_to_phys_in(u64 pml4_phys, u64 virt);
u64 virt_to_phys(u64 virt);

#endif
