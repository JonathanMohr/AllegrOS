[bits 32]

section .text

;
; void CDECL x86_invlpg(uintptr_t addr);
;
global x86_invlpg
x86_invlpg:
    mov eax, [esp + 4]
    invlpg [eax]
    ret

;
; void CDECL x86_reload_cr3(void);
;
global x86_reload_cr3
x86_reload_cr3:
    mov eax, cr3
    mov cr3, eax
    ret

;
; void CDECL x86_load_cr3(uint32_t pageDirectory);
;
global x86_load_cr3
x86_load_cr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret
