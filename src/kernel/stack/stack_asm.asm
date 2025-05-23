[bits 32]

extern kernel_main

; void ASMCALL set_Stack(uint32_t ptr);
global set_Stack
set_Stack:
    mov eax, [esp + 4]    ; neues Stackende (kernel_stack + size)
    mov esp, eax          ; ESP auf neuen Stack setzen
    jmp kernel_main       ; direkt zu kernel_main springen