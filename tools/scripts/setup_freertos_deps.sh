#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/freertos"
KERNEL_DIR="${WS_DIR}/FreeRTOS-Kernel"
FREERTOS_REF="${FREERTOS_REF:-main}"

mkdir -p "${WS_DIR}"

if [ ! -d "${KERNEL_DIR}/.git" ]; then
    echo "[INFO] Cloning FreeRTOS-Kernel into ${KERNEL_DIR} (${FREERTOS_REF})"
    git clone --depth 1 --branch "${FREERTOS_REF}" https://github.com/FreeRTOS/FreeRTOS-Kernel.git "${KERNEL_DIR}"
else
    echo "[INFO] Updating FreeRTOS-Kernel in ${KERNEL_DIR}"
    git -C "${KERNEL_DIR}" fetch --depth 1 origin "${FREERTOS_REF}"
    git -C "${KERNEL_DIR}" checkout "${FREERTOS_REF}"
    git -C "${KERNEL_DIR}" pull --ff-only origin "${FREERTOS_REF}"
fi

echo "[OK] FreeRTOS workspace ready: ${WS_DIR}"
