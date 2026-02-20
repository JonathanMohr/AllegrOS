[bits 32]

;
; void CDECL enterKernel(void* bootParams, void* kernel);
;
global enterKernel
enterKernel:
    ; [esp + 4] = &bootParams
    jmp dword [esp + 8] ; &kernel

    ; shouldn't happen
    ; TODO: Reboot
    hlt
