#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/limine.h>

extern volatile struct limine_hhdm_request hhdm_request;

static inline u64 hhdm(void) {
    return hhdm_request.response ? hhdm_request.response->offset : 0;
}

static inline u64 *get_pml4(void) {
    u64 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (u64 *)(hhdm() + (cr3 & ~0xFFFULL));
}

// walk one level. returns the entry pointer; if not present and create=1,
// allocates a new page table.
static u64 *walk(u64 *table, u64 idx, int create, u64 flags) {
    u64 entry = table[idx];

    if (entry & PTE_PRESENT) {
        // for the last level (pt), present means we've hit a page; return
        return (u64 *)(hhdm() + (entry & ~0xFFFULL));
    }

    if (!create) return 0;

    void *page = pmm_alloc();
    if (!page) return 0;

    u64 phys = (u64)page - hhdm();
    table[idx] = phys | PTE_PRESENT | PTE_WRITE | (flags & PTE_USER);

    // zero the new page table
    u64 *new_table = (u64 *)(hhdm() + phys);
    for (int i = 0; i < 512; i++) new_table[i] = 0;

    return new_table;
}

int map_page(u64 virt, u64 phys, u64 flags) {
    u64 *pml4 = get_pml4();

    u64 i4 = (virt >> 39) & 0x1FF;
    u64 i3 = (virt >> 30) & 0x1FF;
    u64 i2 = (virt >> 21) & 0x1FF;
    u64 i1 = (virt >> 12) & 0x1FF;

    u64 *pdpt = walk(pml4, i4, 1, flags); if (!pdpt) return -1;
    u64 *pd   = walk(pdpt, i3, 1, flags); if (!pd)   return -1;
    u64 *pt   = walk(pd,   i2, 1, flags); if (!pt)   return -1;

    pt[i1] = (phys & ~0xFFFULL) | (flags & 0xFFF) | PTE_PRESENT;

    // flush TLB for this page
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");

    return 0;
}

u64 virt_to_phys(u64 virt) {
    u64 *pml4 = get_pml4();

    u64 i4 = (virt >> 39) & 0x1FF;
    u64 i3 = (virt >> 30) & 0x1FF;
    u64 i2 = (virt >> 21) & 0x1FF;
    u64 i1 = (virt >> 12) & 0x1FF;
    u64 off = virt & 0xFFF;

    u64 e4 = pml4[i4]; if (!(e4 & PTE_PRESENT)) return 0;
    u64 *pdpt = (u64 *)(hhdm() + (e4 & ~0xFFFULL));

    u64 e3 = pdpt[i3]; if (!(e3 & PTE_PRESENT)) return 0;
    u64 *pd = (u64 *)(hhdm() + (e3 & ~0xFFFULL));

    u64 e2 = pd[i2]; if (!(e2 & PTE_PRESENT)) return 0;
    if (e2 & PTE_HUGE) return (e2 & ~0x1FFFFFULL) + (virt & 0x1FFFFF);

    u64 *pt = (u64 *)(hhdm() + (e2 & ~0xFFFULL));
    u64 e1 = pt[i1]; if (!(e1 & PTE_PRESENT)) return 0;

    return (e1 & ~0xFFFULL) + off;
}
