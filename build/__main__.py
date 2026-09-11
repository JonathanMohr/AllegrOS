from build.defs import OS, Architecture, Baseline, architecture_strings, baseline_strings, architecture_baselines, baseline_architectures
import build.build as build
import build.logger as cLogger

from pathlib import Path
from logging.handlers import RotatingFileHandler
import sys
import platform
import logging
import time
import subprocess
import argparse

def run_debugger(stage1: Path, stage2: Path, kernel: Path):
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


def run_qemu(image: Path, hostOS: OS, debugMode: False):
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

        if debugMode:
            qemu_args.extend(["-S", "-s"])

        qemu = "qemu-system-i386"

        subprocess.run([
            qemu, *qemu_args,
            "-drive", f"format=raw,file={image},if=ide"
        ], check=True)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Running QEMU with {image} failed: {e}")

def main() -> bool:
    hostOS: OS
    os_uname = platform.system().lower()
    if (os_uname == "windows"): hostOS = OS.Windows
    elif (os_uname == "darwin"): hostOS = OS.macOS
    elif (os_uname == "linux"): hostOS = OS.Linux
    else:
        print(f"Unknown uname: {os_uname}")
        return False

    log_dir = Path("logs")
    log_dir.mkdir(parents=True, exist_ok=True)

    argparser = argparse.ArgumentParser()

    argparser.add_argument(
        "-d",
        dest="debug_build",
        action="store_true",
        help="Run debug build"
    )

    argparser.add_argument(
        "--architecture", "-a",
        dest="architecture",
        type=str,
        choices=architecture_strings.keys(),
        default=None,
        help="Set architecture"
    )

    argparser.add_argument(
        "--baseline", "-b",
        dest="baseline",
        type=str,
        choices=baseline_strings.keys(),
        default=None,
        help="Set baseline"
    )

    argparser.add_argument(
        "--run", "-r",
        dest="run",
        action="store_true",
        help="Run"
    )

    argparser.add_argument(
        "--debug", "-g",
        dest="debug",
        action="store_true",
        help="Run with debugger support"
    )

    argparser.add_argument(
        "--debugger", "-e",
        dest="debugger",
        action="store_true",
        help="Run debugger"
    )

    args = argparser.parse_args()

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

    current_time = time.strftime("%Y-%m-%d %H:%M:%S")
    logger.debug(f"========== NEW RUN {current_time} ==========")


    if args.baseline is None and args.architecture is None:
        logger.error("No baseline and architecture set")
        return False

    if args.baseline is not None and args.architecture is None:
        baseline = baseline_strings.get(args.baseline)
        if baseline is None:
            logger.error(f"Invalid baseline: {args.baseline}")
            return False

        architecture = baseline_architectures.get(baseline)
        if architecture is None:
            logger.error(f"Could not find architecture for baseline {baseline.name}")
            return False
    elif args.baseline is None and args.architecture is not None:
        architecture = architecture_strings.get(args.architecture)
        if architecture is None:
            logger.error(f"Invalid architecture: {args.architecture}")
            return False

        baseline = architecture_baselines.get(architecture)
        if baseline is None:
            logger.error(f"Could not find default baseline for architecture {architecture.name}")
            return False
    else: # args.baseline is not None and args.architecture is not None
        architecture = architecture_strings.get(args.architecture)
        if architecture is None:
            logger.error(f"Invalid architecture: {args.architecture}")
            return False

        baseline = baseline_strings.get(args.baseline)
        if baseline is None:
            logger.error(f"Invalid baseline: {args.baseline}")
            return False

        if baseline_architectures.get(baseline) != architecture:
            logger.error(f"Invalid architecture {architecture.name} for baseline {baseline.name}")
            return False

    command_run: bool = args.run
    command_debug: bool = args.debug
    command_debugger: bool = args.debugger


    try:
        result = build.build(logger, baseline, architecture, args.debug_build)
        if not result:
            logger.error("Build failed")
            return False
        
    except Exception as e:
        logger.error(f"Building failed: {e}")
        return False
    
    if command_run:
        try:
            run_qemu(result.Image, hostOS, False)

        except Exception as e:
            logger.error(f"QEMU failed: {e}")
            return False
    
    if command_debug:
        try:
            run_qemu(result.Image, hostOS, True)

        except Exception as e:
            logger.error(f"QEMU failed: {e}")
            return False
        
    if command_debugger:
        try:
            run_debugger(result.Stage1, result.Stage2, result.Kernel)

        except Exception as e:
            logger.error(f"Debugger failed: {e}")
            return False

    return True

if not main():
    sys.exit(1)
