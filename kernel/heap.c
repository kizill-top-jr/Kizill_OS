#include <kernel/heap.h>
#include <kernel/pmm.h>

// block header, 16 bytes aligned
// 16 bytes on x86_64: size(8) + next(8) + free+pad(8)
struct block {
    u64 size;              // payload size (not counting header)
    struct block *next;    // next block in the list
    u64 free;              // 1 = free, 0 = used
};

static struct block *head = 0;
static u64 total_bytes = 0;
static u64 used_bytes  = 0;

// minimum payload size for a block to be splittable
#define MIN_SPLIT 32

// align size up to 16 bytes
static inline u64 align16(u64 v) {
    return (v + 15) & ~15ULL;
}

void heap_init(void) {
    // grab one page from pmm
    void *page = pmm_alloc();
    if (!page) return;

    // page is already in HHDM, we can write to it directly
    head = (struct block *)page;
    head->size = PAGE_SIZE - sizeof(struct block);
    head->next = 0;
    head->free = 1;

    total_bytes = head->size;
    used_bytes  = 0;
}

void *kmalloc(u64 size) {
    if (size == 0) return 0;

    u64 want = align16(size);

    struct block *b = head;
    while (b) {
        if (b->free && b->size >= want) {
            // can we split?
            if (b->size >= want + MIN_SPLIT + sizeof(struct block)) {
                struct block *split = (struct block *)((u8 *)b + sizeof(struct block) + want);
                split->size = b->size - want - sizeof(struct block);
                split->next = b->next;
                split->free = 1;

                b->size = want;
                b->next = split;

                total_bytes -= sizeof(struct block);
            }

            b->free = 0;
            used_bytes += b->size;
            return (void *)((u8 *)b + sizeof(struct block));
        }
        b = b->next;
    }

    // no block found -- allocate a new page and chain it
    void *page = pmm_alloc();
    if (!page) return 0;

    struct block *nb = (struct block *)page;
    nb->size = PAGE_SIZE - sizeof(struct block);
    nb->next = 0;
    nb->free = 1;

    // append to end of list
    struct block *tail = head;
    while (tail->next) tail = tail->next;
    tail->next = nb;

    total_bytes += nb->size;

    // retry (recursion would be easier but let's just do it inline)
    if (nb->size >= want) {
        nb->free = 0;
        used_bytes += nb->size;
        return (void *)((u8 *)nb + sizeof(struct block));
    }
    return 0;
}

void kfree(void *ptr) {
    if (!ptr) return;

    // get header from payload pointer
    struct block *b = (struct block *)((u8 *)ptr - sizeof(struct block));
    if (b->free) return;   // double free

    b->free = 1;
    used_bytes -= b->size;

    // coalesce with next if both free
    if (b->next && b->next->free) {
        b->size += sizeof(struct block) + b->next->size;
        b->next = b->next->next;
        total_bytes -= sizeof(struct block);
    }

    // coalesce with previous -- need to walk the list
    // (skipping for now -- O(n) but fine for small heaps)
    struct block *prev = head;
    while (prev && prev->next != b) prev = prev->next;
    if (prev && prev->free) {
        prev->size += sizeof(struct block) + b->size;
        prev->next = b->next;
        total_bytes -= sizeof(struct block);
    }
}

u64 heap_total_bytes(void) { return total_bytes; }
u64 heap_used_bytes(void)  { return used_bytes; }
u64 heap_free_bytes(void)  { return total_bytes - used_bytes; }
