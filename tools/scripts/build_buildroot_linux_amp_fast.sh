#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/buildroot"
BUILDROOT_DIR="${WS_DIR}/buildroot"
OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"
EXTERNAL_DIR="${WS_DIR}/external"
POST_BUILD_SCRIPT="${ROOT_DIR}/apps/linux-hart0/buildroot/post-build.sh"
TARGET_DIR="${OUTPUT_DIR}/target"
HOST_DIR="${OUTPUT_DIR}/host"

SAFE_PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
if [ -d "${HOME}/.local/bin" ]; then
    SAFE_PATH="${HOME}/.local/bin:${SAFE_PATH}"
fi
export PATH="${SAFE_PATH}"

if [ ! -d "${BUILDROOT_DIR}" ]; then
    echo "[ERR] Buildroot workspace not initialized at ${BUILDROOT_DIR}"
    echo "      Run: make buildroot-setup"
    exit 1
fi

if [ ! -f "${OUTPUT_DIR}/.config" ]; then
    echo "[ERR] Buildroot output is not configured at ${OUTPUT_DIR}"
    echo "      Run: make buildroot-linux"
    exit 1
fi

if [ ! -d "${TARGET_DIR}" ] || [ ! -d "${HOST_DIR}" ]; then
    echo "[ERR] Buildroot output is incomplete at ${OUTPUT_DIR}"
    echo "      Run: make buildroot-linux"
    exit 1
fi

if [ ! -x "${POST_BUILD_SCRIPT}" ]; then
    echo "[ERR] Missing Linux app post-build script: ${POST_BUILD_SCRIPT}"
    exit 1
fi

echo "[INFO] Fast Linux app/rootfs rebuild using existing Buildroot output"
echo "[INFO] Buildroot output directory: ${OUTPUT_DIR}"
echo "[INFO] Rebuilding userspace app into target rootfs"

HOST_DIR="${HOST_DIR}" "${POST_BUILD_SCRIPT}" "${TARGET_DIR}"

echo "[INFO] Repacking rootfs.cpio without rebuilding the kernel"
make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${OUTPUT_DIR}" rootfs-cpio

echo "[OK] Expected artifacts:"
echo "     ${OUTPUT_DIR}/images/rootfs.cpio"
echo "     ${OUTPUT_DIR}/target/usr/bin/amp-hart0-app"
