@echo off

python -m venv .venv

.venv\Scripts\python -m pip install --upgrade pip
.venv\Scripts\python -m pip install -r requirements.txt

powershell -NoExit -ExecutionPolicy Bypass -Command ".venv\Scripts\Activate.ps1"
