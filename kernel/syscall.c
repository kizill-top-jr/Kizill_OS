#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/task.h>

// syscall numbers
#define SYS_WRITE 1
#define SYS_EXIT  60

// this struct must match what isr_stub_syscall pushes on the stack
struct syscall_frame {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 vector, errcode;
    u64 rip, cs, rflags, rsp, ss;
};

// return value in rax
void syscall_dispatch(struct syscall_frame *f) {
    u64 num = f->rax;

    switch (num) {
    case SYS_WRITE: {
        // rdi = fd, rsi = buf, rdx = len
        // we ignore fd for now, just print buf
        char *buf = (char *)f->rsi;
        u64   len = f->rdx;

        printk_color("[ring3] ", FB_CYAN);
        for (u64 i = 0; i < len; i++) {
            char c = buf[i];
            if (c == '\0') break;
            if (c == '\n') printk("\n");
            else { char s[2] = { c, 0 }; printk(s); }
        }
        f->rax = len;
        break;
    }

    case SYS_EXIT:
        printk_color("\n[ring3] task exited\n", FB_YELLOW);
        // re-enable interrupts so IRQ0 can preempt us and switch away.
        // interrupt gate clears IF on entry; hlt with IF=0 waits forever.
        asm volatile("sti");
        for (;;) asm volatile("hlt");
        break;

    default:
        printk_color("[ring3] unknown syscall\n", FB_RED);
        f->rax = (u64)-1;
        break;
    }
}
