[BITS 16]
[ORG 0x7C00]

start:
    ; === Nachricht anzeigen ===
    mov ah, 0x0E
    mov si, message
.print:
    lodsb
    or al, al
    jz .load_kernel
    int 0x10
    jmp .print

.load_kernel:
    ; === Kernel von Festplatte laden (1 Sektor) ===
    mov ah, 0x02        ; BIOS: Sektor lesen
    mov al, 1           ; Anzahl Sektoren: 1
    mov ch, 0           ; Zylinder
    mov cl, 2           ; Sektor 2 (1-basiert, der 2. Sektor nach dem MBR)
    mov dh, 0           ; Head 0
    mov dl, 0x80        ; Erste Festplatte
    mov bx, 0x0000      ; Offset im Zielsegment
    mov ax, 0x1000      ; Zielsegment in AX
    mov es, ax          ; AX -> ES
    int 0x13

    jc disk_error
    jmp kernel_loaded

disk_error:
    mov si, disk_fail_msg
.print_fail:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0E
    int 0x10
    jmp .print_fail
.halt:
    cli
    hlt

kernel_loaded:
    jmp 0x1000:0000

; === Daten ===
message db "Bootloader gestartet!", 0
disk_fail_msg db "Fehler beim Laden des Kernels!", 0

times 510 - ($ - $$) db 0
dw 0xAA55
