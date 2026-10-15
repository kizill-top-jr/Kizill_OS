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
        g_master_pml4 = cr3 & 0x000FFFFFFFFFF000ULL;
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
    u64 phys = pte & 0x000FFFFFFFFFF000ULL;
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
            free_user_page(e);
        } else if (level == 2) {
            if (e & PTE_HUGE) {
                // 2MB page, free all 512 small pages inside
                u64 phys = e & 0x000FFFFFFFE00000ULL;
                for (int j = 0; j < 512; j++) {
                    pmm_free((void *)(hhdm() + phys + j * 0x1000));
                }
            } else {
                u64 child = e & 0x000FFFFFFFFFF000ULL;
                free_table_level(child, 1);
                pmm_free((void *)(hhdm() + child));
            }
        } else if (level == 3) {
            u64 child = e & 0x000FFFFFFFFFF000ULL;
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
    if ((cur & 0x000FFFFFFFFFF000ULL) == pml4_phys) return;

    u64 *pml4 = (u64 *)(hhdm() + pml4_phys);

    // walk user entries 0..255 only.
    // kernel entries 256..511 are SHARED -- do NOT touch.
    for (int i = 0; i < 256; i++) {
        u64 e = pml4[i];
        if (!(e & PTE_PRESENT)) continue;

        u64 pdpt = e & 0x000FFFFFFFFFF000ULL;
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
        return (u64 *)(hhdm() + (entry & 0x000FFFFFFFFFF000ULL));
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

    pt[i1] = (phys & 0x000FFFFFFFFFF000ULL)
           | (flags & 0xFFF)
           | PTE_PRESENT
           | (flags & PTE_NX);

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
    u64 *pdpt = (u64 *)(hhdm() + (e4 & 0x000FFFFFFFFFF000ULL));

    u64 e3 = pdpt[i3]; if (!(e3 & PTE_PRESENT)) return 0;
    u64 *pd = (u64 *)(hhdm() + (e3 & 0x000FFFFFFFFFF000ULL));

    u64 e2 = pd[i2]; if (!(e2 & PTE_PRESENT)) return 0;
    if (e2 & PTE_HUGE) return (e2 & 0x000FFFFFFFE00000ULL) + (virt & 0x1FFFFF);

    u64 *pt = (u64 *)(hhdm() + (e2 & 0x000FFFFFFFFFF000ULL));
    u64 e1 = pt[i1]; if (!(e1 & PTE_PRESENT)) return 0;

    return (e1 & 0x000FFFFFFFFFF000ULL) + off;
}

u64 virt_to_phys(u64 virt) {
    return virt_to_phys_in(0, virt);
}
// helper: allocate a zeroed page table, returns its hhdm pointer and phys
static u64 *alloc_table(u64 *out_phys) {
    void *page = pmm_alloc();
    if (!page) return 0;
    u64 phys = (u64)page - hhdm();
    u64 *t = (u64 *)page;
    for (int i = 0; i < 512; i++) t[i] = 0;
    *out_phys = phys;
    return t;
}

// helper: deep-copy one level of tables (PDPT=3, PD=2, PT=1)
// leaf pages (level 1) are shared with COW flag, not copied.
static u64 copy_table_level(u64 src_phys, int level) {
    u64 dst_phys = 0;
    u64 *dst = alloc_table(&dst_phys);
    if (!dst) return 0;

    u64 *src = (u64 *)(hhdm() + src_phys);

    for (int i = 0; i < 512; i++) {
        u64 e = src[i];
        if (!(e & PTE_PRESENT)) continue;

        if (level == 1) {
            // LEAF: real copy, no COW
            u64 old_phys = e & 0x000FFFFFFFFFF000ULL;
            void *new_page = pmm_alloc();
            if (!new_page) return 0;
            u64 new_phys = (u64)new_page - hhdm();

            u8 *dst_page = (u8 *)new_page;
            u8 *src_page = (u8 *)(old_phys + hhdm());
            for (u64 j = 0; j < 4096; j++) dst_page[j] = src_page[j];

            // keep original flags (W if it was W), drop COW, keep NX
            u64 flags = e & 0xFFF;
            flags &= ~(u64)PTE_COW;
            u64 nx = e & PTE_NX;

            dst[i] = new_phys | flags | PTE_PRESENT | nx;
        } else {
            // INTERMEDIATE: recurse, deep copy
            u64 child_src = e & 0x000FFFFFFFFFF000ULL;
            u64 child_dst = copy_table_level(child_src, level - 1);
            if (!child_dst) return 0;

            // preserve USER flag, drop WRITE on intermediates
            u64 flags = e & 0xFFF;
            flags &= ~(u64)PTE_WRITE;
            dst[i] = child_dst | flags | PTE_PRESENT;
        }
    }

    return dst_phys;
}
u64 pml4_copy_for_fork(u64 src_pml4) {
    if (!src_pml4) return 0;

    u64 dst_phys = 0;
    u64 *dst = alloc_table(&dst_phys);
    if (!dst) return 0;

    u64 *src = (u64 *)(hhdm() + src_pml4);

    // 1. kernel half: shallow copy
    for (int i = 256; i < 512; i++) {
        dst[i] = src[i];
    }

    // 2. user half: deep copy with CoW on leaves
    for (int i = 0; i < 256; i++) {
        u64 e = src[i];
        if (!(e & PTE_PRESENT)) continue;

        u64 pdpt_src = e & 0x000FFFFFFFFFF000ULL;
        u64 pdpt_dst = copy_table_level(pdpt_src, 3);
        if (!pdpt_dst) return 0;

        u64 flags = e & 0xFFF;
        flags &= ~(u64)PTE_WRITE;
        dst[i] = pdpt_dst | flags | PTE_PRESENT;
    }

    return dst_phys;
}
u64 *pte_for_va(u64 pml4_phys, u64 va) {
    u64 *pml4 = pml4_ptr(pml4_phys);

    u64 i4 = (va >> 39) & 0x1FF;
    u64 i3 = (va >> 30) & 0x1FF;
    u64 i2 = (va >> 21) & 0x1FF;
    u64 i1 = (va >> 12) & 0x1FF;

    u64 e4 = pml4[i4]; if (!(e4 & PTE_PRESENT)) return 0;
    u64 *pdpt = (u64 *)(hhdm() + (e4 & 0x000FFFFFFFFFF000ULL));

    u64 e3 = pdpt[i3]; if (!(e3 & PTE_PRESENT)) return 0;
    u64 *pd = (u64 *)(hhdm() + (e3 & 0x000FFFFFFFFFF000ULL));

    u64 e2 = pd[i2]; if (!(e2 & PTE_PRESENT)) return 0;
    if (e2 & PTE_HUGE) return &pd[i2];   // 2MB huge page the PTE is the PD entry itself

    u64 *pt = (u64 *)(hhdm() + (e2 & 0x000FFFFFFFFFF000ULL));
    u64 e1 = pt[i1]; if (!(e1 & PTE_PRESENT)) return 0;

    return &pt[i1];
}
int validate_user_ptr(u64 pml4_phys, u64 ptr, u64 len, int write) {
    // reject empty or kernel-half addresses
    if (len == 0) return 0;

    // user half: below 0x0000800000000000
    const u64 USER_MAX = 0x0000800000000000ULL;

    if (ptr >= USER_MAX) return -1;

    // overflow check on ptr + len
    if (ptr + len < ptr) return -1;
    if (ptr + len > USER_MAX) return -1;

    // walk every page in the range
    u64 start = ptr & ~0xFFFULL;
    u64 end   = (ptr + len + 0xFFF) & ~0xFFFULL;

    for (u64 va = start; va < end; va += 0x1000) {
        u64 *pte = pte_for_va(pml4_phys, va);
        if (!pte) return -1;
        if (!(*pte & PTE_PRESENT)) return -1;
        if (!(*pte & PTE_USER))    return -1;
        if (write && !(*pte & PTE_WRITE)) return -1;
    }

    return 0;
}
u64 hhdm_offset(void) {
    return hhdm_request.response
        ? hhdm_request.response->offset
        : HHDM_BASE;
}
