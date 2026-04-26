from scripts.defs import OS, ARCH
import scripts.build as build
import scripts.logger as cLogger

import rich

from pathlib import Path
from logging.handlers import RotatingFileHandler
import sys
import platform
import logging
import time
import subprocess

def run_qemu(image: Path, hostOS: OS):
    if hostOS == OS.macOS:
        displayBackend = "cocoa"
    else:
        displayBackend = "sdl,gl=on"

    try:
        qemu_args = [
            "-m", "32",
    #       "-spice", "port=5930,disable-ticketing",
            "-display", f"{displayBackend}",
            "-debugcon", "stdio"
        ]

        qemu = "qemu-system-i386"

        subprocess.run([
            qemu, *qemu_args,
            "-drive", f"format=raw,file={image},if=ide"
        ], check=True)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Running QEMU with {image} failed: {e}")

def printHelp():
    rich.print(
        "[bold bright_blue]usage:[/bold bright_blue] "
        f"[bold bright_magenta]python{sys.version_info.major}.{sys.version_info.minor}[/bold bright_magenta] "
        "[[bright_green]-h[/bright_green]] "
        "[[bright_green]-d[/bright_green]] "
        "([bright_cyan]commands[/bright_cyan])"
    )

    rich.print()

    rich.print("[bold bright_blue]commands:[/bold bright_blue]")
    rich.print("  [bold bright_green]run[/bold bright_green]               Run the OS using QEMU")

    rich.print()

    rich.print("[bold bright_blue]options:[/bold bright_blue]")
    rich.print("  [bold bright_green]-h[/bold bright_green], [bold bright_cyan]--help[/bold bright_cyan]        Show this help message and exit")
    rich.print("  [bold bright_green]-d[/bold bright_green], [bold bright_cyan]--debug[/bold bright_cyan]       Enable debug build")

def main() -> bool:
    hostOS: OS
    os_uname = platform.system().lower()
    if (os_uname == "windows"): hostOS = OS.Windows
    elif (os_uname == "darwin"): hostOS = OS.macOS
    elif (os_uname == "linux"): hostOS = OS.Linux
    else: raise ValueError("Unknown architecture")

    hostArch: ARCH
    cpu_arch = platform.machine().lower()
    if (cpu_arch in ["x86_64", "amd64"]): hostArch = ARCH.x86_64
    elif (cpu_arch in ["arm64", "aarch64"]): hostArch = ARCH.ARM64
    else: raise ValueError("Unknown architecture")

    log_dir = Path("logs")
    log_dir.mkdir(parents=True, exist_ok=True)

    logger = logging.getLogger("ci")
    logger.setLevel(logging.DEBUG)

    console_handler = logging.StreamHandler()
    console_handler.setLevel(cLogger.BUILD_LEVEL)
    console_formatter = logging.Formatter("[%(levelname)s] %(message)s")
    console_handler.setFormatter(console_formatter)
    logger.addHandler(console_handler)

    log_file = log_dir / "ci.log"

    file_handler = RotatingFileHandler(
        str(log_file),
        maxBytes=5_000_000,
        backupCount=3,
        encoding="utf-8"
    )
    file_handler.setLevel(logging.DEBUG)
    file_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
    file_handler.setFormatter(file_formatter)
    logger.addHandler(file_handler)

    logger.debug(f"========== NEW RUN {time.strftime("%Y-%m-%d %H:%M:%S")} ==========")


    debug: bool = False
    command_run: bool = False

    try:
        for arg in sys.argv[1:]:
            if arg in ("-h", "--help"):
                printHelp()
                return True

            if arg in ("-d", "--debug"):
                debug = True
            elif arg == "run":
                command_run = True
            else:
                raise ValueError(f"Invalid argument: {arg}")
    
    except Exception as e:
        logger.error(f"Parsing Arguments failed: {e}")
        return False


    try:
        image = build.build(hostOS, hostArch, logger, debug)
        if not image:
            return False
        
    except Exception as e:
        logger.error(f"Building failed: {e}")
        return False
    
    if command_run:
        try:
            run_qemu(image, hostOS)

        except Exception as e:
            logger.error(f"QEMU failed: {e}")
            return False
    
    return True

if not main():
    sys.exit(1)
