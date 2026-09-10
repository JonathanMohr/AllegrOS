from build.toolchain.toolchain import BuildContext, Toolchain
import build.toolchain.nasm as nasm
import build.toolchain.llvm as llvm

def Get_Toolchain(context: BuildContext) -> Toolchain:
    toolchain = Toolchain(
        context,
        llvm.Compile_C_Source,
        llvm.Compile_CPP_Source,
        nasm.Compile_Assembly_Source,
        llvm.Archive_Objects,
        llvm.Link_Executable,
        llvm.Extract_Binary
    )

    return toolchain
