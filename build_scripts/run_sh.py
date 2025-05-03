import sys
import subprocess

def convert_line_endings(script_path):
    with open(script_path, 'rb') as f:
        content = f.read()

    content = content.replace(b'\r', b'')

    with open(script_path, 'wb') as f:
        f.write(content)

def ensure_permissions(script_path):
    subprocess.run(['wsl', 'chmod', '+x', script_path], check=True)


convert_line_endings(sys.argv[1])
ensure_permissions(sys.argv[1])
subprocess.run(['wsl', 'sudo', 'bash', sys.argv[1]] + sys.argv[2:], check=True)