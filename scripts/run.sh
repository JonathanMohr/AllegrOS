#!/bin/bash

QEMU_ARGS='-m 32'

if [ "$#" -le 1 ]; then
    echo "Usage: ./run.sh <image_type> <image>"
    exit 1
fi

case "$1" in
    "floppy")   QEMU_ARGS="${QEMU_ARGS} -fda $(wslpath -w $2)"
    ;;
    "disk")     QEMU_ARGS="${QEMU_ARGS} -hda $(wslpath -w $2)"
    ;;
    *)          echo "Unknown image type $1."
                exit 2
esac

mkdir -p /mnt/c/Users/Jonathan/images
cp build/i686_debug/image.img /mnt/c/Users/Jonathan/images/image.img
"/mnt/c/Windows/System32/cmd.exe" /C "qemu-system-i386.exe" $QEMU_ARGS