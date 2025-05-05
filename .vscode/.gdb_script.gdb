    symbol-file /home/jonathan/os/build/i686_debug/kernel/kernel.elf
    set disassembly-flavor intel
    target remote | qemu-system-i386 -S -gdb stdio -m 32 -hda /home/jonathan/os/build/i686_debug/image.img
