#include <kernel/serial.h>
#include <kernel/fb.h>
#include <kernel/types.h>
#include <kernel/exception.h>
#include <kernel/task.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/limine.h>

extern u64   task_current_cr3(void);
extern u64  *pte_for_va(u64 pml4_phys, u64 va);
extern void *pmm_alloc(void);
extern void  pmm_ref_dec(void *page);
extern volatile struct limine_hhdm_request hhdm_request;

static const char *names[] = {
    "Divide error", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound range", "Invalid opcode", "Device n/a",
    "Double fault", "Coprocessor", "Invalid TSS", "Segment not present",
    "Stack fault", "General protection", "Page fault", "Reserved",
    "x87 FPU", "Alignment", "Machine check", "SIMD FPU",
    "Virtualization", "Control protection",
};

// kernel-mode panic -- never returns
static void kernel_panic(struct exception_frame *f) {
    fb_clear(0x00C00000);

    printk_color("KERNEL PANIC: ", FB_WHITE);
    u64 vec = f->vector;
    if (vec < 22) printk(names[vec]);
    else          printk("Unknown");
    printk("  (vector "); printk_dec(vec); printk(")\n");

    printk("rip: "); printk_hex(f->rip);
    printk("  cs: ");  printk_hex(f->cs);
    printk("  err: "); printk_hex(f->errcode);
    printk("\n");

    if (vec == 14) {
        u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        printk("fault addr: "); printk_hex(cr2); printk("\n");
    }

    for (;;) asm volatile("cli; hlt");
}

u64 exception_dispatch(struct exception_frame *f) {
    // DEBUG: log CR4 once
    static int cr4_logged = 0;
    if (!cr4_logged) {
        u64 cr4;
        asm volatile("mov %%cr4, %0" : "=r"(cr4));
        serial_puts("CR4=");
        serial_hex(cr4);
        serial_puts("\n");
        cr4_logged = 1;
    }
    asm volatile("cli" ::: "memory");
    u64 vec = f->vector;

    // rpl bits of CS: 0 = kernel, 3 = user
    int from_user = (f->cs & 3) == 3;

    // #DF is always fatal -- can't recover
    if (vec == 8) {
        fb_clear(0x00C00000);
        printk_color("DOUBLE FAULT -- kernel bug or bad IDT\n", FB_WHITE);
        printk("rip: "); printk_hex(f->rip); printk("\n");
        printk("err: "); printk_hex(f->errcode); printk("\n");
        for (;;) asm volatile("cli; hlt");
    }

    // ---- CoW page fault handling ----
    if (vec == 14 && from_user) {
        int is_write   = (f->errcode & 2) != 0;
        int is_present = (f->errcode & 1) != 0;

        u64 cr2_dbg;
        asm volatile("mov %%cr2, %0" : "=r"(cr2_dbg));

        extern int task_current_pid(void);
        serial_puts("PF user: pid=");
        serial_dec((u64)task_current_pid());
        serial_puts(" err=");
        serial_hex(f->errcode);
        serial_puts(" rip=");
        serial_hex(f->rip);
        serial_puts(" cr2=");
        serial_hex(cr2_dbg);
        serial_puts(" w=");
        serial_dec((u64)is_write);
        serial_puts(" p=");
        serial_dec((u64)is_present);
        serial_puts("\n");

        if (is_write && is_present) {
            u64 cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));

            u64 cur_pml4 = task_current_cr3();
            u64 cr3_actual;
            asm volatile("mov %%cr3, %0" : "=r"(cr3_actual));
            cr3_actual &= 0x000FFFFFFFFFF000ULL;
            serial_puts("CoW: cur_pml4=");
            serial_hex(cur_pml4);
            serial_puts(" cr3_actual=");
            serial_hex(cr3_actual);
            serial_puts("\n");
            u64 *pte = pte_for_va(cur_pml4, cr2);

            serial_puts("CoW: pml4=");
            serial_hex(cur_pml4);
            serial_puts(" pte=");
            serial_hex((u64)pte);
            if (pte) {
                serial_puts(" *pte=");
                serial_hex(*pte);
                serial_puts(" cow=");
                serial_dec((*pte & PTE_COW) ? 1 : 0);
            }
            serial_puts("\n");

            if (pte && (*pte & PTE_COW)) {
                u64 old_phys = *pte & 0x000FFFFFFFFFF000ULL;
                void *new_page = pmm_alloc();

                if (new_page) {
                    u64 hhdm = hhdm_request.response->offset;
                    u8 *dst = (u8 *)new_page;
                    u8 *src = (u8 *)(old_phys + hhdm);
                    for (u64 i = 0; i < 4096; i++) dst[i] = src[i];

                    u64 new_phys = (u64)new_page - hhdm;
                    u64 nx = *pte & PTE_NX;

                    *pte = new_phys | PTE_PRESENT | PTE_WRITE | PTE_USER | nx;

                    pmm_ref_dec((void *)(old_phys + hhdm));

                    // NUCLEAR TLB FLUSH: bounce through master PML4
                    extern u64 pml4_master(void);
                    u64 master_cr3 = pml4_master();
                    asm volatile("mov %0, %%cr3" : : "r"(master_cr3) : "memory");
                    asm volatile("mov %0, %%cr3" : : "r"(cur_pml4)   : "memory");

                    serial_puts("CoW: flushed, returning\n");

                    return (u64)f;
                }
                // out of memory -> kill user below
            }
        }
    }

    if (from_user) {
        // log and kill the task, don't panic
        printk_color("[fault] killed user task: ", FB_YELLOW);
        if (vec < 22) printk(names[vec]); else printk("unknown");
        printk("  vec="); printk_dec(vec);
        printk("  rip="); printk_hex(f->rip);
        printk("\n");

        u64 rsp = task_exit_current(128 + (int)vec);
        return rsp;
    }

    // kernel-mode exception -> unrecoverable
    kernel_panic(f);
    return 0;  // unreachable
}
