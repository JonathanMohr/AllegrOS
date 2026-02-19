[bits 16]

%define ENDL 0x0D, 0x0A
%define MAX_REGIONS 256

; Convert linear address to segment:offset address
; Args:
;    1 - linear address
;    2 - (out) target segment (e.g. es)
;    3 - target 32-bit register to use (e.g. eax)
;    4 - target lower 16-bit half of #3 (e.g. ax)

%macro LinearToSegOffset 4

    mov %3, %1      ; linear address to eax
    shr %3, 4
    mov %2, %4
    mov %3, %1      ; linear address to eax
    and %3, 0xf

%endmacro

section .entry
    global getE820MemoryBlocks

getE820MemoryBlocks:
    push bp
    mov bp, sp
    sub sp, 8
    
    mov dword [bp - 8], 0   ; count = 0
    mov dword [bp - 4], 0   ; continuation = 0

    mov esi, x86_E820MemoryBlocks

    lea edx, [ebp - 4]
    call x86_E820GetNextBlock
.loop:
    cmp eax, 0
    jle .done

    add esi, 24
    inc dword [bp - 8]

    cmp dword [bp - 8], MAX_REGIONS
    jae error_max_regions

    lea edx, [ebp - 4]
    call x86_E820GetNextBlock

    mov eax, [bp - 4]
    cmp eax, 0
    jne .loop

.done:
    mov eax, x86_E820MemoryBlocks
    mov edi, [bp - 8]

    add sp, 8
    pop bp
    ret


error_max_regions:
    cli
    mov si, msg_max_regions

.loop:
    lodsb               ; loads next character in al
    or al, al           ; verify if next character is null?
    jz .done

    mov ah, 0x0E        ; call bios interrupt
    mov bh, 0           ; set page number to 0
    int 0x10

    jmp .loop
.done:

    ; TODO: Reboot
    hlt

E820Signature   equ 0x534D4150

x86_E820GetNextBlock:
    ; make new call frame
    push bp             ; save old call frame
    mov bp, sp          ; initialize new call frame

    ; save modified regs
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ds
    push es

    ; setup params
    LinearToSegOffset esi, es, edi, di     ; es:di pointer to structure
    
    LinearToSegOffset edx, ds, esi, si    ; ebx - pointer to continuationId
    mov ebx, ds:[si]

    mov eax, 0xE820                             ; eax - function
    mov edx, E820Signature                      ; edx - signature
    mov ecx, 24                                 ; ecx - size of structure

    ; call interrupt
    int 0x15

    ; test results
    cmp eax, E820Signature
    jne .Error

.IfSuccedeed:
    mov eax, ecx            ; return size
    mov ds:[si], ebx        ; fill continuation parameter
    jmp .EndIf

.Error:
    mov eax, -1

.EndIf:

    ; restore regs
    pop es
    pop ds
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx

    ; restore old call frame
    mov sp, bp
    pop bp

    ret

section .rodata

msg_max_regions db "Error: MAX_REGIONS reached!", ENDL, 0

section .data
    global x86_E820MemoryBlocks

; MAX_REGIONS
x86_E820MemoryBlocks:
%rep MAX_REGIONS
dq 0, 0 ; base, length
dd 0, 0 ; type, acpi
%endrep
