#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILDROOT_OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"
LAYOUT_TOOL="${ROOT_DIR}/tools/scripts/amp_layout.py"

eval "$(python3 "${LAYOUT_TOOL}" shell-vars)"

HART0="${ROOT_DIR}/boot/hart0/build/bin/hart0.elf"
APP1="${ROOT_DIR}/apps/zephyr-hart1/build/bin/app1.elf"
APP2="${ROOT_DIR}/apps/freertos-hart2/build/bin/app2.elf"
APP3="${ROOT_DIR}/apps/bare-hart3/build/bin/app3.elf"

# Use pre-built artifacts if available, otherwise use build output
if [ -f "${ROOT_DIR}/artifacts/buildroot/images/Image" ]; then
    LINUX_IMAGE="${ROOT_DIR}/artifacts/buildroot/images/Image"
else
    LINUX_IMAGE="${BUILDROOT_OUTPUT_DIR}/images/Image"
fi

if [ -f "${ROOT_DIR}/artifacts/buildroot/images/rootfs.cpio" ]; then
    LINUX_INITRD="${ROOT_DIR}/artifacts/buildroot/images/rootfs.cpio"
else
    LINUX_INITRD="${BUILDROOT_OUTPUT_DIR}/images/rootfs.cpio"
fi

for image in "${HART0}" "${APP1}" "${APP2}" "${APP3}" "${LINUX_IMAGE}" "${LINUX_INITRD}"; do
    if [ ! -f "${image}" ]; then
        echo "[ERR] Missing preload artifact: ${image}"
        exit 1
    fi
done

echo "[INFO] Starting QEMU Hart0 preload mode"
echo "[INFO] Hart0: ${HART0}"
echo "[INFO] APP1 preload @ ${AMP_HART1_ADDR}: ${APP1}"
echo "[INFO] APP2 preload @ ${AMP_HART2_ADDR}: ${APP2}"
echo "[INFO] APP3 preload @ ${AMP_HART3_ADDR}: ${APP3}"
echo "[INFO] Linux preload @ ${AMP_LINUX_ADDR}: ${LINUX_IMAGE}"
echo "[INFO] Linux initrd: ${LINUX_INITRD}"

exec qemu-system-riscv64 \
    -machine virt \
    -bios default \
    -m 512 \
    -smp 5 \
    -nographic \
    -monitor none \
    -serial stdio \
    -kernel "${HART0}" \
    -initrd "${LINUX_INITRD}" \
    -append "${AMP_BOOTARGS}" \
    -device loader,file="${APP1}" \
    -device loader,file="${APP2}" \
    -device loader,file="${APP3}" \
    -device loader,file="${LINUX_IMAGE}",addr=${AMP_LINUX_ADDR},force-raw=on
