[bits 32]
CPU 386

; void CDECL Panic();
global Panic
Panic:
    cli
    hlt
