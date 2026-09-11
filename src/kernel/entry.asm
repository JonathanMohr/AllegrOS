[bits 32]

extern kmain
extern Arch_Idle

section .text
    global kentry

kentry:
    mov eax, [esp + 4] ; &bootParams

    mov esp, stack_top

    push eax
    call kmain

    jmp Arch_Idle

section .bss
global stack_top

align 16
stack_bottom:
    resb 1024 * 16 ; 16 KiB
stack_top:
