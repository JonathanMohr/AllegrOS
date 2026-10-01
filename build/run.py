from pathlib import Path
from build.defs import OS, Baseline

import subprocess
import platform
import sys

def run(image: Path, hostOS: OS, baseline: Baseline, debug: bool, debugMode: bool):
    if hostOS == OS.macOS:
        displayBackend = "cocoa,zoom-to-fit=on,zoom-interpolation=on"
    else:
        displayBackend = "sdl,gl=on"

    match baseline:
        case Baseline.i386: qemu_cpu = "486"
        case Baseline.i486: qemu_cpu = "486"
        case Baseline.i586: qemu_cpu = "pentium"
        case Baseline.i686: qemu_cpu = "pentium2"
        case _:
            raise ValueError("Invalid baseline")

    try:
        qemu_args = [
            "-m", "32",
            "-cpu", qemu_cpu,
    #       "-spice", "port=5930,disable-ticketing",
            "-display", f"{displayBackend}",
            "-debugcon", "stdio"
        ]

        if debug:
            qemu_args.extend([
                "-no-reboot", "-no-shutdown",
                "-d", "int,cpu_reset",
                "-D", "logs/qemu.log"
            ])

        if debugMode:
            qemu_args.extend(["-S", "-s"])

        qemu = "qemu-system-i386"

        subprocess.run([
            qemu, *qemu_args,
            "-drive", f"format=raw,file={image},if=ide"
        ], check=True)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Running QEMU with {image} failed: {e}")

def debugger(stage1: Path, stage2: Path, kernel: Path):
    try:
        lldb = "lldb"

        lldb_args = [
            str(kernel),
            "-o", "gdb-remote localhost:1234",
            "-o", f"target modules add {stage1}",
            "-o", f"target modules add {stage2}"
        ]

        subprocess.run([
            lldb, *lldb_args
        ], check=True)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Running debugger failed: {e}")


if __name__ == "__main__":
    hostOS: OS
    os_uname = platform.system().lower()
    if (os_uname == "windows"): hostOS = OS.Windows
    elif (os_uname == "darwin"): hostOS = OS.macOS
    elif (os_uname == "linux"): hostOS = OS.Linux
    else:
        print(f"Unknown uname: {os_uname}")
        sys.exit(1)

    try:
        run(Path(".dist/image.img"), hostOS, Baseline.i686, False, False)

    except Exception as e:
        print(f"Error: {e}")
