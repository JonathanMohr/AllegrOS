[bits 16]
[org 0x7C00]

%define ENDL 0x0D, 0x0A

%macro CREATE_MBR_HEADER 1-*
    %rep %0
        MBR_HEADER%1:
            .boot           db 0
            .chs_start      db 0, 0, 0
            .type           db 0
            .chs_end        db 0, 0, 0
            .lba_start      dd 0
            .num_sectors    dd 0
        %rotate 1
    %endrep
%endmacro

; ENTRY

start:
    ; Deactivate interrupts for stack and memory setup
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ss, ax
    mov sp, 0x7C00

    xor di, di

    ; some BIOSes might start us at 07C0:0000 instead of 0000:7C00, make sure we are in the
    ; expected location
    push es
    push word .after
    retf

.after:

    ; DL should be drive number
    mov [drive_number], dl

    ; Activate interrupts
    sti

    ; int 13h ah=41h;bx=55AAh
    ; check extensions present

    ; CF set on error (extensions not supported)
    ; CF clear if successful

    ; BX = AA55h if extensions supported

    ; CX = Extensions API support:
    ;  Bit 0: extended disk access functions supported
    ;  Bit 1: removable drive controller functions
    ;  Bit 2: enhanced disk drive functions supported

    mov ah, 41h
    mov bx, 0x55AA
    stc
    int 13h

    jc .no_disk_extensions
    cmp bx, 0xAA55
    jne .no_disk_extensions
    test cx, 1
    jz .no_disk_extensions

    mov byte [extensions_supported], 1
    jmp .after_disk_extensions_check

.no_disk_extensions:
    ; read drive parameters

    ; int 13h ah=08h
    ; get drive parameters

    ; CF set on error (extensions not supported)
    ; CF clear if successful

    mov ah, 08h
    stc
    int 13h

    jc disk_read_error

    mov al, dh
    inc al
    mov [chs.heads], al

    and cl, 0b00111111
    mov [chs.sectors_per_track], cl

    mov byte [extensions_supported], 0

.after_disk_extensions_check:
    
    ; load stage 2

    mov si, MBR_HEADER1.lba_start

    mov ax, STAGE2_LOAD_SEGMENT
    mov es, ax

    mov bx, STAGE2_LOAD_OFFSET

    jmp disk_read_error

; TEXT

;
; Error handler
;

disk_read_error:
    mov si, msg_read_failed
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 16h
    ; jump to beginning of BIOS
    jmp 0FFFFh:0

.halt:
    cli
    hlt

;
; Prints a string to the screen
; Params:
;   - ds:si points to string
;
puts:
    ; save registers we will modify
    push si
    push ax
    push bx

.loop:
    lodsb               ; loads next character in al
    or al, al           ; verify if next character is null?
    jz .done

    mov ah, 0x0E        ; call bios interrupt
    mov bh, 0           ; set page number to 0
    int 0x10

    jmp .loop

.done:
    pop bx
    pop ax
    pop si

    ret

;
; Converts an LBA address to a CHS address
; Parameters:
;   - ax: LBA address
; Returns:
;   - cx [bits 0-5]: sector number
;   - cx [bits 6-15]: cylinder
;   - dh: head
;

lba_to_chs:

    push ax
    push dx

    xor dx, dx                          ; dx = 0
    div word [chs.sectors_per_track]    ; ax = LBA / SectorsPerTrack
                                        ; dx = LBA % SectorsPerTrack

    inc dx                              ; dx = (LBA % SectorsPerTrack + 1) = sector
    mov cx, dx                          ; cx = sector

    xor dx, dx                          ; dx = 0
    div word [chs.heads]                ; ax = (LBA / SectorsPerTrack) / Heads = cylinder
                                        ; dx = (LBA / SectorsPerTrack) % Heads = head
    mov dh, dl                          ; dh = head
    mov ch, al                          ; ch = cylinder (lower 8 bits)
    shl ah, 6
    or cl, ah                           ; put upper 2 bits of cylinder in CL

    pop ax
    mov dl, al                          ; restore DL
    pop ax
    ret

;
; Reads sectors from a disk
; Parameters:
;   - eax: LBA address
;   - cl: number of sectors to read (up to 128)
;   - dl: drive number
;   - es:bx: memory address where to store read data
;

disk_read:

    ; save registers
    push eax
    push bx
    push cx
    push dx
    push si
    push di

    cmp byte [extensions_supported], 1
    jne .no_disk_extensions

    ; retry count
    mov di, 3

.disk_extensions:
    
    mov [extensions_dap.lba], eax
    mov [extensions_dap.segment], es
    mov [extensions_dap.offset], bx
    mov [extensions_dap.count], cl

    mov ah, 42h
    mov si, extensions_dap
    jmp .retry

.no_disk_extensions:
    push cx
    call lba_to_chs
    pop ax

    mov ah, 02h

.retry:
    ; save registers
    pusha

    stc

    ; CF set on error (extensions not supported)
    ; CF clear if successful
    int 13h

    jnc .done

    ; failed
    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry

.fail:
    
    jmp disk_read_error

.done:
    popa

    ; restore registers
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop eax

    ret

;
; Resets disk controller
; Parameters:
;   dl: drive number
;
disk_reset:
    pusha

    mov ah, 0
    stc
    int 13h
    jc disk_read_error

    popa

    ret

; RODATA

msg_read_failed db "Read failed!", ENDL, 0

; DATA

STAGE2_LOAD_SEGMENT     equ 0000h
STAGE2_LOAD_OFFSET      equ 0500h

times 446-($-$$) db 0

CREATE_MBR_HEADER 1, 2, 3, 4

dw 0xAA55

; BSS

drive_number         db 0
extensions_supported db 0

extensions_dap:
    .size:              db 10h
                        db 0
    .count:             dw 0
    .offset:            dw 0
    .segment:           dw 0
    .lba:               dq 0

chs: ; default values
    .heads              dw 2
    .sectors_per_track  dw 18
