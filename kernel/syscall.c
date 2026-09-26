#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/task.h>

#define SYS_WRITE  1
#define SYS_GETPID 39
#define SYS_EXIT   60
#define SYS_WAIT   61

struct syscall_frame {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 vector, errcode;
    u64 rip, cs, rflags, rsp, ss;
};

// non-blocking wait: returns exit code if any dead child, else -1.
// user loop: while ((r = wait()) == -1) {}
u64 syscall_dispatch(struct syscall_frame *f) {
    switch (f->rax) {
    case SYS_WRITE: {
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
        return (u64)f;
    }

    case SYS_GETPID:
        f->rax = (u64)task_current_pid();
        return (u64)f;

    case SYS_WAIT: {
        int code = task_check_dead_child();
        if (code >= 0) {
            f->rax = (u64)code;
        } else {
            f->rax = (u64)-1;
        }
        return (u64)f;
    }

    case SYS_EXIT:
        return task_exit_current((int)f->rdi);

    default:
        printk_color("[ring3] unknown syscall\n", FB_RED);
        f->rax = (u64)-1;
        return (u64)f;
    }
}
