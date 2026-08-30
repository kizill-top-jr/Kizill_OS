[bits 32]

kernel_start:
    ; Очищаем экран (заполняем всю видеопамять пробелами)
    mov edi, 0xB8000
    mov ecx, 80*25
    mov al, ' '
    mov ah, 0x07          ; белый на чёрном
    rep stosw

    ; Выводим строку
    mov esi, msg
    mov edi, 0xB8000      ; начало VGA
    call print_string

    ; Бесконечный цикл
    cli
    hlt

print_string:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    ; символ в AL, атрибут (цвет) = 0x1F (синий фон, белый текст)
    mov ah, 0x1F
    stosw
    jmp .loop
.done:
    popa
    ret

msg db 'Kizill_OS: Protected Mode!', 0
