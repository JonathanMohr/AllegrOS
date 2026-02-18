[bits 32]

%define ENDL 0x0D, 0x0A

section .text
    global start

start:
    mov esi, msg
    call debug_puts

    cli
    hlt

;
; Prints a string to the host terminal
; Params:
;   - esi points to string
;
debug_puts:
    ; save registers we will modify
    push si
    push ax
    push bx

.loop:
    lodsb               ; loads next character in al
    or al, al           ; verify if next character is null?
    jz .done

    out 0xE9, al

    jmp .loop

.done:
    pop bx
    pop ax
    pop si

    ret


section .rodata

msg db "Hello World!", ENDL, 0
