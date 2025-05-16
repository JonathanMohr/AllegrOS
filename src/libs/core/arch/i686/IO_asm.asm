[bits 32]

global Outb
Outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global Inb
Inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret