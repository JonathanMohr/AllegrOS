from pathlib import Path
from typing import Optional, Dict
import json
import hashlib

class BuildCache:
    def __init__(self, cache_file: Path):
        self.cache_file = cache_file
        self.hashes: Dict[str, str] = {}
        self.load()

    def load(self):
        if self.cache_file.exists():
            try:
                data = json.loads(self.cache_file.read_text())
                self.hashes = data.get("hashes", {})
            except Exception:
                print(f"Warning: Failed to load build cache from {self.cache_file}")
                self.hashes = {}
                self.stored_version = None

    def save(self):
        data = {
            "hashes": self.hashes,
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
    
def hash_files(files: list[Path]) -> str:
    hasher = hashlib.new("sha256")

    for file in sorted(files, key=lambda f: str(f)):
        if not file.exists():
            continue

        with file.open("rb") as f:
            while chunk := f.read(8192):
                hasher.update(chunk)
                
    return hasher.hexdigest()


def parse_gcc_dep_file(dep_path: Path) -> list[str]:
    if not dep_path.exists():
        return []
    
    content = dep_path.read_text()
    content = content.replace('\\\n', ' ')
    parts = content.split()
    if not parts: return []
    
    # first part is target
    return parts[1:]
