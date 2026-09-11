[bits 32]
CPU 386

section .text

global _start
_start:
    int 0x80
    jmp _start
