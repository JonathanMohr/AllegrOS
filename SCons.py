import sys
import subprocess

def check_wsl():
    try:
        subprocess.run(['wsl', '--status'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    except subprocess.CalledProcessError:
        raise RuntimeError("WSL isn't installed. Run 'wsl --install' in PowerShell.")

def check_linux_distribution():
    result = subprocess.run(['wsl', 'lsb_release', '-a'], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError("No WSL distro installed. Install 'Ubuntu' for example.")

def ensure_python_and_scons():
    try:
        result_python = subprocess.run(['wsl', 'which', 'python3'], capture_output=True, text=True)
        if result_python.returncode != 0 or not result_python.stdout.strip():
            subprocess.run(['wsl', 'sudo', 'apt', 'update'], check=True)
            subprocess.run(['wsl', 'sudo', 'apt', 'install', '-y', 'python3'], check=True)
        
        result_scons = subprocess.run(['wsl', 'which', 'scons'], capture_output=True, text=True)
        if result_scons.returncode != 0 or not result_scons.stdout.strip():
            subprocess.run(['wsl', 'pip', 'install', 'scons'], check=True)
    
    except subprocess.CalledProcessError as e:
        print(f"{e}")
    except Exception as e:
        print(f"{e}")

def main():
    check_wsl()
    check_linux_distribution()
    ensure_python_and_scons()

    try:
        subprocess.run(['wsl', 'sudo', 'scons'] + sys.argv[1:], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running SCons: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")

if __name__ == "__main__":
    main()