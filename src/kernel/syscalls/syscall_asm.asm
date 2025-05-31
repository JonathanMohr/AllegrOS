[bits 32]

global set_eax
set_eax:
    mov eax, [esp + 4]
    ret

global set_edx
set_edx:
    mov edx, [esp + 4]
    ret