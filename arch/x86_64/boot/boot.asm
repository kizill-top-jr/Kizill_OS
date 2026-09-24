section .text
global _start
extern kmain
extern __bss_start
extern __bss_end

_start:
    cli
    mov rsp, stack_top
    and rsp, ~0xF

    ; enable SSE (Limine doesn't do it)
    mov rax, cr0
    and ax, 0xFFFB        ; clear CR0.EM (bit 2) — no FPU emulation
    or  ax, 0x2           ; set   CR0.MP (bit 1) — monitor coprocessor
    mov cr0, rax

    mov rax, cr4
    or  ax, (3 << 9)      ; set CR4.OSFXSR (bit 9) + CR4.OSXMMEXCPT (bit 10)
    mov cr4, rax

    ; zero .bss — Limine doesn't guarantee it
    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    xor eax, eax
    rep stosb

    call kmain
.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 65536
stack_top:
