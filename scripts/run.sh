#!/bin/bash

QEMU_ARGS='-m 32'

if [ "$#" -le 1 ]; then
    echo "Usage: ./run.sh <image_type> <image>"
    exit 1
fi

IMAGE_PATH=$2

if [ ! -f "$IMAGE_PATH" ]; then
    echo "Image file does not exist: $IMAGE_PATH"
    exit 3
fi

IMAGE_DIR=$(dirname "$IMAGE_PATH")
IMAGE_NAME=$(basename "$IMAGE_PATH")

DEST_DIR="/mnt/c/wsl/os_images/$IMAGE_DIR"
mkdir -p "$DEST_DIR"

cp "$IMAGE_PATH" "$DEST_DIR/$IMAGE_NAME"
cp -ru "$IMAGE_DIR/images" "$DEST_DIR"

WINDOWS_PATH=$(wslpath -w "$DEST_DIR/$IMAGE_NAME")

FORMAT=$3

case "$1" in
    "floppy")   QEMU_ARGS="${QEMU_ARGS} -drive file=$WINDOWS_PATH,format=${FORMAT},if=floppy"
    ;;
    "disk")     QEMU_ARGS="${QEMU_ARGS} -drive file=$WINDOWS_PATH,format=${FORMAT},if=ide"
    ;;
    *)          echo "Unknown image type $1."
                exit 2
esac

(
  cd /mnt/c && \
  /mnt/c/Windows/System32/cmd.exe /C "qemu-system-i386.exe $QEMU_ARGS"
)