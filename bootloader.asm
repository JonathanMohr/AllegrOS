[BITS 16]       ; CPU startet im 16-Bit-Real-Mode
[ORG 0x7C00]    ; BIOS lädt den Bootloader immer an Adresse 0x7C00

start:
    ; Lösche das Bildschirmlayout
    mov ah, 0x0E   ; BIOS-Funktion für Textausgabe
    mov si, message
print:
    lodsb           ; Lade nächstes Zeichen aus message in AL
    or al, al       ; Ist es das Null-Zeichen (Ende der Nachricht)?
    jz halt         ; Wenn ja, springe zu halt
    int 0x10        ; BIOS-Aufruf für Textausgabe
    jmp print       ; Wiederhole

halt:
    cli             ; Interrupts deaktivieren (Sicherheit)
    hlt             ; CPU anhalten

message db "Bootloader gestartet!", 0

times 510 - ($ - $$) db 0  ; Fülle die restlichen Bytes mit Nullen
dw 0xAA55                  ; Boot-Signatur, damit BIOS es erkennt
