    b *0x7c00
    layout asm
    target remote | qemu-system-i386 -S -gdb stdio -m 32 -hda build/i686_debug/image.img
