from typing import Callable, TypeAlias
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
import logging
import shutil

from build.cache import BuildCache
from build.compile_commands import CompileCommands

class Baseline(Enum):
    i386 = 1
    i486 = 2
    i586 = 3
    i686 = 4

class Architecture(Enum):
    x86 = 1

class OPTIMIZATION(Enum):
    NONE = 1
    SPEED = 2
    SIZE = 3

@dataclass()
class BuildContext:
    logger: logging.Logger
    buildCache: BuildCache
    compileCommands: CompileCommands
    project_root: str

@dataclass()
class BuildMode:
    target_arch: Architecture
    baseline: Baseline

    werror: bool

    lto: bool

    optimization: OPTIMIZATION

    assertions: bool
    debuginfo: bool


class Toolchain:
    # self, mode, src, src_rel, out_dir, doCompileCommands -> object
    Compile_Function: TypeAlias = Callable[["Toolchain", BuildMode, Path, Path, Path, bool], Path]
    # self, mode, objects, name, out_dir -> lib
    Archive_Function: TypeAlias = Callable[["Toolchain", BuildMode, list[Path], str, Path, bool], Path]
    # self, mode, objects, libraries, linker_script, name, out_dir -> executable, map
    Link_Executable_Function: TypeAlias = Callable[["Toolchain", BuildMode, list[Path], list[Path], Path | None, str, Path], tuple[Path, Path]]
    # self, mode, executable, name, out_dir -> binary
    Extract_Binary_Function: TypeAlias = Callable[["Toolchain", BuildMode, Path, str, Path], Path]

    _Compile_C_Source: Compile_Function
    _Compile_CPP_Source: Compile_Function
    _Compile_Assembly_Source: Compile_Function
    _Archive_Objects: Archive_Function
    _Link_Executable: Link_Executable_Function
    _Extract_Binary: Extract_Binary_Function

    def __init__(self, context: BuildContext,
                 Compile_C_Source: Compile_Function,
                 Compile_CPP_Source: Compile_Function,
                 Compile_Assembly_Source: Compile_Function,
                 Archive_Objects: Archive_Function,
                 Link_Executable: Link_Executable_Function,
                 Extract_Binary: Extract_Binary_Function):
        self.context = context

        self.defines: list[tuple[str, str]] = []
        self.includeDirectories: list[Path] = []
        self.libraryDirectories: list[Path] = []
        self.libraryNames: list[tuple[str, Path | None]] = []
        self.libraries: list[Path] = []

        self.stdc: str = "c99"
        self.stdcpp: str = "c++98"

        self._Compile_C_Source = Compile_C_Source
        self._Compile_CPP_Source = Compile_CPP_Source
        self._Compile_Assembly_Source = Compile_Assembly_Source
        self._Archive_Objects = Archive_Objects
        self._Link_Executable = Link_Executable
        self._Extract_Binary = Extract_Binary

    def Add_Define(self, name: str, value: str | None = None):
        val = value
        if val is None:
            val = ""
        self.defines.append((name, val))

    def Add_Include_Directory(self, dir: Path):
        self.includeDirectories.append(dir)

    def Add_Library_Directory(self, dir: Path):
        self.libraryDirectories.append(dir)

    def Add_Library_Name(self, lib: str, path: Path | None):
        self.libraryNames.append((lib, path))

    def Add_Library(self, lib: Path):
        self.libraries.append(lib)


    def Compile_C_Source(self, mode: BuildMode, src: Path, src_rel: Path, out_dir: Path, doCompileCommands: bool) -> Path:
        return self._Compile_C_Source(self, mode, src, src_rel, out_dir, doCompileCommands)
    
    def Compile_CPP_Source(self, mode: BuildMode, src: Path, src_rel: Path, out_dir: Path, doCompileCommands: bool) -> Path:
        return self._Compile_CPP_Source(self, mode, src, src_rel, out_dir, doCompileCommands)

    def Compile_Assembly_Source(self, mode: BuildMode, src: Path, src_rel: Path, out_dir: Path, doCompileCommands: bool) -> Path:
        return self._Compile_Assembly_Source(self, mode, src, src_rel, out_dir, doCompileCommands)

    def Archive_Objects(self, mode: BuildMode, objects: list[Path], name: str, out_dir: Path) -> Path:
        return self._Archive_Objects(self, mode, objects, name, out_dir)

    def Link_Executable(self, mode: BuildMode, objects: list[Path], libraries: list[Path], linker_script: Path | None, name: str, out_dir: Path) -> tuple[Path, Path]:
        return self._Link_Executable(self, mode, objects, libraries, linker_script, name, out_dir)

    def Extract_Binary(self, mode: BuildMode, executable: Path, name: str, out_dir: Path) -> Path:
        return self._Extract_Binary(self, mode, executable, name, out_dir)

class ToolchainError(RuntimeError):
    pass

def Require_Tool(name: str) -> str:
    path = shutil.which(name)
    if not path:
        raise ToolchainError(f"Missing required tool: {name}")
    return path
