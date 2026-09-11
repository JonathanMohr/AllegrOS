[bits 32]
CPU 386

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

global x86_outw
x86_outw:
    mov dx, [esp + 4]
    mov ax, [esp + 8]
    out dx, ax
    ret

global x86_inw
x86_inw:
    mov dx, [esp + 4]
    xor eax, eax
    in ax, dx
    ret

global x86_outl
x86_outl:
    mov dx, [esp + 4]
    mov eax, [esp + 8]
    out dx, eax
    ret

global x86_inl
x86_inl:
    mov dx, [esp + 4]
    in eax, dx
    ret
