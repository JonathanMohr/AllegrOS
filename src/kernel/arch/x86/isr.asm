[bits 32]
CPU 386

section .text

extern x86_ISR_Handler

global isr_common
isr_common:
    push dword [esp + 12] ; marker

    and [esp], 3
    jnz .privilege_ok

    sub esp, 8
    mov dword [esp], 0xFFFFFFFF

.privilege_ok:
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

    mov eax, [esp + 48]
    cmp eax, 0xFFFFFFFF
    jne .skip_fill

    lea eax, [esp + 80]
    mov [esp + 56], eax

    xor ebx, ebx
    mov bx, ss
    mov [esp + 52], ebx

.skip_fill:

    push esp ; pass pointer to stack to C
    call x86_ISR_Handler
    add esp, 4

    ; restore segment
    pop eax
    mov ds, ax

    pop eax
    mov es, ax

    pop eax
    mov fs, ax
    
    pop eax
    mov gs, ax

    popa

    cmp dword [esp], 0xFFFFFFFF
    jne .privilege_was_changed

    add esp, 8 ; remove 8 byte buffer (actually marker and 4 bytes of buffer)

.privilege_was_changed:
    add esp, 12 ; remove marker (or remaining 4 bytes of buffer when privilege was not changed), error code and interrupt number

    iret       ; pop cs, eip, eflags, ss, esp
