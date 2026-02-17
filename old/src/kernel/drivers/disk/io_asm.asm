[bits 32]

global outb
outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global inb
inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global inw
inw:
    mov edx, [esp + 4]  ; Load port number into EDX
    xor eax, eax
    in ax, dx           ; Read 16 bits from port into AX
    ret