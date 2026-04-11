#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
LAYOUT_TOOL="${ROOT_DIR}/tools/scripts/amp_layout.py"

eval "$(python3 "${LAYOUT_TOOL}" shell-vars)"

HART0="${ROOT_DIR}/boot/hart0/build/bin/hart0.elf"
APP1="${ROOT_DIR}/apps/zephyr-hart1/build/bin/app1.bin"
APP2="${ROOT_DIR}/apps/freertos-hart2/build/bin/app2.bin"
APP3="${ROOT_DIR}/apps/bare-hart3/build/bin/app3.bin"
BUILDROOT_OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"
FW_JUMP="${ROOT_DIR}/deps/opensbi/build/platform/generic/firmware/fw_jump.bin"

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
WORK_DIR="${ROOT_DIR}/boot/hart0/build/qemu-linux"
DTB_RAW="${WORK_DIR}/qemu-virt.dtb"
DTB_DTS="${WORK_DIR}/qemu-virt.dts"
DTB_PATCHED="${WORK_DIR}/qemu-virt-linux.dtb"
DTB_OVERLAY="${WORK_DIR}/qemu-virt-linux.overlay.dts"
INITRD_ADDR="${AMP_LINUX_INITRD_ADDR}"
DTB_ADDR="${AMP_LINUX_DTB_ADDR}"
FW_BRIDGE_ADDR="${AMP_OPENSBI_BRIDGE_ADDR}"
HART1_ADDR="${AMP_HART1_ADDR}"
HART2_ADDR="${AMP_HART2_ADDR}"
HART3_ADDR="${AMP_HART3_ADDR}"
LINUX_ADDR="${AMP_LINUX_ADDR}"
BOOTARGS="${AMP_BOOTARGS}"

for image in "${HART0}" "${APP1}" "${APP2}" "${APP3}" "${FW_JUMP}" "${LINUX_IMAGE}" "${LINUX_INITRD}"; do
    if [ ! -f "${image}" ]; then
        echo "[ERR] Missing direct M-mode artifact: ${image}"
        exit 1
    fi
done

mkdir -p "${WORK_DIR}"
rm -f "${DTB_RAW}" "${DTB_DTS}" "${DTB_PATCHED}" "${DTB_OVERLAY}"

qemu-system-riscv64 \
    -machine virt,dumpdtb="${DTB_RAW}" \
    -m 512 \
    -smp 5 \
    -display none \
    -serial none \
    -monitor none >/dev/null 2>&1

# QEMU's dumped DTB stores interrupt parents as numeric phandles; when dtc
# decompiles that blob back to DTS it emits a known false-positive warning for
# interrupts-extended because the original symbolic references are gone.
dtc -W no-interrupts_extended_property -I dtb -O dts -o "${DTB_DTS}" "${DTB_RAW}"

INITRD_SIZE="$(stat -c%s "${LINUX_INITRD}")"
python3 "${LAYOUT_TOOL}" patch-dts \
    --input "${DTB_DTS}" \
    --output "${DTB_DTS}" \
    --initrd-start "${INITRD_ADDR}" \
    --initrd-size "${INITRD_SIZE}" \
    --overlay-out "${DTB_OVERLAY}"

dtc -W no-interrupts_extended_property -I dts -O dtb -o "${DTB_PATCHED}" "${DTB_DTS}"

echo "[INFO] Starting QEMU direct M-mode AMP launch"
echo "[INFO] Hart0 firmware (M-mode): ${HART0}"
echo "[INFO] APP1 Zephyr raw preload @ ${HART1_ADDR}: ${APP1}"
echo "[INFO] APP2 FreeRTOS raw preload @ ${HART2_ADDR}: ${APP2}"
echo "[INFO] APP3 bare-metal raw preload @ ${HART3_ADDR}: ${APP3}"
echo "[INFO] Hart4 OpenSBI bridge @ 0x$(printf '%08x' "${FW_BRIDGE_ADDR}"): ${FW_JUMP}"
echo "[INFO] Linux kernel raw preload @ ${LINUX_ADDR}: ${LINUX_IMAGE}"
echo "[INFO] Linux initrd raw preload @ 0x$(printf '%08x' "${INITRD_ADDR}"): ${LINUX_INITRD}"
echo "[INFO] Linux DTB raw preload @ 0x$(printf '%08x' "${DTB_ADDR}"): ${DTB_PATCHED}"
echo "[INFO] Linux DT overlay fragment: ${DTB_OVERLAY}"

exec qemu-system-riscv64 \
    -machine virt \
    -bios none \
    -m 512 \
    -smp 5 \
    -nographic \
    -monitor none \
    -serial stdio \
    -device loader,file="${HART0}",cpu-num=0 \
    -device loader,file="${HART0}",cpu-num=1 \
    -device loader,file="${HART0}",cpu-num=2 \
    -device loader,file="${HART0}",cpu-num=3 \
    -device loader,file="${HART0}",cpu-num=4 \
    -device loader,file="${APP1}",addr=${HART1_ADDR},force-raw=on \
    -device loader,file="${APP2}",addr=${HART2_ADDR},force-raw=on \
    -device loader,file="${APP3}",addr=${HART3_ADDR},force-raw=on \
    -device loader,file="${FW_JUMP}",addr=${FW_BRIDGE_ADDR},force-raw=on \
    -device loader,file="${LINUX_IMAGE}",addr=${LINUX_ADDR},force-raw=on \
    -device loader,file="${LINUX_INITRD}",addr=${INITRD_ADDR},force-raw=on \
    -device loader,file="${DTB_PATCHED}",addr=${DTB_ADDR},force-raw=on
