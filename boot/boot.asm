[bits 16]
[org 0x7c00]

start:
    cli
    lgdt [gdt_descriptor]
    ; Включаем защищённый режим
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode   ; прыжок на 32-битный код с селектором 0x08

[bits 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Очищаем экран
    mov edi, 0xB8000
    mov ecx, 80*25
    mov al, ' '
    mov ah, 0x07
    rep stosw

    ; Выводим строку
    mov esi, msg
    mov edi, 0xB8000
    call print

    cli
    hlt

print:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x1F   ; синий фон, белый текст (можно поменять на 0x07)
    stosw
    jmp .loop
.done:
    popa
    ret

msg db 'Kizill_OS: Protected Mode!', 0

; GDT
gdt_start:
    dd 0x00000000
    dd 0x00000000
    ; селектор кода 0x08
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0b10011010
    db 0b11001111
    db 0x00
    ; селектор данных 0x10
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

times 510 - ($-$$) db 0
dw 0xaa55
