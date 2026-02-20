[bits 32]

section .text

global kmain
kmain:
    mov al, 'H'
    out 0xE9, al

    mov al, 'i'
    out 0xE9, al

    mov al, '!'
    out 0xE9, al

    mov al, 10
    out 0xE9, al

    jmp $
