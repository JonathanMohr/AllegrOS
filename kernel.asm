[BITS 16]
[ORG 0x1000]      ; Setzt den Startpunkt des Codes auf 0x1000, der von deinem Bootloader geladen wird.

start:
    ; --- Einfacher Video-Mode ändern (Textmodus) ---
    mov ah, 0x0E
    mov al, 'K'        ; Ein Buchstabe anzeigen
    int 0x10           ; BIOS-Interrupt für Video

    ; --- Nachricht ausgeben ---
    mov ah, 0x0E
    mov al, 'e'        ; Nächster Buchstabe
    int 0x10
    mov al, 'r'
    int 0x10
    mov al, 'n'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'l'
    int 0x10

    ; --- Endlosschleife (bis der Benutzer den Rechner ausschaltet) ---
.loop:
    jmp .loop

