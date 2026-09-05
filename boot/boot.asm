[bits 16]
[org 0x7c00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Сброс диска
    mov ah, 0x00
    int 0x13
    jc error

    ; Чтение через LBA (расширенное чтение)
    ; Структура DAP (Disk Address Packet) в памяти
    mov si, dap
    mov ah, 0x42
    mov dl, 0x80
    int 0x13
    jc error

    ; Копируем ядро из 0x10000 в 0x100000
    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, 4096      ; 16 КБ (хватит для 5008 байт ядра)
    cld
    rep movsd

    ; Переключение в защищённый режим
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

error:
    mov si, msg_error
    call print
    cli
    hlt

print:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp print
.done:
    ret

msg_error db 'Error loading kernel!', 0

; DAP – Disk Address Packet
align 4
dap:
    db 0x10             ; размер структуры (16 байт)
    db 0x00             ; зарезервировано
    dw 32               ; количество секторов для чтения (достаточно для 16 КБ)
    dw 0x0000           ; смещение буфера (0x1000:0x0000)
    dw 0x1000           ; сегмент буфера
    dq 1                ; LBA-адрес первого сектора (сектор 1 — сразу после загрузчика)

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

[bits 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    call 0x100000
    cli
    hlt

times 510 - ($-$$) db 0
dw 0xaa55
