import subprocess
from pathlib import Path
from typing import Optional, Dict
import hashlib
import json
import sys
import shutil

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

def hash_files(files: list[Path]) -> str:
    hasher = hashlib.new("sha256")

    for file in sorted(files, key=lambda f: str(f)):
        if not file.exists():
            continue

        with file.open("rb") as f:
            while chunk := f.read(8192):
                hasher.update(chunk)
                
    return hasher.hexdigest()

def build_assembly_sources(buildCache: BuildCache, build_dir: Path, source_dir: Path) -> list[Path]:
    nasm = shutil.which("nasm")

    files: list[Path] = source_dir.rglob("*.asm")

    objects: list[Path] = []
    for file in files:
        rel_path = file.relative_to(source_dir)

        target_path = build_dir / rel_path.with_suffix(".o")
        target_path.parent.mkdir(parents=True, exist_ok=True)

        objects.append(target_path)

        try:
            content_hash = hash_files([file])

            if not buildCache.is_up_to_date(target_path, content_hash):
                print(f"Assembling {file} -> {target_path}")
                subprocess.run([
                    nasm,
                    "-f", "elf",
                    str(file),
                    "-o", str(target_path)
                ], check=True)

                buildCache.update(target_path, content_hash)

        except subprocess.CalledProcessError as e:
            print(f"Error: Assembling failed for {file}")
            raise e
        
    return objects

def link_objects(buildCache: BuildCache, out: Path, objects: list[Path], linker_script: Path):
    ld_lld = shutil.which("ld.lld")

    try:
        content_hash = hash_files(objects)

        if not buildCache.is_up_to_date(out, content_hash):
            print(f"Linking {out}")
            subprocess.run([
                ld_lld,
                "-T", str(linker_script),
                *[str(o) for o in objects],
                "-o", str(out)
            ], check=True)

            buildCache.update(out, content_hash)

    except subprocess.CalledProcessError as e:
        print(f"Error: Linking failed for {out}")
        raise e

def compile_bootloader_stage1(buildCache: BuildCache) -> Path:
    src_dir = Path("src/bootloader/stage1")
    build_dir = Path("build/bootloader/stage1")
    linker_script = src_dir / "linker.ld"

    out = Path("build/bootloader/stage1.bin")
    #map_path = Path("build/bootloader/stage1.map")
    
    objects = build_assembly_sources(buildCache, build_dir, src_dir)
    
    link_objects(buildCache, out, objects, linker_script)

    return out

def compile_bootloader_stage2(buildCache: BuildCache) -> Path:
    src_dir = Path("src/bootloader/stage2")
    build_dir = Path("build/bootloader/stage2")
    linker_script = src_dir / "linker.ld"

    out = Path("build/bootloader/stage2.bin")
    #map_path = Path("build/bootloader/stage2.map")
    
    objects = build_assembly_sources(buildCache, build_dir, src_dir)

    link_objects(buildCache, out, objects, linker_script)

    return out

def create_disk_image(buildCache: BuildCache, image: Path, stage1: Path, stage2: Path):
    lfs = shutil.which("lfs")

    deps = [stage1, stage2]
    content_hash = hash_files(deps)

    if not buildCache.is_up_to_date(image, content_hash):
        try:
            print(f"Creating MBR disk image {image}")
            subprocess.run([
                lfs, "create", str(image), "mbr",
                "--size", "16M",
                "--boot", str(stage1)
            ], check=True)

        except subprocess.CalledProcessError as e:
            print(f"Error: Creating MBR disk image {image} failed")
            raise e
        
        try:
            print(f"Creating partition 1")
            subprocess.run([
                lfs, "create", f"{image}:1", "none",
                "--size", str(stage2.stat().st_size + (512 - (stage2.stat().st_size % 512))),
            ], check=True)

            print(f"Writing stage 2 to partition 1")
            subprocess.run([
                lfs, "write", f"{image}:1", str(stage2)
            ], check=True)

        except subprocess.CalledProcessError as e:
            print(f"Error: Creating partition 1 with {stage2} failed")
            raise e
        
        buildCache.update(image, content_hash)

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
    buildCache: BuildCache = BuildCache(Path(".buildcache.json"))

    stage1: Path
    try:
        stage1 = compile_bootloader_stage1(buildCache)

    except Exception as e:
        buildCache.save()
        print(f"Error: Building stage 1 failed: {e}")
        sys.exit(1)
    
    stage2: Path
    try:
        stage2 = compile_bootloader_stage2(buildCache)

    except Exception as e:
        buildCache.save()
        print(f"Error: Building stage 2 failed: {e}")
        sys.exit(1)

    image = Path("build/disk.img")
    try:
        create_disk_image(buildCache, image, stage1, stage2)

    except Exception as e:
        buildCache.save()
        print(f"Error: Creating disk image: {e}")
        sys.exit(1)

    buildCache.save()

    run_qemu(image)
