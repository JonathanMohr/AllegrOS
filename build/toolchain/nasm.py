from build.toolchain.toolchain import Toolchain, Require_Tool, BuildMode, Architecture
from build.cache import hash_files
from build.compile_commands import Language

from pathlib import Path
import subprocess

def Compile_Assembly_Source(self: Toolchain, mode: BuildMode, src: Path, src_rel: Path, out_dir: Path, doCompileCommands: bool) -> Path:
    if mode.target_arch != Architecture.x86:
        raise RuntimeError(f"Invalid architecture for NASM: {mode.target_arch.name}")

    NASM = Require_Tool("nasm")

    out_file = out_dir / f"{src_rel}.o"

    args: list[str] = []

    flags: list[str] = []
    if mode.debuginfo:
        flags.extend(["-g", "-F", "dwarf"])

    args.extend(flags)
    
    for define in self.defines:
        k, v = define
        if v: args.extend(["-D", f"{k}={v}"])
        else: args.extend(["-D", k])

    for include in self.includeDirectories:
        args.extend(["-I", str(include)])

    args.extend([
        "-f", "elf32",
        str(src),
        "-o", str(out_file)
    ])

    content_hash = hash_files([src], args, self.context.logger)

    if doCompileCommands: self.context.compileCommands.add("nasm", Language.ASSEMBLY, src, out_file, args)
    if not self.context.buildCache.is_up_to_date(out_file, content_hash):
        self.context.logger.build(f"Assembling {src} -> {out_file}")

        out_file.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([NASM, *args], check=True)

        self.context.buildCache.update(out_file, content_hash)

    return out_file
