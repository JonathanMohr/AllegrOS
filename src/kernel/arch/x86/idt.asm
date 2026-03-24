[bits 32]

; void CDECL x86_IDT_Load(IDTDescriptor* descriptor);
global x86_IDT_Load
x86_IDT_Load:
    ; load idt
    mov eax, [esp + 4]
    lidt [eax]

    ret
