[bits 32]
cpu 386

;
; syscall_t syscall0(syscall_t number);
;
global syscall0
syscall0:
    mov eax, [esp + 4]
    int 0x80
    ret

;
; syscall_t syscall1(syscall_t number, syscall_t arg1);
;
global syscall1
syscall1:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    int 0x80
    ret

;
; syscall_t syscall2(syscall_t number, syscall_t arg1, syscall_t arg2);
;
global syscall2
syscall2:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

;
; syscall_t syscall3(syscall_t number, syscall_t arg1, syscall_t arg2, syscall_t arg3);
;
global syscall3
syscall3:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    mov ebx, [esp + 16]
    int 0x80
    ret
