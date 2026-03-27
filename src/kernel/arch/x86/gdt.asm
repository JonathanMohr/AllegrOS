[bits 32]

section .text

;
; void CDECL x86_GDT_Load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);
;
global x86_GDT_Load
x86_GDT_Load:
    ; load gdt
    mov eax, [esp + 4]
    lgdt [eax]

    ; reload code segment
    mov eax, [esp + 8]
    push eax
    push .reload_cs
    retf

.reload_cs:

    ; reload data segments
    mov ax, [esp + 12]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret
