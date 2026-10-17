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

    printk_color("=== tests done: ", FB_YELLOW);
    printk_dec(tests_passed);
    printk(" passed, ");
    printk_dec(tests_failed);
    printk(" failed ===\n");
}
