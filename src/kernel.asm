[bits 16]
[org 0x1000]

start:
    ; Ausgabe einer Nachricht, um zu zeigen, dass der Kernel läuft
    mov ah, 0x0E
    mov al, 'K'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'r'
    int 0x10
    mov al, 'n'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'l'
    int 0x10

    ; Endlosschleife im Kernel
    jmp $

