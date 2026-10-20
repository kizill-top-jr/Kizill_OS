#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include <kernel/types.h>

#define PAGE_SIZE 4096

#define PMM_MAX_PAGES (1 << 20)   // 1M pages = 4 GiB; TODO: dynamic for larger

void pmm_init(void);
void pmm_ref_inc(void *page);
void pmm_ref_dec(void *page);
void pmm_free_huge(void *page);
void *pmm_alloc(void);          // returns virtual address (HHDM) of a free page
void  pmm_free(void *page);
u64   pmm_total_pages(void);
u64   pmm_used_pages(void);

#endif
