[bits 32]

section .text

global Arch_Idle
Arch_Idle:
    hlt
    jmp Arch_Idle
