import subprocess
import os
import shutil

def check_wsl():
    try:
        subprocess.run(['wsl', '--status'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    except subprocess.CalledProcessError:
        raise RuntimeError("WSL ist nicht korrekt installiert. Führe 'wsl --install' in PowerShell aus.")

def check_linux_distribution():
    result = subprocess.run(['wsl', 'lsb_release', '-a'], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError("Keine WSL-Distribution installiert. Bitte z. B. 'Ubuntu' aus dem Microsoft Store installieren.")

def ensure_mkfs_fat():
    result = subprocess.run(['wsl', 'which', 'mkfs.fat'], capture_output=True, text=True)
    if result.returncode != 0 or not result.stdout.strip():
        subprocess.run(['wsl', 'sudo', 'apt', 'update'], check=True)
        subprocess.run(['wsl', 'sudo', 'apt', 'install', '-y', 'dosfstools'], check=True)

def ensure_mtools():
    result = subprocess.run(['wsl', 'which', 'mcopy'], capture_output=True, text=True)
    
    if result.returncode != 0 or not result.stdout.strip():
        subprocess.run(['wsl', 'sudo', 'apt', 'update'], check=True)
        subprocess.run(['wsl', 'sudo', 'apt', 'install', '-y', 'mtools'], check=True)

def ensure_parted():
    result = subprocess.run(['wsl', 'which', 'parted'], capture_output=True, text=True)
    
    if result.returncode != 0 or not result.stdout.strip():
        subprocess.run(['wsl', 'sudo', 'apt', 'update'], check=True)
        subprocess.run(['wsl', 'sudo', 'apt', 'install', '-y', 'parted'], check=True)

def ensure_xxd():
    result = subprocess.run(['wsl', 'which', 'xxd'], capture_output=True, text=True)
    
    if result.returncode != 0 or not result.stdout.strip():
        subprocess.run(['wsl', 'sudo', 'apt', 'update'], check=True)
        subprocess.run(['wsl', 'sudo', 'apt', 'install', '-y', 'xxd'], check=True)


try:
    check_wsl()
    check_linux_distribution()
    ensure_mkfs_fat()
    ensure_mtools()
    ensure_parted()
    ensure_xxd()

except Exception as e:
    print(f"Error: {e}")