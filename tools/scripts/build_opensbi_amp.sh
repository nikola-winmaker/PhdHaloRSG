#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
OPEN_SBI_DIR="${ROOT_DIR}/deps/opensbi"
LAYOUT_TOOL="${ROOT_DIR}/tools/scripts/amp_layout.py"

eval "$(python3 "${LAYOUT_TOOL}" shell-vars)"

CROSS_COMPILE="${CROSS_COMPILE:-riscv64-linux-gnu-}"
FW_MODE="${FW_MODE:-jump}"
LINUX_ADDR="${LINUX_ADDR:-${AMP_LINUX_ADDR}}"
LINUX_DTB_ADDR="${LINUX_DTB_ADDR:-${AMP_LINUX_DTB_ADDR}}"

echo "[INFO] Building OpenSBI AMP bridge"
echo "[INFO] OpenSBI tree: ${OPEN_SBI_DIR}"
echo "[INFO] CROSS_COMPILE=${CROSS_COMPILE}"
echo "[INFO] FW_MODE=${FW_MODE}"

case "${FW_MODE}" in
    jump)
        echo "[INFO] Clean rebuilding FW_JUMP for Linux handoff"
        echo "[INFO] FW_JUMP_ADDR=${LINUX_ADDR}"
        echo "[INFO] FW_JUMP_FDT_ADDR=${LINUX_DTB_ADDR}"
        make -C "${OPEN_SBI_DIR}" clean
        make -C "${OPEN_SBI_DIR}" \
            PLATFORM=generic \
            FW_JUMP=y \
            FW_JUMP_ADDR="${LINUX_ADDR}" \
            FW_JUMP_FDT_ADDR="${LINUX_DTB_ADDR}" \
            CROSS_COMPILE="${CROSS_COMPILE}"
        echo "[INFO] Output:"
        echo "       ${OPEN_SBI_DIR}/build/platform/generic/firmware/fw_jump.bin"
        echo "       ${OPEN_SBI_DIR}/build/platform/generic/firmware/fw_jump.elf"
        ;;
    dynamic)
        echo "[INFO] Building FW_DYNAMIC reference image"
        make -C "${OPEN_SBI_DIR}" clean
        make -C "${OPEN_SBI_DIR}" \
            PLATFORM=generic \
            FW_DYNAMIC=y \
            CROSS_COMPILE="${CROSS_COMPILE}"
        echo "[INFO] Output:"
        echo "       ${OPEN_SBI_DIR}/build/platform/generic/firmware/fw_dynamic.bin"
        echo "       ${OPEN_SBI_DIR}/build/platform/generic/firmware/fw_dynamic.elf"
        ;;
    *)
        echo "[ERR] Unsupported FW_MODE='${FW_MODE}'. Use 'jump' or 'dynamic'."
        exit 1
        ;;
esac
