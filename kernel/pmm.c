#include <kernel/pmm.h>
#include <kernel/fb.h>
#include <kernel/limine.h>

extern volatile struct limine_memmap_request   memmap_request;
extern volatile struct limine_hhdm_request     hhdm_request;
extern volatile struct limine_framebuffer_request framebuffer_request;

static u8  *bitmap        = 0;   // 1 bit per page, 1 = used
static u64  total_pages   = 0;   // covers all physical RAM up to max usable
static u64  used_pages    = 0;
static u64  hhdm_offset   = 0;

// set/clear/test a bit
static inline void bitmap_set(u64 idx)   { bitmap[idx >> 3] |=  (1 << (idx & 7)); }
static inline void bitmap_clr(u64 idx)   { bitmap[idx >> 3] &= ~(1 << (idx & 7)); }
static inline int  bitmap_tst(u64 idx)   { return bitmap[idx >> 3] & (1 << (idx & 7)); }

// mark range [base, base+len) as used
static void mark_used(u64 base, u64 len) {
    u64 start = base / PAGE_SIZE;
    u64 end   = (base + len + PAGE_SIZE - 1) / PAGE_SIZE;

    for (u64 i = start; i < end && i < total_pages; i++) {
        if (!bitmap_tst(i)) {
            bitmap_set(i);
            used_pages++;
        }
    }
}

// mark range as free
static void mark_free(u64 base, u64 len) {
    u64 start = base / PAGE_SIZE;
    u64 end   = (base + len) / PAGE_SIZE;

    for (u64 i = start; i < end && i < total_pages; i++) {
        if (bitmap_tst(i)) {
            bitmap_clr(i);
            used_pages--;
        }
    }
}

void pmm_init(void) {
    if (!memmap_request.response || !hhdm_request.response) return;

    struct limine_memmap_response *mm = memmap_request.response;
    hhdm_offset = hhdm_request.response->offset;

    // find highest usable address -> that's our range
    u64 max_addr = 0;
    for (u64 i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type == 0) {   // USABLE
            u64 end = e->base + e->length;
            if (end > max_addr) max_addr = end;
        }
    }

    total_pages = max_addr / PAGE_SIZE;

    // bitmap size in bytes = total_pages / 8, rounded up
    u64 bitmap_bytes = (total_pages + 7) / 8;

    // place bitmap in first usable region that is big enough and above 1 MB
    u64 bitmap_phys = 0;
    for (u64 i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type != 0) continue;

        u64 start = e->base;
        u64 end   = e->base + e->length;

        // must be above 1 MB (don't touch low memory)
        if (start < 0x100000) {
            if (end < 0x100000) continue;
            start = 0x100000;
        }
        if (end - start >= bitmap_bytes) {
            bitmap_phys = start;
            break;
        }
    }

    if (!bitmap_phys) return;   // no place for bitmap — bug

    bitmap = (u8 *)(hhdm_offset + bitmap_phys);

    // clear all bits — assume free
    for (u64 i = 0; i < bitmap_bytes; i++) bitmap[i] = 0;
    used_pages = 0;

    // mark EVERYTHING used first (pessimistic), then free the usable
    for (u64 i = 0; i < total_pages; i++) bitmap_set(i);
    used_pages = total_pages;

    // free usable regions that are above 1 MB (avoid low memory)
    for (u64 i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type != 0) continue;
        u64 base = e->base;
        u64 len  = e->length;
        if (base < 0x100000) {
            if (base + len <= 0x100000) continue;
            len -= (0x100000 - base);
            base = 0x100000;
        }
        mark_free(base, len);
    }

    // now mark used: bitmap itself, kernel, framebuffer
    mark_used(bitmap_phys, bitmap_bytes);

    // kernel physical range: kernel is at virtual 0xFFFFFFFF80000000,
    // and that maps to physical 0 in our case?  Let's just mark everything
    // below __kernel_end_phys as used.
    extern u64 __kernel_end;
    u64 kernel_phys_end = (u64)&__kernel_end - 0xFFFFFFFF80000000ULL;
    mark_used(0, kernel_phys_end);

    // framebuffer — mark its whole range as used
    extern volatile struct limine_framebuffer_request framebuffer_request;
    if (framebuffer_request.response &&
        framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        u64 fb_size = fb->pitch * fb->height;
        // fb->address is virtual in HHDM already, so subtract offset
        u64 fb_phys = (u64)fb->address - hhdm_offset;
        mark_used(fb_phys, fb_size);
    }
}

void *pmm_alloc(void) {
    for (u64 i = 0; i < total_pages; i++) {
        if (!bitmap_tst(i)) {
            bitmap_set(i);
            used_pages++;
            return (void *)(hhdm_offset + i * PAGE_SIZE);
        }
    }
    return 0;   // out of memory
}

void pmm_free(void *page) {
    u64 v = (u64)page;
    if (v < hhdm_offset) return;   // sanity
    u64 idx = (v - hhdm_offset) / PAGE_SIZE;
    if (idx >= total_pages) return;
    if (bitmap_tst(idx)) {
        bitmap_clr(idx);
        used_pages--;
    }
}

u64 pmm_total_pages(void) { return total_pages; }
u64 pmm_used_pages(void)  { return used_pages; }
