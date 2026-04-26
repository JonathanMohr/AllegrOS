from scripts.defs import OS, ARCH
import scripts.cache as cache

from dataclasses import dataclass, field
from pathlib import Path
import subprocess
import shutil
import logging

@dataclass
class Toolchain:
    LFS: str

    Assembler: str
    Compiler_C: str
    Linker: str

    Assembler_Flags: list[str] = field(default_factory=list)
    Compiler_C_Flags: list[str] = field(default_factory=list)
    Linker_Flags: list[str] = field(default_factory=list)

def build_assembly_sources(logger: logging.Logger, toolchain: Toolchain,buildCache: cache.BuildCache, build_dir: Path, source_dir: Path) -> list[Path]:
    files: list[Path] = source_dir.rglob("*.asm")

    objects: list[Path] = []
    for file in files:
        rel_path = file.relative_to(source_dir)

        target_path = build_dir / rel_path.with_suffix(".asm.o")
        target_path.parent.mkdir(parents=True, exist_ok=True)

        objects.append(target_path)

        try:
            content_hash = cache.hash_files([file])

            if not buildCache.is_up_to_date(target_path, content_hash):
                logger.build(f"Assembling {file} -> {target_path}")
                subprocess.run([
                    toolchain.Assembler,
                    *toolchain.Assembler_Flags,
                    str(file),
                    "-o", str(target_path)
                ], check=True)

                buildCache.update(target_path, content_hash)

        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Assembling failed for {file}: {e}")
        
    return objects

def build_c_sources(logger: logging.Logger, toolchain: Toolchain,buildCache: cache.BuildCache, build_dir: Path, source_dir: Path) -> list[Path]:
    files: list[Path] = source_dir.rglob("*.c")

    objects: list[Path] = []
    for file in files:
        rel_path = file.relative_to(source_dir)

        target_path = build_dir / rel_path.with_suffix(".c.o")
        target_path.parent.mkdir(parents=True, exist_ok=True)
        dep_path = target_path.with_suffix(".d")

        objects.append(target_path)

        try:
            deps = cache.parse_gcc_dep_file(dep_path)
            all_deps = [file, *map(Path, deps)]
            content_hash = cache.hash_files(all_deps)

            if not buildCache.is_up_to_date(target_path, content_hash):
                logger.build(f"Compiling {file} -> {target_path}")
                subprocess.run([
                    toolchain.Compiler_C,
                    *toolchain.Compiler_C_Flags,
                    "-c", str(file),
                    "-o", str(target_path),
                    "-MD", "-MF", str(dep_path)
                ], check=True)

                new_deps = cache.parse_gcc_dep_file(dep_path)
                new_all_deps = [file, *map(Path, new_deps)]
                new_content_hash = cache.hash_files(new_all_deps)
                buildCache.update(target_path, new_content_hash)

        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Compiling failed for {file}: {e}")
        
    return objects

def link_objects(logger: logging.Logger, toolchain: Toolchain,buildCache: cache.BuildCache, out: Path, objects: list[Path], linker_script: Path, map: Path):
    try:
        deps = [*objects, linker_script]
        content_hash = cache.hash_files(deps)

        if not buildCache.is_up_to_date(out, content_hash):
            logger.build(f"Linking {out}")
            subprocess.run([
                toolchain.Linker,
                "-T", str(linker_script),
                f"-Map={map}",
                *toolchain.Linker_Flags,
                *[str(o) for o in objects],
                "-o", str(out)
            ], check=True)

            buildCache.update(out, content_hash)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Error: Linking failed for {out}: {e}")

def compile_bootloader_stage1(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, src_dir: Path, build_dir: Path) -> Path:
    src = src_dir / "bootloader/stage1"
    build = build_dir / "bootloader/stage1"
    linker_script = src / "linker.ld"

    out = build / "stage1.bin"
    map_path = build / "stage1.map"
    
    objects = build_assembly_sources(logger, toolchain, buildCache, build, src)
    
    link_objects(logger, toolchain, buildCache, out, objects, linker_script, map_path)

    return out

def compile_bootloader_stage2(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, src_dir: Path, build_dir: Path) -> Path:
    src = src_dir / "bootloader/stage2"
    build = build_dir / "bootloader/stage2"
    linker_script = src / "linker.ld"

    out = build / "stage2.bin"
    map_path = build / "stage2.map"
    
    asm_objects = build_assembly_sources(logger, toolchain, buildCache, build, src)
    c_objects = build_c_sources(logger, toolchain, buildCache, build, src)

    link_objects(logger, toolchain, buildCache, out, [*asm_objects, *c_objects], linker_script, map_path)

    return out

def compile_bootloader_kernel(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, src_dir: Path, build_dir: Path) -> Path:
    src = src_dir / "kernel"
    build = build_dir / "kernel"
    linker_script = src / "linker.ld"

    out = build / "kernel.elf"
    map_path = build / "kernel.map"
    
    asm_objects = build_assembly_sources(logger, toolchain, buildCache, build, src)
    c_objects = build_c_sources(logger, toolchain, buildCache, build, src)

    link_objects(logger, toolchain, buildCache, out, [*asm_objects, *c_objects], linker_script, map_path)

    return out

def create_disk_image(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, image: Path, stage1: Path, stage2: Path, fs_root: Path):
    all_root_files = [f for f in fs_root.rglob("*") if f.is_file()]
    deps = [stage1, stage2, *all_root_files]
    content_hash = cache.hash_files(deps)

    if not buildCache.is_up_to_date(image, content_hash):
        try:
            logger.debug(f"Creating MBR disk image {image}")
            subprocess.run([
                toolchain.LFS, "create", str(image), "mbr",
                "--size", "100M",
                "--boot", str(stage1)
            ], check=True)

        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Creating MBR disk image {image} failed: {e}")
        
        try:
            logger.info(f"Creating partition 1")
            subprocess.run([
                toolchain.LFS, "create", f"{image}:1", "none",
                "--size", str(stage2.stat().st_size + (512 - (stage2.stat().st_size % 512))),
            ], check=True)

            logger.debug(f"Writing stage 2 to partition 1")
            subprocess.run([
                toolchain.LFS, "write", f"{image}:1", str(stage2)
            ], check=True)

        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Creating partition 1 with {stage2} failed: {e}")

        try:
            logger.info(f"Creating partition 2")
            subprocess.run([
                toolchain.LFS, "create", f"{image}:2", "fat32",
                "--root", str(fs_root)
            ], check=True)

        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Creating partition 2 with {stage2} failed: {e}")
        
        buildCache.update(image, content_hash)

class ToolchainError(RuntimeError):
    pass

def require_tool(name: str) -> str:
    path = shutil.which(name)
    if not path:
        raise ToolchainError(f"Missing required tool: {name}")
    return path

def build(hostOS: OS, hostArch: ARCH, logger: logging.Logger, debug: bool) -> Path | None:
    buildCache: cache.BuildCache = cache.BuildCache(Path(".buildcache.json"), logger)

    try:
        toolchain: Toolchain = Toolchain(
            LFS = require_tool("lfs"),

            Assembler = require_tool("nasm"),
            Assembler_Flags = [
                "-f", "elf32"
            ],

            Compiler_C = require_tool("clang"),
            Compiler_C_Flags = [
                "-target", "i386-pc-none-elf",
                "-m32",
                "-ffreestanding", "-nostdinc",
                "-mno-sse", "-mno-sse2"
            ],

            Linker = require_tool("ld.lld"),
            Linker_Flags = [
                "-nostdlib"
            ]
        )
    
    except ToolchainError as e:
        logger.error(f"Toolchain setup failed: {e}")
        return None
    
    if debug:
        toolchain.Compiler_C_Flags.extend([
            "-O0",
            "-g"
        ])
    else:
        toolchain.Compiler_C_Flags.extend([
            "-O2"
        ])

    
    src_dir = Path("src")
    build_root_dir = Path("build")

    if debug:
        build_dir = build_root_dir / "debug"
    else:
        build_dir = build_root_dir / "release"

    # TODO: hardcoded
    lib_path = Path("src/libs/core")
    toolchain.Compiler_C_Flags.append(f"-I{lib_path}")

    lib_path = Path("src/libs/boot")
    toolchain.Compiler_C_Flags.append(f"-I{lib_path}")

    stage1: Path
    try:
        stage1 = compile_bootloader_stage1(logger, toolchain, buildCache, src_dir, build_dir)

    except Exception as e:
        buildCache.save()
        logger.error(f"Building stage 1 failed: {e}")
        return None
    
    stage2: Path
    try:
        stage2 = compile_bootloader_stage2(logger, toolchain, buildCache, src_dir, build_dir)

    except Exception as e:
        buildCache.save()
        logger.error(f"Building stage 2 failed: {e}")
        return None

    kernel: Path
    try:
        kernel = compile_bootloader_kernel(logger, toolchain, buildCache, src_dir, build_dir)

    except Exception as e:
        buildCache.save()
        logger.error(f"Building kernel failed: {e}")
        return None
    
    build_fs_root = build_dir / "fs_root"
    shutil.rmtree(build_fs_root, ignore_errors=True)
    build_fs_root.mkdir(parents=True, exist_ok=True)

    fs_root = Path("fs_root")
    shutil.copytree(fs_root, build_fs_root, dirs_exist_ok=True)

    system_dir = build_fs_root / "sys"
    system_dir.mkdir(parents=True, exist_ok=True)

    kernel_path = system_dir / "kernel.elf"
    shutil.copy2(kernel, kernel_path)

    image = build_dir / "disk.img"
    try:
        create_disk_image(logger, toolchain, buildCache, image, stage1, stage2, build_fs_root)

    except Exception as e:
        buildCache.save()
        logger.error(f"Creating disk image: {e}")
        return None

    buildCache.save()
    
    return image
