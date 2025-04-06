[bits 16]                ; Real Mode
[org 0x7C00]             ; Setze den Bootsektor-Start bei 0x7C00

jmp short start
    nop
    db "MSWIN4.1"          ; OEM Name

    dw 512                 ; Bytes per sector
    db 1                   ; Sectors per cluster
    dw 1                   ; Reserved sectors
    db 2                   ; FAT copies
    dw 224                 ; Root entries
    dw 2880                ; Total sectors (for 1.44 MB floppy)
    db 0xF0                ; Media descriptor
    dw 9                   ; Sectors per FAT
    dw 18                  ; Sectors per track
    dw 2                   ; Number of heads
    dd 0                   ; Hidden sectors
    dd 0                   ; Total sectors (if above 65535)

; Start des Bootloaders
start:
    ; BIOS Init und Segment-Setups
    cli
    xor ax, ax           ; Setze AX auf 0
    mov ds, ax           ; Setze DS auf 0 (Daten-Segment)
    mov es, ax           ; Setze ES auf 0 (Extra-Segment)

    ; Ausgabe einer einfachen Nachricht (Debugging)
    mov si, bootloader_msg   ; Setze SI auf die Adresse der Nachricht
    call print_string        ; Nachricht drucken

    call load_kernel

    ; Springe zum Kernel (der Kernel ist direkt nach der MBR-Signatur)
    jmp 0x1000

; Funktion zum Drucken einer Zeichenkette
print_string:
    mov ah, 0x0E        ; BIOS-Teletype-Ausgabe (Character Output)
.next_char:
    lodsb               ; Lade das nächste Byte aus DS:SI (ASCII-Zeichen)
    or al, al           ; Prüfe, ob Nullbyte (Ende der Zeichenkette)
    jz .done            ; Wenn Nullbyte, beende die Schleife
    int 0x10            ; BIOS-Interrupt: Ausgabe des Zeichens
    jmp .next_char      ; Nächster Buchstabe
.done:
    ret                 ; Rückkehr zur aufrufenden Funktion

load_kernel:
    ; BIOS-Interrupt 0x13 zum Lesen des Sektors vom Laufwerk
    mov ah, 0x02        ; BIOS: Sektor lesen
    mov al, 1           ; Anzahl der Sektoren: 1
    mov ch, 0           ; Zylinder: 0
    mov cl, 2           ; Sektor: 2 (der zweite Sektor nach der MBR)
    mov dh, 0           ; Head: 0
    mov dl, 0x80        ; Erste Festplatte (0x80 für die erste Festplatte)
    
    mov bx, 0x1000      ; Zieladresse im RAM (0x1000)
    mov es, bx          ; ES = Zielsegment (0x1000)
    xor bx, bx          ; BX = 0 (Offset im Zielsegment)
    int 0x13            ; BIOS Interrupt zum Lesen des Sektors

    jc disk_error       ; Fehlerbehandlung, falls etwas schiefgeht
    ret

disk_error:
    ; Ausgabe einer Fehlermeldung
    mov si, disk_error_msg
    call print_string
    
    ; Endlosschleife, um das System anzuhalten
    jmp $

disk_error_msg db "Fehler beim Laden des Kernels!", 0

bootloader_msg db "Bootloader läuft!", 0  ; String, der ausgegeben wird

times 446 - ($ - $$) db 0

db 0x80
db 0x01, 0x01, 0x00
db 0x0B
db 0xFE, 0xFF, 0xFF
dd 0x00000001
dd 0x000000FF

times 16 * 3 db 0

dw 0xAA55                     ; Boot-Signatur