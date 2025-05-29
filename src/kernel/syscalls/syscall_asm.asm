[bits 32]

global set_eax
set_eax:
    mov eax, [esp + 4]
    ret