make clean
make -s
qemu-system-i386 -fda build/os.img -drive if=floppy,format=raw,file=build/os.img