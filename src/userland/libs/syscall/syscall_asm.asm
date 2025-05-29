[bits 32]

; void ASMCALL syscall(uint32_t eax,
;                      uint32_t ebx,
;                      uint32_t ecx,
;                      uint32_t edx,
;                      uint32_t esi,
;                      uint32_t edi);
global syscall
syscall:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    ; save changed regs
    push ebx
    push esi
    push edi

    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, [ebp + 20]
    mov esi, [ebp + 24]
    mov edi, [ebp + 28]

    int 0x80

    ; restore changed regs
    pop edi
    pop esi
    pop ebx

    ; restore old call frame
    mov esp, ebp
    pop ebp

    ret