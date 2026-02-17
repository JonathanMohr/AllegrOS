import subprocess
from pathlib import Path
from typing import Optional, Dict
import hashlib
import json

class BuildCache:
    def __init__(self, cache_file: Path):
        self.cache_file = cache_file
        self.hashes: Dict[str, str] = {}
        self.stored_version: Optional[str] = None
        self.load()

    def load(self):
        if self.cache_file.exists():
            try:
                data = json.loads(self.cache_file.read_text())
                self.hashes = data.get("hashes", {})
                self.stored_version = data.get("version")
            except Exception:
                print(f"Warning: Failed to load build cache from {self.cache_file}")
                self.hashes = {}
                self.stored_version = None

    def save(self):
        data = {
            "hashes": self.hashes,
            "version": self.stored_version
        }
        self.cache_file.write_text(json.dumps(data, indent=4, ensure_ascii=False))

    def get(self, target: Path) -> Optional[str]:
        return self.hashes.get(str(target))

    def update(self, target: Path, hash_value: str):
        self.hashes[str(target)] = hash_value

    def is_up_to_date(self, target: Path, hash_value: str) -> bool:
        if not target.exists():
            return False
        return self.get(target) == hash_value
    
    def is_version_up_to_date(self, version: str) -> bool:
        up_to_date = self.stored_version == version
        self.stored_version = version
        return up_to_date

def parse_gcc_dep_file(dep_path: Path) -> list[str]:
    if not dep_path.exists():
        return []
    
    content = dep_path.read_text()
    content = content.replace('\\\n', ' ')
    parts = content.split()
    if not parts: return []
    
    # first part is target
    return parts[1:]

def hash_files(files: list[Path]) -> str:
    hasher = hashlib.new("sha256")

    for file in sorted(files, key=lambda f: str(f)):
        if not file.exists():
            continue

        with file.open("rb") as f:
            while chunk := f.read(8192):
                hasher.update(chunk)
                
    return hasher.hexdigest()

def compile_bootloader_stage1() -> Path:
    nasm = "nasm"
    
    src = Path("src/bootloader/stage1/boot.asm")
    out = Path("build/bootloader/stage1/stage1.bin")
    map_path = Path("build/bootloader/stage1/stage1.lst")

    out.parent.mkdir(parents=True, exist_ok=True)

    print(f"Assembling {src} -> {out}")
    subprocess.run([nasm, "-f", "bin", str(src), "-o", str(out), "-l", str(map_path)])

    return out

def compile_bootloader_stage2() -> Path:
    nasm = "nasm"

    src = Path("src/bootloader/stage2/main.asm")
    out = Path("build/bootloader/stage2.bin")

    out.parent.mkdir(parents=True, exist_ok=True)

    print(f"Assembling {src} -> {out}")
    subprocess.run([nasm, "-f", "bin", str(src), "-o", str(out)])

    return out

def run_qemu(image: Path):
    qemu_args = [
        "-m", "32",
#       "-spice", "port=5930,disable-ticketing",
        "-display", "sdl,gl=on",
        "-debugcon", "stdio"
    ]

    qemu = "qemu-system-i386"

    subprocess.run([
        qemu, *qemu_args,
        "-drive", f"format=raw,file={image},if=ide"
    ])

if __name__ == "__main__":
    stage1 = compile_bootloader_stage1()
    stage2 = compile_bootloader_stage2()

    image = Path("build/disk.img")

    subprocess.run([
        "lfs", "create", str(image), "mbr", "--size", "16M", "--boot", str(stage1)
    ])

    subprocess.run([
        "lfs", "create", f"{image}:1", "none", "--size", str(stage2.stat().st_size + (512 - (stage2.stat().st_size % 512)))
    ])

    subprocess.run([
        "lfs", "write", f"{image}:1", str(stage2)
    ])

    run_qemu(image)
