#ifndef KERNEL_EXCEPTION_H
#define KERNEL_EXCEPTION_H

#include <kernel/types.h>

// stack layout after ISR stub + isr_common pushes.
// see isr.asm for exact push order.
struct exception_frame {
    // saved by isr_common
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    // pushed by stub
    u64 vector;
    u64 errcode;
    // pushed by CPU
    u64 rip;
    u64 cs;
    u64 rflags;
    // only present on ring3 -> ring0 transition
    u64 rsp;
    u64 ss;
};

// returns new rsp (may be another task's). never returns if kernel panic.
u64 exception_dispatch(struct exception_frame *f);

#endif
