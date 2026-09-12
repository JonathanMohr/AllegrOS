[bits 32]
CPU 386

;
; void CDECL Arch_JumpToUserMode(uintptr_t entryPoint, uintptr_t stackTop);
;
global Arch_JumpToUserMode
Arch_JumpToUserMode:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]

    mov dx, 0x20 | 3
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx

    push 0x20 | 3
    push ecx
    pushfd
    pop edx
    or edx, 0x200
    push edx
    push 0x18 | 3
    push eax

    iret
