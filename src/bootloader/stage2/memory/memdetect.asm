[bits 32]

%define ENDL 0x0D, 0x0A
%define MAX_REGIONS 256

%macro x86_EnterRealMode 1
    [bits 32]
    jmp word 18h:.pmode16%1       ; 1 - jump to 16-bit protected mode segment

.pmode16%1:
    [bits 16]
    ; 2 - disable protected mode bit in cr0
    mov eax, cr0
    and al, ~1
    mov cr0, eax

    ; 3 - jump to real mode
    jmp word 00h:.rmode%1

.rmode%1:
    ; 4 - setup segments
    mov ax, 0
    mov ds, ax
    mov ss, ax

    ; 5 - enable interrupts
    sti

%endmacro


%macro x86_EnterProtectedMode 1
    cli

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode%1


.pmode%1:
    ; we are now in protected mode!
    [bits 32]
    
    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

%endmacro

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

extern IO_PutStringCritical

section .entry
    global getE820MemoryBlocks

getE820MemoryBlocks:
    push ebp
    mov ebp, esp
    sub esp, 8
    
    mov dword [ebp - 8], 0   ; count = 0
    mov dword [ebp - 4], 0   ; continuation = 0

    mov esi, x86_E820MemoryBlocks

    lea edx, [ebp - 4]
    push edx
    push esi

    ;push eax
    ;x86_EnterRealMode a
    ;pop eax

    call test

    ;push eax
    ;x86_EnterProtectedMode a
    ;pop eax

    add esp, 8
.loop:
    cmp eax, 0
    jle .done

    add esi, 24
    inc dword [ebp - 8]

    cmp dword [ebp - 8], MAX_REGIONS
    jae error_max_regions

    lea edx, [ebp - 4]
    push edx
    push esi

    ;push eax
    ;x86_EnterRealMode b
    ;pop eax

    call test

    ;push eax
    ;x86_EnterProtectedMode b
    ;pop eax

    add esp, 8

    mov eax, [ebp - 4]
    cmp eax, 0
    jne .loop

.done:
    mov eax, x86_E820MemoryBlocks
    mov edi, [ebp - 8]

    add esp, 8
    pop ebp
    ret


error_max_regions:
    push dword msg_max_regions
    call IO_PutStringCritical

    ; TODO: Reboot
    hlt

test:
    x86_EnterRealMode a
    jmp x86_E820GetNextBlock

E820Signature   equ 0x534D4150

x86_E820GetNextBlock:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    ; save modified regs
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ds
    push es

    ; setup params
    LinearToSegOffset [bp + 8], es, edi, di     ; es:di pointer to structure
    
    LinearToSegOffset [bp + 12], ds, esi, si    ; ebx - pointer to continuationId
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
    mov esp, ebp
    pop ebp

    push eax
    x86_EnterProtectedMode a
    pop eax

    ret

section .rodata

msg_max_regions db "Error: MAX_REGIONS reached!", ENDL, 0

section .bss
    global x86_E820MemoryBlocks

; MAX_REGIONS
x86_E820MemoryBlocks:
%rep MAX_REGIONS
resq 2 ; base, length
resd 2 ; type, acpi
%endrep
