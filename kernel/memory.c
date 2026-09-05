// memory.c - 32-bit paging with identity mapping + simple bitmap allocator
#include <stdint.h>
#include <stddef.h>
#include "memory.h"

#define PAGE_SIZE       4096
#define TOTAL_MEMORY_MB 16
#define TOTAL_PAGES     (TOTAL_MEMORY_MB * 1024 * 1024 / PAGE_SIZE)
#define BITMAP_SIZE     (TOTAL_PAGES / 8)

// Page tables must be 4KB aligned and placed in BSS to avoid overwriting kernel code
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_table[1024]    __attribute__((aligned(4096)));

static uint8_t bitmap[BITMAP_SIZE];
static uint32_t next_free_page = 0;

// ASM helper - defined in boot/start.asm
extern void enable_paging(uint32_t* page_dir);

void init_paging(void) {
    // Zero out page directory and first page table
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0;
        page_table[i] = 0;
    }

    // Identity map first 4MB (0x00000000 - 0x00400000)
    // Each entry maps one 4KB page: address | PRESENT | WRITABLE
    for (int i = 0; i < 1024; i++) {
        page_table[i] = (i * PAGE_SIZE) | 0x03;  // 0x03 = PRESENT (1) | WRITABLE (2)
    }

    // Page directory entry 0 points to the page table
    page_directory[0] = ((uint32_t)&page_table) | 0x03;

    // Optional: map the page directory itself into the last 4MB (fractal mapping)
    // This allows easy manipulation of page tables later
    page_directory[1023] = ((uint32_t)&page_directory) | 0x03;

    // Load page directory and enable paging
    enable_paging((uint32_t*)page_directory);

    // Mark first 1024 pages (4MB) as used (kernel + page tables)
    for (int i = 0; i < BITMAP_SIZE; i++) bitmap[i] = 0;
    for (int i = 0; i < 1024; i++) {
        bitmap[i >> 3] |= (1 << (i & 7));
    }
    next_free_page = 1024;
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t start = next_free_page;
    uint32_t found = 0;

    for (uint32_t i = next_free_page; i < TOTAL_PAGES; i++) {
        uint32_t byte_idx = i >> 3;
        uint8_t bit_mask = 1 << (i & 7);

        if (!(bitmap[byte_idx] & bit_mask)) {
            found++;
            if (found == pages) {
                // Mark pages as used
                for (uint32_t j = start; j < start + pages; j++) {
                    bitmap[j >> 3] |= (1 << (j & 7));
                }
                next_free_page = start + pages;
                return (void*)(start * PAGE_SIZE);
            }
        } else {
            found = 0;
            start = i + 1;
        }
    }
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;
    uint32_t addr = (uint32_t)ptr;
    uint32_t page = addr / PAGE_SIZE;
    bitmap[page >> 3] &= ~(1 << (page & 7));
}
