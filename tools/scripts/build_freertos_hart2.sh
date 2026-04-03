#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
APP_DIR="${ROOT_DIR}/apps/freertos-hart2"
WS_DIR="${ROOT_DIR}/deps/freertos"
KERNEL_DIR="${WS_DIR}/FreeRTOS-Kernel"

if [ ! -d "${KERNEL_DIR}" ]; then
    echo "[ERR] FreeRTOS workspace not initialized at ${KERNEL_DIR}"
    echo "      Run: make freertos-setup"
    exit 1
fi

echo "[INFO] Using FreeRTOS workspace: ${KERNEL_DIR}"
if [ "${FREERTOS_HART2_MODE:-default}" = "smode" ]; then
    echo "[INFO] Building Hart2 in OpenSBI S-mode bring-up mode"
    make -C "${APP_DIR}" clean app2 FREERTOS_KERNEL_DIR="${KERNEL_DIR}" OPEN_SBI_SMODE=1
else
    make -C "${APP_DIR}" app2 FREERTOS_KERNEL_DIR="${KERNEL_DIR}"
fi
