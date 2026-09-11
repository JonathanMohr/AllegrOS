[bits 32]
CPU 386

section .text

extern Task_Returned

;
; void CDECL Task_SwitchTo(uintptr_t* oldStackPointer, uintptr_t newStackPointer);
;
global Task_SwitchTo
Task_SwitchTo:
    pushfd
    push ebx
    push esi
    push edi
    push ebp

    mov ebx, [esp + 24]
    mov [ebx], esp
    mov esp, [esp + 28]

    pop ebp
    pop edi
    pop esi
    pop ebx
    popfd

    cld

    ret

;
; void CDECL Task_JumpTo(uintptr_t newStackPointer);
;
global Task_JumpTo
Task_JumpTo:
    mov esp, [esp + 4]

    pop ebp
    pop edi
    pop esi
    pop ebx
    popfd

    cld

    ret

;
; uintptr_t CDECL Task_Create(uintptr_t stackTop, void* function, void* arg);
;
global Task_Create
Task_Create:
    mov eax, [esp + 4]

    mov ecx, [esp + 12]
    mov [eax - 4], ecx

    mov dword [eax - 8], Task_Returned

    mov ecx, [esp + 8]
    mov [eax - 12], ecx

    mov dword [eax - 16], 0x202

    sub eax, 32

    ret
