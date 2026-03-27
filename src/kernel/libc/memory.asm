[bits 32]

section .text

;
; void* CDECL memcpy(void* dst, const void* src, uint32_t count);
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
; void* CDECL memset(void* dst, int32_t value, uint32_t count);
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

    pop edi
    ret
