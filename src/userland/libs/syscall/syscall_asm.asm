[bits 32]

; void ASMCALL syscall(uint32_t eax,
;                      uint32_t ebx,
;                      uint32_t ecx,
;                      uint32_t edx,
;                      uint32_t esi,
;                      uint32_t edi,
;                      uint32_t ebp);
global syscall
syscall:
    ; save changed regs
    push ebx
    push esi
    push edi
    push ebp

    mov eax, [esp + 20]
    mov ebx, [esp + 24]
    mov ecx, [esp + 28]
    mov edx, [esp + 32]
    mov esi, [esp + 36]
    mov edi, [esp + 40]
    mov ebp, [esp + 44]

    int 0x80

    ; restore changed regs
    pop ebp
    pop edi
    pop esi
    pop ebx

    ret