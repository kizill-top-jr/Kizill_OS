#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/limine.h>

extern volatile struct limine_hhdm_request hhdm_request;

#define HHDM_BASE 0xFFFF800000000000ULL

static inline u64 hhdm(void) {
    return hhdm_request.response ? hhdm_request.response->offset : HHDM_BASE;
}

static u64 g_master_pml4 = 0;

u64 pml4_master(void) {
    if (g_master_pml4 == 0) {
        u64 cr3;
        asm volatile("mov %%cr3, %0" : "=r"(cr3));
        g_master_pml4 = cr3 & ~0xFFFULL;
    }
    return g_master_pml4;
}

static u64 *pml4_ptr(u64 pml4_phys) {
    if (pml4_phys == 0) pml4_phys = pml4_master();
    return (u64 *)(hhdm() + pml4_phys);
}

u64 pml4_create(void) {
    void *page = pmm_alloc();
    if (!page) return 0;

    u64 new_phys = (u64)page - hhdm();
    u64 *new_pml4 = (u64 *)(hhdm() + new_phys);
    u64 *master   = pml4_ptr(0);

    // zero all
    for (int i = 0; i < 512; i++) new_pml4[i] = 0;

    // copy kernel half -- entries 256..511
    for (int i = 256; i < 512; i++) new_pml4[i] = master[i];

    return new_phys;
}

// free a leaf page table entry -- the phys page
static void free_user_page(u64 pte) {
    if (!(pte & PTE_PRESENT)) return;
    u64 phys = pte & ~0xFFFULL;
    if (phys == 0) return;
    pmm_free((void *)(hhdm() + phys));
}

// free a page table (512 entries) and all its children.
// level 1 = PT (leaves), level 2 = PD, level 3 = PDPT.
static void free_table_level(u64 table_phys, int level) {
    u64 *table = (u64 *)(hhdm() + table_phys);

    for (int i = 0; i < 512; i++) {
        u64 e = table[i];
        if (!(e & PTE_PRESENT)) continue;

        if (level == 1) {
            // leaf
            free_user_page(e);
        } else if (level == 2) {
            // could be 2MB huge page
            if (e & PTE_HUGE) {
                u64 phys = e & ~0x1FFFFFULL;
                // huge page is 2MB = 512 small pages
                for (int j = 0; j < 512; j++) {
                    pmm_free((void *)(hhdm() + phys + j * 0x1000));
                }
            } else {
                u64 child = e & ~0xFFFULL;
                free_table_level(child, 1);
                pmm_free((void *)(hhdm() + child));
            }
        } else if (level == 3) {
            u64 child = e & ~0xFFFULL;
            free_table_level(child, 2);
            pmm_free((void *)(hhdm() + child));
        }
    }
}

void pml4_destroy(u64 pml4_phys) {
    if (!pml4_phys) return;

    // safety: don't destroy the PML4 currently loaded
    u64 cur;
    asm volatile("mov %%cr3, %0" : "=r"(cur));
    if ((cur & ~0xFFFULL) == pml4_phys) return;

    u64 *pml4 = (u64 *)(hhdm() + pml4_phys);

    // walk user entries 0..255 only.
    // kernel entries 256..511 are SHARED -- do NOT touch.
    for (int i = 0; i < 256; i++) {
        u64 e = pml4[i];
        if (!(e & PTE_PRESENT)) continue;

        u64 pdpt = e & ~0xFFFULL;
        free_table_level(pdpt, 3);
        pmm_free((void *)(hhdm() + pdpt));
        pml4[i] = 0;
    }

    // finally the PML4 page itself
    pmm_free((void *)(hhdm() + pml4_phys));
}

// same, but takes a pointer to the table directly
static u64 *walk_ptr(u64 *table, u64 idx, int create, u64 flags) {
    u64 entry = table[idx];

    if (entry & PTE_PRESENT) {
        return (u64 *)(hhdm() + (entry & ~0xFFFULL));
    }

    if (!create) return 0;

    void *page = pmm_alloc();
    if (!page) return 0;

    u64 phys = (u64)page - hhdm();
    table[idx] = phys | PTE_PRESENT | PTE_WRITE | (flags & PTE_USER);

    u64 *new_table = (u64 *)(hhdm() + phys);
    for (int i = 0; i < 512; i++) new_table[i] = 0;

    return new_table;
}

int map_page_in(u64 pml4_phys, u64 virt, u64 phys, u64 flags) {
    u64 *pml4 = pml4_ptr(pml4_phys);

    u64 i4 = (virt >> 39) & 0x1FF;
    u64 i3 = (virt >> 30) & 0x1FF;
    u64 i2 = (virt >> 21) & 0x1FF;
    u64 i1 = (virt >> 12) & 0x1FF;

    u64 *pdpt = walk_ptr(pml4, i4, 1, flags); if (!pdpt) return -1;
    u64 *pd   = walk_ptr(pdpt, i3, 1, flags); if (!pd)   return -1;
    u64 *pt   = walk_ptr(pd,   i2, 1, flags); if (!pt)   return -1;

    pt[i1] = (phys & ~0xFFFULL) | (flags & 0xFFF) | PTE_PRESENT;

    // flush TLB only if we're mapping into the active PML4
    if (pml4_phys == 0 || pml4_phys == pml4_master()) {
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    }

    return 0;
}

int map_page(u64 virt, u64 phys, u64 flags) {
    return map_page_in(0, virt, phys, flags);
}

u64 virt_to_phys_in(u64 pml4_phys, u64 virt) {
    u64 *pml4 = pml4_ptr(pml4_phys);

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

u64 virt_to_phys(u64 virt) {
    return virt_to_phys_in(0, virt);
}
