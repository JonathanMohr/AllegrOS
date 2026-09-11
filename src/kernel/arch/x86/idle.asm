[bits 32]
CPU 386

section .text

global Arch_Idle
Arch_Idle:
    hlt
    jmp Arch_Idle
