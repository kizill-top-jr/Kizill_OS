; multiboot2 header for 64-bit kernel
section .multiboot
align 8
    dd 0xE85250D6                 ; magic
    dd 0                          ; architecture (0 = i386, but we'll use long mode later)
    dd header_end - header_start  ; header length
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) ; checksum

header_start:
    ; Optional: add tags if needed (we keep minimal)
    ; end tag
    dw 0
    dw 0
    dd 8
header_end:
