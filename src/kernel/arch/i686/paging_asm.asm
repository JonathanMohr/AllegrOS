[bits 32]

; void ASMCALL write_cr3(uint32_t val);
global write_cr3
write_cr3:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov eax, [ebp + 8]
    mov cr3, eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

; uint32_t ASMCALL read_cr0();
global read_cr0
read_cr0:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov eax, cr0

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

; void ASMCALL write_cr0(uint32_t val);
global write_cr0
write_cr0:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov eax, [ebp + 8]
    mov cr0, eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

; void ASMCALL i686_invlpg(uint32_t addr);
global i686_invlpg
i686_invlpg:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov eax, [ebp + 8]
    invlpg [eax]

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret