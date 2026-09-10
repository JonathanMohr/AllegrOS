from scripts.toolchain.toolchain import Toolchain, Require_Tool, BuildMode, BuildContext, Architecture, Baseline, OPTIMIZATION
from scripts.cache import hash_files
from scripts.compile_commands import Language

from pathlib import Path
import subprocess

def Parse_Dependency_File(dep_file: Path) -> list[str]:
    if not dep_file.exists():
        return []

    content = dep_file.read_text()

    content = content.replace('\\\r\n', ' ').replace('\\\n', ' ')

    tokens: list[str] = []
    current: list[str] = []
    i, n = 0, len(content)

    while i < n:
        c = content[i]

        if c == '\\' and i + 1 < n and content[i + 1] in (' ', '\\', '#'):
            current.append(content[i + 1])
            i += 2
            continue

        if c == '$' and i + 1 < n and content[i + 1] == '$':
            current.append('$')
            i += 2
            continue

        if c.isspace():
            if current:
                tokens.append(''.join(current))
                current = []
            i += 1
            continue

        current.append(c)
        i += 1

    if current:
        tokens.append(''.join(current))

    if not tokens:
        return []

    return tokens[1:]


def Get_Link_Target(mode: BuildMode):
    match mode.target_arch:
        case Architecture.x86:
            return "elf_i386"
    
    raise ValueError("Unknown target_arch")

def Get_Target(mode: BuildMode):
    match mode.target_arch:
        case Architecture.x86:
            return "i386-unknown-none-elf"

    raise ValueError("Unknown target_arch")

def Get_Portability_Flags(mode: BuildMode) -> list[str]:
    flags: list[str] = []

    match mode.baseline:
        case Baseline.i386: flags.append("-march=i386")
        case Baseline.i486: flags.append("-march=i486")
        case Baseline.i586: flags.append("-march=i586")
        case Baseline.i686: flags.append("-march=i686")
        case _:
            raise ValueError(f"Unknown baseline: {mode.baseline}")

    flags.append("-mtune=generic")

    return flags


def Get_Compile_Flags(context: BuildContext, mode: BuildMode) -> list[str]:
    flags = ["-fno-pic", "-fno-pie"]

    # Stuff
    flags.extend(["-mno-sse", "-mno-sse2", "-mno-mmx", "-mno-80387", "-mgeneral-regs-only"])

    # Do not leak absolute paths
    flags.append(f"-fdebug-prefix-map={context.project_root}=project_root")
    flags.append(f"-fmacro-prefix-map={context.project_root}=project_root")

    # Warnings
    flags.extend([
        "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow",
        "-Wformat=2", "-Wundef", "-Wunreachable-code",
        "-Wnull-dereference", "-Wunused-parameter", "-Wformat-security",
        "-Wsign-conversion", "-Wcast-align", "-Wcast-qual", "-Wdouble-promotion",
        "-Wimplicit-fallthrough", "-Wmisleading-indentation", "-Wswitch-enum",
        "-Wloop-analysis", "-Wcomma", "-Wshadow-all", "-Wextra-semi", "-Wheader-hygiene"
    ])

    if mode.werror: flags.append("-Werror")

    if mode.lto: flags.append("-flto=full")

    # Optimization
    Useful_Flags = [
        "-ffunction-sections",
        "-fdata-sections",

        "-fno-ident",
        "-fmerge-all-constants"
    ]

    match mode.optimization:
        case OPTIMIZATION.NONE:
            flags.extend([
                "-O0",
                "-fno-inline",
            ])

        case OPTIMIZATION.SPEED:
            flags.extend([
                "-O3",
                "-funroll-loops",

                *Useful_Flags,
            ])

        case OPTIMIZATION.SIZE:
            flags.extend([
                "-Os",

                *Useful_Flags,
            ])

    flags.extend(Get_Portability_Flags(mode))

    # Assertions
    if not mode.assertions:
        flags.append("-DNDEBUG")

    flags.append("-fno-stack-protector")

    # Debug information
    if mode.debuginfo:
        flags.extend([
            "-g3",
            "-fno-omit-frame-pointer"
        ])
    else:
        if mode.optimization == OPTIMIZATION.NONE:
            flags.append("-fno-omit-frame-pointer")
        else:
            flags.append("-fomit-frame-pointer")

    flags.extend([
        "-ffreestanding",
        "-nostdlib",
        "-nostdinc",
        "-fno-builtin",
    ])
    
    return flags

def Get_Link_Flags(context: BuildContext, mode: BuildMode) -> list[str]:
    flags = [
        "-z", "noexecstack",
        "--no-pie"
    ]

    flags.append("--build-id=none")
    # TODO: Check flags.append(f"-fdebug-prefix-map={context.project_root}=project_root")

    # Optimization
    Useful_Flags = [
        "--gc-sections",
        "--as-needed",
        "-O2"
    ]

    match mode.optimization:
        case OPTIMIZATION.NONE:
            if mode.lto:
                flags.extend([
                    "--lto-O0",
                ])
            flags.append("-O0")

        case OPTIMIZATION.SPEED:
            if mode.lto:
                flags.extend([
                    "--lto-O3"
                ])
            flags.extend(Useful_Flags)

        case OPTIMIZATION.SIZE:
            if mode.lto:
                pass # TODO
            flags.extend(Useful_Flags)

    flags.append("-static")

    # Debug information
    if mode.debuginfo:
        if mode.lto:
            flags.append("--mllvm=-frame-pointer=all")
    else:
        if mode.lto:
            if mode.optimization == OPTIMIZATION.NONE:
                flags.append("--mllvm=-frame-pointer=all")
            else:
                flags.append("--mllvm=-frame-pointer=none")

        flags.append("--strip-all")

    return flags


def Compile_C_Source(self: Toolchain, mode: BuildMode, src: Path, src_rel: Path, out_dir: Path, doCompileCommands: bool) -> Path:
    CLANG = Require_Tool("clang")

    out_file = out_dir / f"{src_rel}.o"
    dep_file = out_dir / f"{src_rel}.d"

    args: list[str] = []

    flags: list[str] = Get_Compile_Flags(self.context, mode)
    flags.append(f"-std={self.stdc}")

    args.append(f"--target={Get_Target(mode)}")
    args.extend(flags)

    for define in self.defines:
        k, v = define
        if v: args.append(f"-D{k}={v}")
        else: args.append(f"-D{k}")

    for includeDirectory in self.includeDirectories:
        args.append(f"-I{includeDirectory}")

    args.extend([
        "-MMD", "-MF", str(dep_file),
        "-c", str(src),
        "-o", str(out_file)
    ])

    deps = Parse_Dependency_File(dep_file)
    content_hash = hash_files([src, *map(Path, deps)], args, self.context.logger)

    if doCompileCommands: self.context.compileCommands.add("clang", Language.C, src, out_file, args)
    if not self.context.buildCache.is_up_to_date(out_file, content_hash):
        self.context.logger.build(f"Compiling {src} -> {out_file}")

        out_file.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([CLANG, *args], check=True)

        new_deps = Parse_Dependency_File(dep_file)
        new_content_hash = hash_files([src, *map(Path, new_deps)], args, self.context.logger)

        self.context.buildCache.update(out_file, new_content_hash)

    return out_file

def Compile_CPP_Source(self: Toolchain, mode: BuildMode, src: Path, src_rel: Path, out_dir: Path, doCompileCommands: bool) -> Path:
    CLANGXX = Require_Tool("clang++")

    out_file = out_dir / f"{src_rel}.o"
    dep_file = out_dir / f"{src_rel}.d"

    args: list[str] = []

    flags: list[str] = Get_Compile_Flags(self.context, mode)
    flags.append(f"-std={self.stdcpp}")

    args.append(f"--target={Get_Target(mode)}")
    args.extend(flags)

    for define in self.defines:
        k, v = define
        if v: args.append(f"-D{k}={v}")
        else: args.append(f"-D{k}")

    for includeDirectory in self.includeDirectories:
        args.append(f"-I{includeDirectory}")

    args.extend([
        "-MMD", "-MF", str(dep_file),
        "-c", str(src),
        "-o", str(out_file)
    ])

    deps = Parse_Dependency_File(dep_file)
    content_hash = hash_files([src, *map(Path, deps)], args, self.context.logger)

    if doCompileCommands: self.context.compileCommands.add("clang++", Language.CPP, src, out_file, args)
    if not self.context.buildCache.is_up_to_date(out_file, content_hash):
        self.context.logger.build(f"Compiling {src} -> {out_file}")

        out_file.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([CLANGXX, *args], check=True)

        new_deps = Parse_Dependency_File(dep_file)
        new_content_hash = hash_files([src, *map(Path, new_deps)], args, self.context.logger)

        self.context.buildCache.update(out_file, new_content_hash)

    return out_file

def Archive_Objects(self: Toolchain, mode: BuildMode, objects: list[Path], name: str, out_dir: Path) -> Path:
    LLVM_AR = Require_Tool("llvm-ar")

    lib = out_dir / f"lib{name}.a"

    args: list[str] = [
        "rcs", str(lib),
        *map(str, objects)
    ]

    content_hash = hash_files(objects, [], self.context.logger)
    if not self.context.buildCache.is_up_to_date(lib, content_hash):
        self.context.logger.build(f"Archiving {lib}")

        lib.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([LLVM_AR, *args], check=True)

        self.context.buildCache.update(lib, content_hash)

    return lib

def Link_Executable(self: Toolchain, mode: BuildMode, objects: list[Path], libraries: list[Path], linker_script: Path | None, name: str, out_dir: Path) -> tuple[Path, Path]:
    LD_LLD = Require_Tool("ld.lld")

    executable = out_dir / f"{name}"
    map_out = out_dir / f"{name}.map"

    args: list[str] = []

    flags: list[str] = Get_Link_Flags(self.context, mode)

    args.extend(["-m", Get_Link_Target(mode)])

    args.extend([*map(str, objects)])

    for libraryDirectory in self.libraryDirectories:
        args.append(f"-L{libraryDirectory}")

    args.extend([*map(str, libraries)])
    args.extend([*map(str, self.libraries)])

    lib_deps: list[Path] = [*libraries, *self.libraries]
    for libraryName in self.libraryNames:
        lib, path = libraryName
        args.append(f"-l{lib}")
        if path is not None:
            lib_deps.append(path)

    args.extend(flags)

    if linker_script is not None:
        args.extend(["-T", str(linker_script)])

    args.extend([
        f"-Map={map_out}",
        "-o", str(executable)
    ])

    content_hash = hash_files([*objects, *lib_deps], args, self.context.logger)

    if not self.context.buildCache.is_up_to_date(executable, content_hash):
        self.context.logger.build(f"Linking {executable}")

        executable.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([LD_LLD, *args], check=True)

        self.context.buildCache.update(executable, content_hash)

    return executable, map_out

def Extract_Binary(self: Toolchain, mode: BuildMode, executable: Path, name: str, out_dir: Path) -> Path:
    LLVM_OBJCOPY = Require_Tool("llvm-objcopy")

    bin = out_dir / f"{name}.bin"
    
    args: list[str] = [
        "-O", "binary",
        str(executable),
        str(bin)
    ]

    content_hash = hash_files([executable], [], self.context.logger)
    if not self.context.buildCache.is_up_to_date(bin, content_hash):
        self.context.logger.build(f"Extracting {executable} -> {bin}")

        bin.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([LLVM_OBJCOPY, *args], check=True)

        self.context.buildCache.update(bin, content_hash)

    return bin
