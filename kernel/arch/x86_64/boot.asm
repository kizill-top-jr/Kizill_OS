section .bss
align 16
stack_bottom:
    resb 65536
stack_top:

section .text
global _start
extern kmain

_start:
    mov rsp, stack_top
    and rsp, ~0xF
    call kmain
.hang:
    cli
    hlt
    jmp .hang
