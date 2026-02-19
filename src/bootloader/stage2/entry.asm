extern __bss_start
extern __end

extern start
extern setup_paging
extern enter_protected
extern getE820MemoryBlocks

section .entry
    global entry

    global bootDrive

    global entry.pmode

entry:
    [bits 16]

    cli

    ; save boot drive
    mov [bootDrive], dl

    ; setup stack
    mov ax, ds
    mov ss, ax
    mov sp, 0xFFF0
    mov bp, sp

    ; switch to protected mode
    push dword .pmode
    jmp enter_protected

.pmode:
    [bits 32]

    ; clear bss
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb

    ; x86_E820MemoryBlocks pointer in eax
    ; count in edi
    call getE820MemoryBlocks

    push edi
    push eax

    ; page_directory pointer in eax
    call setup_paging

    push eax

    xor edx, edx
    mov dl, [bootDrive]
    push edx

    call start

    ; TODO: Reboot
    hlt

; DATA

bootDrive: db 0
