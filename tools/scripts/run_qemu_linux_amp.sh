#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
LAYOUT_TOOL="${ROOT_DIR}/tools/scripts/amp_layout.py"

eval "$(python3 "${LAYOUT_TOOL}" shell-vars)"

BUILDROOT_OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"

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

APP1="${ROOT_DIR}/apps/zephyr-hart1/build/bin/app1.elf"
APP2="${ROOT_DIR}/apps/freertos-hart2/build/bin/app2.elf"
APP3="${ROOT_DIR}/apps/bare-hart3/build/bin/app3.elf"
HART0="${ROOT_DIR}/boot/hart0/build/bin/hart0.elf"

for image in "${HART0}" "${LINUX_IMAGE}" "${LINUX_INITRD}" "${APP1}" "${APP2}" "${APP3}"; do
    if [ ! -f "${image}" ]; then
        echo "[ERR] Missing Linux AMP artifact: ${image}"
        echo "      Buildroot/Linux setup lives under deps/buildroot and still needs to be built."
        exit 1
    fi
done

echo "[INFO] Starting QEMU Linux AMP launch"
echo "[INFO] Hart 0 chainloader: ${HART0}"
echo "[INFO] Hart 0 Linux image: ${LINUX_IMAGE}"
echo "[INFO] Hart 1 Zephyr: ${APP1}"
echo "[INFO] Hart 2 FreeRTOS: ${APP2}"
echo "[INFO] Hart 3 Bare-metal: ${APP3}"

exec qemu-system-riscv64 \
    -machine virt \
    -bios default \
    -m 512 \
    -smp 4 \
    -nographic \
    -monitor none \
    -serial stdio \
    -kernel "${HART0}" \
    -initrd "${LINUX_INITRD}" \
    -append "${AMP_BOOTARGS}" \
    -device loader,file="${LINUX_IMAGE}",addr=${AMP_LINUX_ADDR},force-raw=on \
    -device loader,file="${APP1}" \
    -device loader,file="${APP2}" \
    -device loader,file="${APP3}"
