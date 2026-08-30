[bits 16]
[org 0x7c00]
start:
    mov si, msg
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

msg db 'Kizill_OS booted!', 0

times 510 - ($-$$) db 0
dw 0xaa55
