global shutdown
shutdown:
    mov dx, 0x604
    mov ax, 0x2000
    out dx, ax

    mov dx, 0xB004
    mov ax, 0x3400
    out dx, ax

    ret