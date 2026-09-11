from build.defs import baseline_architectures
import build.cache as cache
import build.compile_commands as compile_commands

from dataclasses import dataclass
from pathlib import Path
import subprocess
import shutil
import logging

from build.toolchain.toolchain import Toolchain, Require_Tool, BuildContext, BuildMode, Architecture, Baseline, OPTIMIZATION
from build.toolchain.get import Get_Toolchain

def Build_Sources_To_Objects(logger: logging.Logger, toolchain: Toolchain, mode: BuildMode, src_dir: Path, build_dir: Path, doCompileCommands: bool) -> list[Path]:
    patterns = ["*.c", "*.cpp", "*.asm"]

    files: list[Path] = []
    for pattern in patterns:
        files.extend(src_dir.rglob(pattern))

    objects: list[Path] = []
    for file in files:
        try:
            if file.suffix == ".c":
                object = toolchain.Compile_C_Source(mode, file, file.relative_to(src_dir), build_dir, doCompileCommands)
            elif file.suffix == ".cpp":
                object = toolchain.Compile_CPP_Source(mode, file, file.relative_to(src_dir), build_dir, doCompileCommands)
            elif file.suffix == ".asm":
                object = toolchain.Compile_Assembly_Source(mode, file, file.relative_to(src_dir), build_dir, False) # TODO
            else:
                logger.warning(f"Invalid source extension of file {file}")
                continue

        except Exception as e:
            logger.error(f"Compilation of {file} failed: {e}")
            raise e

        objects.append(object)

    return objects


def Build_Binary(logger: logging.Logger, toolchain: Toolchain, mode: BuildMode, libraries: list[Path], src_dir: Path, build_dir: Path, linker_script: Path, name: str) -> tuple[Path, Path, Path]:
    objects = Build_Sources_To_Objects(logger, toolchain, mode, src_dir, build_dir, True)

    executable, executable_map = toolchain.Link_Executable(mode, objects, libraries, linker_script, name, build_dir)
    binary = toolchain.Extract_Binary(mode, executable, name, build_dir)

    return executable, executable_map, binary


def Create_Root(logger: logging.Logger, fs_root: Path, kernel: Path, out_root: Path):
    if out_root.exists():
        shutil.rmtree(str(out_root))
    out_root.mkdir(parents=True, exist_ok=True)

    # sys/
    system_dir = out_root / "sys"
    system_dir.mkdir(parents=True, exist_ok=True)

    ## kernel.elf
    kernel_path = system_dir / "kernel.elf"
    shutil.copy2(str(kernel), str(kernel_path))

    # fs_root
    shutil.copytree(str(fs_root), str(out_root), dirs_exist_ok=True)

def Create_Disk_Image(logger: logging.Logger, buildCache: cache.BuildCache, root: Path, out_dir: Path, stage1: Path, stage2: Path) -> Path:
    PHX_LFS = Require_Tool("phx-lfs")

    all_root_files = sorted((f for f in root.rglob("*") if f.is_file()), key=lambda p: p.as_posix())
    content_hash = cache.hash_files([stage1, stage2, *all_root_files], [*map(str, all_root_files)], logger)

    image = out_dir / "image.img"

    if not buildCache.is_up_to_date(image, content_hash):
        image.parent.mkdir(parents=True, exist_ok=True)

        logger.build(f"Creating MBR disk image at {image}")
        subprocess.run([
            PHX_LFS, "create", str(image), "mbr",
            "--size", "100M",
            "--boot", str(stage1)
        ])

        logger.build(f"Creating partition 1 of {image}")
        subprocess.run([
            PHX_LFS, "create", f"{image}:1", "none",
            "--size", str(stage2.stat().st_size + (512 - (stage2.stat().st_size % 512)))
        ])

        logger.build(f"Writing stage 2 to partition 1 of {image}")
        subprocess.run([
            PHX_LFS, "write", f"{image}:1", str(stage2)
        ])

        logger.info(f"Creating partition 2 of {image}")
        subprocess.run([
            PHX_LFS, "create", f"{image}:2", "fat32",
            "--root", str(root)
        ], check=True)

    return image

@dataclass
class BuildResult:
    Image: Path

    Stage1: Path
    Stage2: Path
    Kernel: Path

def build(logger: logging.Logger, baseline: Baseline, architecture: Architecture, debug: bool) -> BuildResult | None:
    project_root = Path(".")

    compileCommandsPath = project_root / "compile_commands.json"
    
    fs_root = project_root / "fs_root"
    src_dir = project_root / "src"
    build_root_dir = project_root / ".build"

    buildCache = cache.BuildCache(build_root_dir / "cache.json", logger)
    compileCommands = compile_commands.CompileCommands()

    buildContext = BuildContext(logger, buildCache, compileCommands, str(project_root.resolve()))
    toolchain = Get_Toolchain(buildContext)

    if debug:
        build_mode = BuildMode(
            architecture,
            baseline,
            False,
            False,
            OPTIMIZATION.NONE,
            True,
            True
        )
    else:
        build_mode = BuildMode(
            architecture,
            baseline,
            False,
            True,
            OPTIMIZATION.SPEED,
            False,
            False
        )

    if debug:
        mode_path = "debug"
    else:
        mode_path = "release"

    if baseline_architectures.get(build_mode.baseline) != build_mode.target_arch:
        raise RuntimeError(f"Invalid combination of baseline {build_mode.baseline.name} and architecture {build_mode.target_arch.name}")

    match build_mode.baseline:
        case Baseline.i386: target_path = "x86-i386"
        case Baseline.i486: target_path = "x86-i486"
        case Baseline.i586: target_path = "x86-i586"
        case Baseline.i686: target_path = "x86-i686"

    build_dir = build_root_dir / mode_path / target_path

    bootloader_src = src_dir / "bootloader"
    bootloader_build = build_dir / "bootloader"

    kernel_src = src_dir / "kernel"
    kernel_build = build_dir / "kernel"

    libk_src = src_dir / "libk"
    libk_build = build_dir / "libk"

    userspace_dir = src_dir / "userspace"

    root_build = build_dir / "root"

    toolchain.Add_Include_Directory(libk_src)

    result = None

    try:
        libk_objects = Build_Sources_To_Objects(logger, toolchain, build_mode, libk_src, libk_build, True)
        libk = toolchain.Archive_Objects(build_mode, libk_objects, "k", libk_build)

        stage1_elf, stage1_map, stage1_bin = Build_Binary(logger, toolchain, build_mode, [], bootloader_src / "stage1", bootloader_build / "stage1", bootloader_src / "stage1" / "linker.ld", "stage1")
        stage2_elf, stage2_map, stage2_bin = Build_Binary(logger, toolchain, build_mode, [libk], bootloader_src / "stage2", bootloader_build / "stage2", bootloader_src / "stage2" / "linker.ld", "stage2")

        kernel_objects = Build_Sources_To_Objects(logger, toolchain, build_mode, kernel_src, kernel_build, True)
        kernel, kernel_map = toolchain.Link_Executable(build_mode, kernel_objects, [libk], kernel_src / "linker.ld", "kernel", kernel_build)

        Create_Root(logger, fs_root, kernel, root_build)

        image = Create_Disk_Image(logger, buildCache, root_build, build_dir, stage1_bin, stage2_bin)

        result = BuildResult(
            image,
            stage1_elf,
            stage2_elf,
            kernel
        )

    except Exception as e:
        logger.error(f"Build failed: {e}")
        result = None

    buildCache.save()
    compileCommands.write(compileCommandsPath)

    return result
