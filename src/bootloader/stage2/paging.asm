[bits 32]

%define ENDL 0x0D, 0x0A

extern __end

extern IO_PutStringCritical

section .text
    global setup_paging

setup_paging:
    mov ecx, __end

    cmp ecx, 1048576
    jae .setup

    mov ecx, 1048576
.setup:
    cmp ecx, 4194304
    ja error_stage2_too_big

    shr ecx, 12             ; page count (end in bytes / 4KB)
    xor ebx, ebx
    mov edi, page_table

.fill_identity:
    mov eax, ebx
    shl eax, 12             ; phys addr = page * 4KB
    or eax, 0x03            ; Present + RW
    mov [edi], eax
    add edi, 4
    inc ebx
    loop .fill_identity

    ; fill page directory
    mov eax, page_table
    or eax, 0x03
    mov [page_directory], eax

    ; set cr3 to page directory
    mov eax, page_directory
    mov cr3, eax

    ; activate paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    jmp .finished

.finished:
    mov eax, page_directory

    ret

error_stage2_too_big:
    push dword msg_stage2_too_big
    call IO_PutStringCritical
    
    ; TODO: Reboot
    hlt

section .rodata

msg_stage2_too_big db "Stage 2 is too big (__end is above 4194304 bytes)", ENDL, 0

section .bss

align 4096
page_directory resd 1024

align 4096
page_table resd 1024
