import subprocess
from pathlib import Path

nasm = "nasm"

def compile_bootloader_stage1() -> Path:
    src = Path("src/boot.asm")
    out = Path("build/boot.bin")

    out.parent.mkdir(parents=True, exist_ok=True)

    subprocess.run([nasm, "-f", "bin", str(src), "-o", str(out)])

    return out

def run_qemu(image: Path):
    qemu_args = [
        "-m", "32",
#       "-spice", "port=5930,disable-ticketing",
        "-display", "sdl,gl=on",
        "-debugcon", "stdio"
    ]

    qemu = "qemu-system-i386"

    subprocess.run([
        qemu, *qemu_args,
        "-drive", f"format=raw,file={image},if=floppy"
    ])

if __name__ == "__main__":
    stage1 = compile_bootloader_stage1()

    run_qemu(stage1)
