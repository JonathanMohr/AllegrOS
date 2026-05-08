[bits 32]

section .text

;
; int CDECL memcmp(const void* s1, const void* s2, size_t n);
;
global memcmp
memcmp:
    push edi
    push esi

    mov edi, [esp + 12] ; s1
    mov esi, [esp + 16] ; s2
    mov ecx, [esp + 20] ; count
    
    xor eax, eax    ; return value
    test ecx, ecx   ; return when n == 0
    jz .done

.loop:
    mov al, [edi]
    mov dl, [esi]
    cmp al, dl
    jne .diff

    inc edi
    inc esi
    dec ecx
    jnz .loop

    xor eax, eax
    jmp .done

.diff:
    movzx eax, al
    movzx edx, dl
    sub eax, edx
    
.done:
    pop esi
    pop edi

    ret

;
; void* CDECL memcpy(void* dst, const void* src, size_t n);
;
global memcpy
memcpy:
    push edi
    push esi

    mov edi, [esp + 12] ; dst
    mov esi, [esp + 16] ; src
    mov ecx, [esp + 20] ; count
    mov eax, edi        ; return value

    cld
    rep movsb

    pop esi
    pop edi

    ret

;
; void* CDECL memset(void* dst, int value, size_t n);
;
global memset
memset:
    push edi

    mov edi, [esp + 8]  ; dst
    mov eax, [esp + 12] ; value
    mov ecx, [esp + 16] ; count
    mov edx, edi        ; return value

    cld
    rep stosb

    mov eax, edx
    pop edi
    ret

;
; void* CDECL memmove(void* dst, const void* src, size_t n);
;
global memmove
memmove: ; TODO: Check
    push edi
    push esi

    mov edi, [esp + 12] ; dst
    mov esi, [esp + 16] ; src
    mov ecx, [esp + 20] ; count
    mov eax, edi        ; return value

    test ecx, ecx
    jz .done

    cmp edi, esi
    jb .forward

    mov edx, esi
    add edx, ecx          ; edx = src + n

    cmp edi, edx
    jae .forward          ; no overlap -> forward copy

    ; backward copy
    add esi, ecx
    add edi, ecx
    std
    rep movsb
    cld

    jmp .done

.forward:
    cld
    rep movsb

.done:
    pop esi
    pop edi

    ret
