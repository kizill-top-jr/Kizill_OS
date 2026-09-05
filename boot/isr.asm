; boot/isr.asm
section .text
global isr0
global isr_timer
global isr_keyboard
extern timer_handler
extern keyboard_handler

isr0:
    pusha
    mov al, 0x20
    out 0x20, al
    out 0xA0, al
    popa
    iret

isr_timer:
    pusha
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    call timer_handler
    mov al, 0x20
    out 0x20, al
    out 0xA0, al
    popa
    iret

isr_keyboard:
    pusha
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    call keyboard_handler
    mov al, 0x20
    out 0x20, al
    out 0xA0, al
    popa
    iret
