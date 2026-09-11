CPU 386

extern __bss_start
extern __end

extern start
extern setup_paging
extern enter_protected
extern getE820MemoryBlocks

section .entry
    global entry

entry:
    [bits 16]

    ; save boot drive
    mov [bootDrive], dl

    ; setup stack
    mov ax, ds
    mov ss, ax
    mov sp, 0x7BFC
    mov bp, sp

    sti

    ; x86_E820MemoryBlocks pointer in eax
    ; count in edi
    call getE820MemoryBlocks

    mov dword [memoryBlockPointer], eax
    mov dword [memoryBlockCount], edi

    cli

    ; switch to protected mode
    push dword .pmode
    jmp enter_protected

.pmode:
    [bits 32]

    ; copy bootsector
    mov esi, 0x7C00
    mov edi, mbr_bootsector
    mov ecx, 128            ; 512 bytes = 128 dwords
    cld
    rep movsd

    ; clear bss
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb

    push dword [memoryBlockCount]
    push dword [memoryBlockPointer]

    ; page_directory pointer in eax
    call setup_paging

    push eax

    xor edx, edx
    mov dl, [bootDrive]
    push edx

    call start

    ; TODO: Reboot
    hlt

section .data

global mbr_bootsector

memoryBlockPointer dd 0
memoryBlockCount   dd 0

bootDrive db 0

mbr_bootsector times 512 db 0
