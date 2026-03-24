[bits 32]

extern x86_ISR_Handler

global isr_common
isr_common:
    pusha   ; pushes edi, esi, ebp, esp, ebx, edx, ecx, eax

    ; push gs
    xor eax, eax
    mov ax, gs
    push eax

    ; push fs
    xor eax, eax
    mov ax, fs
    push eax

    ; push es
    xor eax, eax
    mov ax, es
    push eax

    ; push ds
    xor eax, eax
    mov ax, ds
    push eax

    ; use kernel data segment
    mov ax, 0x10 ; use kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp ; pass pointer to stack to C
    call x86_ISR_Handler
    add esp, 4
    mov ebx, eax

    ; restore segment
    pop eax
    mov ds, ax

    pop eax
    mov es, ax

    pop eax
    mov fs, ax
    
    pop eax
    mov gs, ax

    ; save eax
    mov [esp + 28], ebx
    popa

    add esp, 8 ; remove error code and interrupt number
    iret       ; pop cs, eip, eflags, ss, esp
