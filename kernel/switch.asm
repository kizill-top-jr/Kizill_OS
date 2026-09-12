; switch.asm - context switch for 32-bit ring 0
; void switch_task(uint32_t *old_esp_ptr, uint32_t new_esp)
global switch_task

switch_task:
    pusha
    pushf

    mov eax, [esp + 40]   ; arg1: &old_esp
    mov edx, [esp + 44]   ; arg2: new_esp

    mov [eax], esp        ; *old_esp = current ESP
    mov esp, edx          ; ESP = new_esp

    popf
    popa
    ret
