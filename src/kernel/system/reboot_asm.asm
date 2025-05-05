global triple_fault

triple_fault:
    lidt [empty_idt]
    int3

    ret

empty_idt:
    dw 0                ; IDT limit = 0
    dd 0                ; IDT base = 0
