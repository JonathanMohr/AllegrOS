[bits 32]

;
; void CDECL enterKernel(void* kernel, void* bootParams);
;

global enterKernel
enterKernel:
    mov eax, [esp + 4] ; &kernel
    mov ebx, [esp + 8] ; &bootParams

    push ebx

    jmp eax

    ; shouldn't happen
    ; TODO: Reboot
    hlt
