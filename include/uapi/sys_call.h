#ifndef UAPI_SYS_CALL_H
#define UAPI_SYS_CALL_H

// basic syscall wrapper for userspace
static inline long sys_call(long n, long a, long b, long c) {
    long ret;
    asm volatile(
        "mov %1, %%rax\n"
        "mov %2, %%rdi\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r"(ret)
        : "r"(n), "r"(a), "r"(b), "r"(c)
        : "rax", "rdi", "rsi", "rdx", "memory"
    );
    return ret;
}

#endif
