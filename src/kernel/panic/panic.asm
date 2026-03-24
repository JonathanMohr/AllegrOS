[bits 32]

; void CDECL Panic();
global Panic
Panic:
    cli
    hlt
