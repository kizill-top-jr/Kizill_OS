#include <kernel/elf.h>
#include <kernel/paging.h>
#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/pmm.h>
#include <kernel/heap.h>

static int tests_passed = 0;
static int tests_failed = 0;

static void check(int cond, const char *name) {
    if (cond) {
        printk_color("[ktest] OK   ", FB_GREEN);
        printk(name);
        printk("\n");
        tests_passed++;
    } else {
        printk_color("[ktest] FAIL ", FB_RED);
        printk(name);
        printk("\n");
        tests_failed++;
    }
}

void ktest_run(void) {
    printk_color("=== kernel tests ===\n", FB_YELLOW);

    // test 1: pmm alloc and free
    void *p1 = pmm_alloc();
    void *p2 = pmm_alloc();
    check(p1 != 0, "pmm_alloc returns non-null");
    check(p1 != p2, "pmm_alloc returns different pages");

    pmm_free(p1);
    pmm_free(p2);

    // test 2: kmalloc basic
    char *buf = (char *)kmalloc(64);
    check(buf != 0, "kmalloc(64) returns non-null");
    if (buf) {
        buf[0] = 'A';
        buf[63] = 'Z';
        check(buf[0] == 'A' && buf[63] == 'Z', "kmalloc memory is writable");
        kfree(buf);
    }

    // test 3: kmalloc multiple
    void *a = kmalloc(128);
    void *b = kmalloc(128);
    check(a != 0 && b != 0, "kmalloc multiple allocations");
    check(a != b, "kmalloc returns different addresses");
    kfree(a);
    kfree(b);

    // test 7: pml4_create returns valid phys addr
    u64 test_pml4 = pml4_create();
    check(test_pml4 != 0, "pml4_create returns non-null");

    if (test_pml4) {
        // test 8: map_page_in and virt_to_phys_in
        void *phys_page = pmm_alloc();
        u64 virt_addr = 0x10000000; // какой-то левый адрес в юзерспейсе

        if (phys_page) {
            u64 phys_addr = (u64)phys_page - hhdm_offset();
            int map_res = map_page_in(test_pml4, virt_addr, phys_addr, PTE_PRESENT | PTE_WRITE | PTE_USER);

            check(map_res == 0, "map_page_in succeeds");

            u64 mapped_phys = virt_to_phys_in(test_pml4, virt_addr);
            check(mapped_phys == phys_addr, "virt_to_phys_in returns correct phys addr");

            pmm_free(phys_page);
        } else {
            check(0, "pmm_alloc failed for paging test");
        }

        // test 9: pml4_destroy frees without crashing
        pml4_destroy(test_pml4);
        check(1, "pml4_destroy completes");
    }

    // test 10: heap stress (many allocations, write, verify, free)
    void *ptrs[10];
    int heap_ok = 1;

    for (int i = 0; i < 10; i++) {
        ptrs[i] = kmalloc(32);
        if (!ptrs[i]) { heap_ok = 0; break; }
        *(int *)ptrs[i] = i + 100;
    }

    if (heap_ok) {
        for (int i = 0; i < 10; i++) {
            if (*(int *)ptrs[i] != i + 100) {
                heap_ok = 0;
                break;
            }
        }
    }

    for (int i = 0; i < 10; i++) {
        if (ptrs[i]) kfree(ptrs[i]);
    }
    check(heap_ok, "heap stress test (10 blocks)");

    // test 11: ELF rejects garbage
    u8 bad_elf[64];
    for (int i = 0; i < 64; i++) bad_elf[i] = 0xAA;
    int elf_res = elf_check(bad_elf, 64);
    check(elf_res != 0, "elf_check rejects garbage");

    // test 12: ELF rejects truncated file
    u8 bad_elf2[64];
    for (int i = 0; i < 64; i++) bad_elf2[i] = 0;
    bad_elf2[0] = 0x7F;
    bad_elf2[1] = 'E';
    bad_elf2[2] = 'L';
    bad_elf2[3] = 'F';
    bad_elf2[4] = 2; // 64-bit
    bad_elf2[5] = 1; // little endian
    elf_res = elf_check(bad_elf2, 64);
    check(elf_res != 0, "elf_check rejects missing phdrs");

    printk_color("=== tests done: ", FB_YELLOW);
    printk_dec(tests_passed);
    printk(" passed, ");
    printk_dec(tests_failed);
    printk(" failed ===\n");
}
