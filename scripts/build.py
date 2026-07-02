from scripts.defs import OS, ARCH
import scripts.cache as cache
import scripts.compile_commands as compile_commands

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
    ObjectCopy: str
    Archiver: str

    Assembler_Flags: list[str] = field(default_factory=list)
    Compiler_C_Flags: list[str] = field(default_factory=list)
    Linker_Flags: list[str] = field(default_factory=list)

    Library_Directories: list[Path] = field(default_factory=list)
    Libraries: list[str] = field(default_factory=list)

def build_assembly_sources(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, compileCommands: compile_commands.CompileCommands, build_dir: Path, source_dir: Path) -> list[Path]:
    files: list[Path] = source_dir.rglob("*.asm")

    objects: list[Path] = []
    for file in files:
        rel_path = file.relative_to(source_dir)

        target_path = build_dir / rel_path.with_suffix(".asm.o")
        target_path.parent.mkdir(parents=True, exist_ok=True)

        objects.append(target_path)

        try:
            args = [
                *toolchain.Assembler_Flags,
                str(file),
                "-o", str(target_path)
            ]

            content_hash = cache.hash_files([file])

            # TODO
            #compileCommands.add("nasm", compile_commands.Language.ASSEMBLY, file, target_path, args)
            if not buildCache.is_up_to_date(target_path, content_hash):
                logger.build(f"Assembling {file} -> {target_path}")
                subprocess.run([
                    toolchain.Assembler, *args
                ], check=True)

                buildCache.update(target_path, content_hash)

        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Assembling failed for {file}: {e}")
        
    return objects

def build_c_sources(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, compileCommands: compile_commands.CompileCommands, build_dir: Path, source_dir: Path) -> list[Path]:
    files: list[Path] = source_dir.rglob("*.c")

    objects: list[Path] = []
    for file in files:
        rel_path = file.relative_to(source_dir)

        target_path = build_dir / rel_path.with_suffix(".c.o")
        target_path.parent.mkdir(parents=True, exist_ok=True)
        dep_path = target_path.with_suffix(".d")

        objects.append(target_path)

        try:
            args = [
                *toolchain.Compiler_C_Flags,
                "-c", str(file),
                "-o", str(target_path),
                "-MD", "-MF", str(dep_path)
            ]

            deps = cache.parse_gcc_dep_file(dep_path)
            all_deps = [file, *map(Path, deps)]
            content_hash = cache.hash_files(all_deps)

            # TODO
            compileCommands.add("clang", compile_commands.Language.C, file, target_path, args)
            if not buildCache.is_up_to_date(target_path, content_hash):
                logger.build(f"Compiling {file} -> {target_path}")
                subprocess.run([
                    toolchain.Compiler_C, *args
                ], check=True)

                new_deps = cache.parse_gcc_dep_file(dep_path)
                new_all_deps = [file, *map(Path, new_deps)]
                new_content_hash = cache.hash_files(new_all_deps)
                buildCache.update(target_path, new_content_hash)

        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Compiling failed for {file}: {e}")
        
    return objects

def link_objects(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, out: Path, objects: list[Path], linker_script: Path, map: Path):
    try:
        args = [
            "-T", str(linker_script),
            f"-Map={map}",
            *toolchain.Linker_Flags,
            *[str(o) for o in objects],
            *[f"-L{d}" for d in toolchain.Library_Directories],
            *[f"-l{l}" for l in toolchain.Libraries],
            "-o", str(out)
        ]

        deps = [*objects, linker_script]
        content_hash = cache.hash_files(deps)

        if not buildCache.is_up_to_date(out, content_hash):
            logger.build(f"Linking {out}")
            subprocess.run([
                toolchain.Linker, *args
            ], check=True)

            buildCache.update(out, content_hash)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Linking failed for {out}: {e}")

def link_objects_to_binary(logger: logging.Logger, toolchain: Toolchain,buildCache: cache.BuildCache, elfOut: Path, out: Path, objects: list[Path], linker_script: Path, map: Path):
    try:
        linkerArgs = [
            "-T", str(linker_script),
            f"-Map={map}",
            *toolchain.Linker_Flags,
            *[str(o) for o in objects],
            *[f"-L{d}" for d in toolchain.Library_Directories],
            *[f"-l{l}" for l in toolchain.Libraries],
            "-o", str(elfOut)
        ]

        deps = [*objects, linker_script]
        elf_content_hash = cache.hash_files(deps)

        if not buildCache.is_up_to_date(elfOut, elf_content_hash):
            logger.build(f"Linking ELF {elfOut}")
            subprocess.run([
                toolchain.Linker, *linkerArgs
            ], check=True)

            buildCache.update(elfOut, elf_content_hash)

        objectCopyArgs = [
            "-O", "binary",
            str(elfOut),
            str(out)
        ]

        bin_content_hash = cache.hash_files([elfOut])
        if not buildCache.is_up_to_date(out, bin_content_hash):
            logger.build(f"Creating binary {out}")
            subprocess.run([
                toolchain.ObjectCopy, *objectCopyArgs
            ], check=True)

            buildCache.update(out, bin_content_hash)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Linking failed for {out}: {e}")

def create_static_library(logger: logging.Logger, toolchain: Toolchain,buildCache: cache.BuildCache, out: Path, objects: list[Path]):
    try:
        args = [
            "rcs",
            str(out),
            *[str(o) for o in objects]
        ]

        content_hash = cache.hash_files(objects)

        if not buildCache.is_up_to_date(out, content_hash):
            logger.build(f"Creating static library {out}")
            subprocess.run([
                toolchain.Archiver, *args
            ], check=True)

            buildCache.update(out, content_hash)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Creating static library failed for {out}: {e}")

def compile_libk(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, compileCommands: compile_commands.CompileCommands, src_dir: Path, build_dir: Path):
    src = src_dir / "libk"
    build = build_dir / "libk"

    out = build_dir / "libk.a"

    asm_objects = build_assembly_sources(logger, toolchain, buildCache, compileCommands, build, src)
    c_objects = build_c_sources(logger, toolchain, buildCache, compileCommands, build, src)

    create_static_library(logger, toolchain, buildCache, out, [*asm_objects, *c_objects])

    return out

def compile_bootloader_stage1(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, compileCommands: compile_commands.CompileCommands, src_dir: Path, build_dir: Path) -> tuple[Path, Path]:
    src = src_dir / "bootloader/stage1"
    build = build_dir / "bootloader/stage1"
    linker_script = src / "linker.ld"

    elfOut = build / "stage1.elf"
    out = build / "stage1.bin"
    map_path = build / "stage1.map"
    
    objects = build_assembly_sources(logger, toolchain, buildCache, compileCommands, build, src)
    
    link_objects_to_binary(logger, toolchain, buildCache, elfOut, out, objects, linker_script, map_path)

    return (elfOut, out)

def compile_bootloader_stage2(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, compileCommands: compile_commands.CompileCommands, src_dir: Path, build_dir: Path) -> tuple[Path, Path]:
    src = src_dir / "bootloader/stage2"
    build = build_dir / "bootloader/stage2"
    linker_script = src / "linker.ld"

    elfOut = build / "stage2.elf"
    out = build / "stage2.bin"
    map_path = build / "stage2.map"
    
    asm_objects = build_assembly_sources(logger, toolchain, buildCache, compileCommands, build, src)
    c_objects = build_c_sources(logger, toolchain, buildCache, compileCommands, build, src)

    link_objects_to_binary(logger, toolchain, buildCache, elfOut, out, [*asm_objects, *c_objects], linker_script, map_path)

    return (elfOut, out)

def compile_bootloader_kernel(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, compileCommands: compile_commands.CompileCommands, src_dir: Path, build_dir: Path) -> Path:
    src = src_dir / "kernel"
    build = build_dir / "kernel"
    linker_script = src / "linker.ld"

    out = build / "kernel.elf"
    map_path = build / "kernel.map"
    
    asm_objects = build_assembly_sources(logger, toolchain, buildCache, compileCommands, build, src)
    c_objects = build_c_sources(logger, toolchain, buildCache, compileCommands, build, src)

    link_objects(logger, toolchain, buildCache, out, [*asm_objects, *c_objects], linker_script, map_path)

    return out

def create_disk_image(logger: logging.Logger, toolchain: Toolchain, buildCache: cache.BuildCache, compileCommands: compile_commands.CompileCommands, image: Path, stage1: Path, stage2: Path, fs_root: Path):
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

@dataclass
class BuildResult:
    Image: Path

    Stage1: Path
    Stage2: Path
    Kernel: Path

def build(hostOS: OS, hostArch: ARCH, logger: logging.Logger, debug: bool) -> BuildResult | None:
    buildCache = cache.BuildCache(Path(".buildcache.json"), logger)
    compileCommands = compile_commands.CompileCommands()

    try:
        toolchain: Toolchain = Toolchain(
            LFS = require_tool("phx-lfs"),

            Assembler = require_tool("nasm"),
            Assembler_Flags = [
                "-f", "elf32"
            ],

            Compiler_C = require_tool("clang"),
            Compiler_C_Flags = [
                "-target", "i386-pc-none-elf",
                "-m32",

                "-fno-pic",

                "-ffreestanding", "-nostdinc",

                "-mno-sse", "-mno-sse2",

                "-fno-builtin",

                "-fno-stack-protector",
            ],

            Linker = require_tool("ld.lld"),
            Linker_Flags = [
                "-nostdlib",

                "-z", "noexecstack"
            ],

            ObjectCopy = require_tool("llvm-objcopy"),

            Archiver = require_tool("llvm-ar")
        )
    
    except ToolchainError as e:
        logger.error(f"Toolchain setup failed: {e}")
        return None
    
    if debug:
        toolchain.Assembler_Flags.extend([
            "-g",
            "-F", "dwarf"
        ])

        toolchain.Compiler_C_Flags.extend([
            "-O0",
            "-g",
            "-gdwarf-4"
        ])

    else:
        toolchain.Compiler_C_Flags.extend([
            "-O2"
        ])

        toolchain.Linker_Flags.extend([
            "--gc-sections",
            "--strip-all"
        ])

    
    src_dir = Path("src")
    build_root_dir = Path("build")

    if debug:
        build_dir = build_root_dir / "debug"
    else:
        build_dir = build_root_dir / "release"

    # Libk
    toolchain.Library_Directories.append(build_dir)

    lib_path = src_dir / "libk"
    toolchain.Compiler_C_Flags.append(f"-I{lib_path}")

    try:
        compile_libk(logger, toolchain, buildCache, compileCommands, src_dir, build_dir)

        toolchain.Libraries.append("k")

    except Exception as e:
        buildCache.save()
        logger.error(f"Building libk failed: {e}")
        return None
    

    stage1: Path
    try:
        elfStage1, stage1 = compile_bootloader_stage1(logger, toolchain, buildCache, compileCommands, src_dir, build_dir)

    except Exception as e:
        buildCache.save()
        logger.error(f"Building stage 1 failed: {e}")
        return None
    
    stage2: Path
    try:
        elfStage2, stage2 = compile_bootloader_stage2(logger, toolchain, buildCache, compileCommands, src_dir, build_dir)

    except Exception as e:
        buildCache.save()
        logger.error(f"Building stage 2 failed: {e}")
        return None

    kernel: Path
    try:
        kernel = compile_bootloader_kernel(logger, toolchain, buildCache, compileCommands, src_dir, build_dir)

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
        create_disk_image(logger, toolchain, buildCache, compileCommands, image, stage1, stage2, build_fs_root)

    except Exception as e:
        buildCache.save()
        logger.error(f"Creating disk image: {e}")
        return None

    compileCommandsPath = Path("compile_commands.json")
    compileCommands.write(compileCommandsPath)

    buildCache.save()

    result = BuildResult(
        Image=image,
        Stage1=elfStage1,
        Stage2=elfStage2,
        Kernel=kernel
    )
    
    return result
