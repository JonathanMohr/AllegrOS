[bits 32]
CPU 386

section .text

extern __entry

global _start
_start:
    xor ebp, ebp

    and esp, -16

    call __entry

    mov ecx, eax 
    mov eax, 0
    int 0x80

    jmp $
