section .text

global gdt_load
global tss_load

; void gdt_load(struct gdt_ptr *ptr)
gdt_load:
    lgdt [rdi]

    ; reload CS via far return
    push 0x08
    lea rax, [rel .reload]
    push rax
    retfq

.reload:
    ; reload data segment selectors
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

; void tss_load(void)
tss_load:
    mov ax, 0x28    ; selector: slot 5 * 8
    ltr ax
    ret
