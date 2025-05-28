[bits 32]

; void ASMCALL enter(void* entryPoint, void* userStack);
global enter
enter:
    push ebp
    mov ebp, esp
    
    cli

    ; User-Datensegment in DS, ES, FS, GS setzen (0x23 = User Data Segment + RPL3)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, [ebp+8]    ; entryPoint
    mov ebx, [ebp+12]   ; userStack

    ; iret Stackframe aufbauen (SS:ESP, EFLAGS, CS:EIP)
    push 0x23             ; SS (User Data Segment)
    push ebx              ; ESP (User Stack)

    pushfd                ; EFLAGS pushen
    pop ecx
    or ecx, 0x200         ; Interrupt Flag (IF) setzen
    push ecx

    push 0x1B             ; CS (User Code Segment + RPL3)
    push eax              ; EIP (Entry Point)

    iret