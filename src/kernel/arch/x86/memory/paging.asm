[bits 32]
CPU 386

section .text

;
; void CDECL (*x86_invlpg)(uintptr_t addr)
;
invlp_start:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 0x200000
    push eax
    popfd

    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jnz .has_cpuid

.no_cpuid:
    mov dword [x86_invlpg], flush
    jmp flush

.has_cpuid:
    mov dword [x86_invlpg], invlp
    ; Fallthrough to invlp

;
; void CDECL (*x86_invlpg)(uintptr_t addr)
;
CPU 486
invlp:
    mov eax, [esp + 4]
    invlpg [eax]
    ret

;
; void CDECL (*x86_invlpg)(uintptr_t addr)
;
CPU 386
flush:
    mov eax, cr3
    mov cr3, eax
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

section .data

global x86_invlpg
x86_invlpg dd invlp_start
