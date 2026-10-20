// first real user program, compiled as ELF
// raw syscalls, no libc

#include <uapi/sys_call.h>
#include <uapi/syscall.h>

#define SYS_WRITE  1

int main(void) {
    const char *msg = "hello from ELF!\n";
    sys_call(SYS_WRITE, 1, (long)msg, sizeof(msg)-1);
    return 33;
}

// entry point for the ELF binary
void _start(void) {
    asm volatile("andq $-16, %rsp");
    int code = main();
    sys_call(SYS_EXIT, code, 0, 0);
    for (;;) {}
}
