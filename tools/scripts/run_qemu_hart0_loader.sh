#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
HART0="${ROOT_DIR}/boot/hart0/build/bin/hart0.elf"

if [ ! -f "${HART0}" ]; then
    echo "[ERR] Missing Hart0 image: ${HART0}"
    echo "      Run: make hart0"
    exit 1
fi

echo "[INFO] Starting QEMU with Hart0 UART loader"
echo "[INFO] Hart0: ${HART0}"
echo "[INFO] Wait for QEMU to print: char device redirected to /dev/pts/N"
echo "[INFO] Then run: python3 tools/send_multihart_apps.py /dev/pts/N"

exec qemu-system-riscv64 \
    -machine virt \
    -bios default \
    -m 512 \
    -smp 5 \
    -nographic \
    -monitor none \
    -serial pty \
    -kernel "${HART0}"
