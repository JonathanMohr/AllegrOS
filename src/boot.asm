[bits 16]
[org 0x7C00]

%define ENDL 0x0D, 0x0A

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov si, msg
    call print_string

print_string:
    mov ah, 0x0E
.next_char:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .next_char
.done:
    ret

msg db "Hello World!", ENDL, 0

times 510-($-$$) db 0
dw 0xAA55
