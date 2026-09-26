section .text

extern exception_handler
extern keyboard_handler
extern scheduler_tick

; exceptions without error code -- push dummy 0 to keep stack uniform
%macro ISR_NOERR 1
global isr_stub_%1
isr_stub_%1:
    push qword 0
    push qword %1
    jmp isr_common
%endmacro

; exceptions where CPU pushes error code itself
%macro ISR_ERR 1
global isr_stub_%1
isr_stub_%1:
    push qword %1
    jmp isr_common
%endmacro

ISR_NOERR 0
ISR_ERR   8
ISR_ERR   13
ISR_ERR   14

global isr_stub_default
isr_stub_default:
    push qword 0
    mov rax, 0xFFFFFFFF
    push rax
    jmp isr_common

; ---- syscall: int 0x80 ----
global isr_stub_syscall
extern syscall_dispatch
isr_stub_syscall:
    push qword 0
    push qword 0x80

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call syscall_dispatch
    mov rsp, rax                ; use returned rsp (may be a different task)

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq

; ---- timer: this one does the switch ----
global isr_stub_timer
isr_stub_timer:
    push qword 0            ; dummy error code
    push qword 32           ; vector

    ; save all regs
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; EOI to master PIC only (IRQ0)
    mov al, 0x20
    out 0x20, al

    ; switch task
    mov rdi, rsp
    call scheduler_tick
    mov rsp, rax

    ; restore all regs (of the new task)
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16             ; drop vector + error code
    iretq

; ---- keyboard: normal dispatch ----
global isr_stub_keyboard
isr_stub_keyboard:
    push qword 0
    push qword 33
    jmp isr_common

; ---- common entry for everything else ----
isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, [rsp + 15*8]       ; vector number

    cmp rdi, 33
    je .kbd
    jmp .exception

.kbd:
    call keyboard_handler
    jmp .eoi

.exception:
    call exception_handler
    jmp .done

.eoi:
    mov al, 0x20
    out 0x20, al
    ; no slave EOI: we only handle master PIC IRQs (0-7) for now

.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq
