; boot/start.asm - Multiboot header, GDT, Protected Mode, Paging helper
section .multiboot
align 4
    dd 0x1BADB002          ; multiboot magic
    dd 0x00                ; flags
    dd -(0x1BADB002 + 0x00); checksum

section .text
global _start
extern kmain

_start:
    ; set up stack
    mov esp, 0x90000

    ; load GDT
    lgdt [gdt_descriptor]

    ; switch to protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; far jump to code segment
    jmp 0x08:protected_mode

[bits 32]
protected_mode:
    ; load data segment selector
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; call main kernel
    call kmain

    ; halt if returns
    cli
    hlt

; GDT
gdt_start:
    dd 0x00000000
    dd 0x00000000
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0b10011010
    db 0b11001111
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0b10010010
    db 0b11001111
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ------------------------------------------------------------------
; enable_paging(uint32_t* page_directory)
; Called from C. Loads CR3 with page directory address,
; then sets CR0.PG bit to enable paging.
; CRITICAL: The code itself must be in an identity-mapped region,
; which is why we enable paging here in assembly, not in C.
; ------------------------------------------------------------------
global enable_paging
enable_paging:
    mov eax, [esp + 4]   ; page directory address (argument)
    mov cr3, eax         ; load into CR3

    mov eax, cr0
    or eax, 0x80000000   ; set PG bit (bit 31)
    mov cr0, eax         ; enable paging

    ret                  ; return to C (paging is now active)
