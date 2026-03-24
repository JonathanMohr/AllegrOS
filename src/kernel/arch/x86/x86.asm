[bits 32]

section .text

global x86_outb
x86_outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global x86_inb
x86_inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret
