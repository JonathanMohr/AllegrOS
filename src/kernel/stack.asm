[bits 32]

extern kmain

section .text
    global kentry

kentry:
    mov eax, [esp + 4] ; &bootParams

    mov esp, stack_top

    push eax
    call kmain

    cli
    hlt

section .bss
stack_bottom:
    resb 1024 * 1024 ; 1 MB
stack_top:
