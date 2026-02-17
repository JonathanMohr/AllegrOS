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

times 446-($-$$) db 0

%macro CREATE_MBR_HEADER 1-*
    %rep %0
        MBR_HEADER%1:
            .boot            db 0
            .chs_start       db 0, 0, 0
            .type            db 0
            .chs_end         db 0, 0, 0
            .lba_start       dd 0
            .size_in_sectors dd 0

        %rotate 1
    %endrep
%endmacro

CREATE_MBR_HEADER 1, 2, 3, 4

dw 0xAA55
