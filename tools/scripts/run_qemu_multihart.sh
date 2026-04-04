#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

APP1="${ROOT_DIR}/apps/zephyr-hart1/build/bin/app1.elf"
APP2="${ROOT_DIR}/apps/freertos-hart2/build/bin/app2.elf"
APP3="${ROOT_DIR}/apps/bare-hart3/build/bin/app3.elf"

for image in "${APP1}" "${APP2}" "${APP3}"; do
    if [ ! -f "${image}" ]; then
        echo "[ERR] Missing QEMU app image: ${image}"
        exit 1
    fi
done

echo "[INFO] Starting QEMU multihart app launch"
echo "[INFO] Hart 1: ${APP1}"
echo "[INFO] Hart 2: ${APP2}"
echo "[INFO] Hart 3: ${APP3}"

exec qemu-system-riscv64 \
    -machine virt \
    -bios none \
    -m 256 \
    -smp 5 \
    -nographic \
    -monitor none \
    -serial stdio \
    -device loader,file="${APP1}",cpu-num=1 \
    -device loader,file="${APP2}",cpu-num=2 \
    -device loader,file="${APP3}",cpu-num=3 \
