; isr.asm - interrupt and exception handlers
section .text

global isr0
global isr13
global isr14
global isr_default
global isr_timer
global isr_keyboard

extern exception_handler
extern timer_handler
extern keyboard_handler

; ------------------------------------------------------------------
; Exception 0: Divide by zero. CPU does NOT push error code.
; We push a dummy 0 to keep uniform stack layout.
; ------------------------------------------------------------------
isr0:
    push 0              ; dummy error code
    pusha
    push 0              ; exception number
    call exception_handler
    add esp, 4
    popa
    add esp, 4          ; remove dummy error code
    iret

; ------------------------------------------------------------------
; Exception 13: General Protection Fault. CPU pushes error code.
; ------------------------------------------------------------------
isr13:
    pusha
    push 13
    call exception_handler
    add esp, 4
    popa
    add esp, 4          ; remove CPU-pushed error code
    iret

; ------------------------------------------------------------------
; Exception 14: Page Fault. CPU pushes error code.
; ------------------------------------------------------------------
isr14:
    pusha
    push 14
    call exception_handler
    add esp, 4
    popa
    add esp, 4          ; remove CPU-pushed error code
    iret

; ------------------------------------------------------------------
; Default handler for all other vectors.
; Marker 0xFFFFFFFF tells C handler that the exception is unknown.
; We push a dummy error code to keep stack layout uniform (CPU may or
; may not have pushed one, depending on the exception).
; ------------------------------------------------------------------
isr_default:
    push 0              ; dummy error code
    pusha
    push 0xFFFFFFFF     ; marker: unknown exception
    call exception_handler
    add esp, 4
    popa
    add esp, 4
    iret

; ------------------------------------------------------------------
; IRQ0: timer
; ------------------------------------------------------------------
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

; ------------------------------------------------------------------
; IRQ1: keyboard
; ------------------------------------------------------------------
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
