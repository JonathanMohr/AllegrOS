[bits 32]

; void ASMCALL set_Stack(uint32_t ptr);
global set_Stack
set_Stack:
    mov eax, [esp + 4]      ; neues Stack-Ende (Kernel Stack oben)
    mov ebx, [esp]          ; Rücksprungadresse vom alten Stack holen
    mov [eax - 4], ebx      ; Rücksprungadresse ganz oben auf neuen Stack legen
    lea esp, [eax - 4]      ; esp auf die Rücksprungadresse setzen
    mov ebp, eax            ; ebp auf neuen Stack (optional)
    ret