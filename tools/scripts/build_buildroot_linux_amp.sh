#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/buildroot"
BUILDROOT_DIR="${WS_DIR}/buildroot"
OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"
EXTERNAL_DIR="${WS_DIR}/external"
DEFCONFIG_NAME="risc5_eval_linux_amp_defconfig"
DEFCONFIG_SRC="${WS_DIR}/${DEFCONFIG_NAME}"
DEFCONFIG_DST="${BUILDROOT_DIR}/configs/${DEFCONFIG_NAME}"
JOBS="${BUILDROOT_JOBS:-$(nproc)}"

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

for tool in cpio unzip; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "[ERR] Missing host tool: ${tool}"
        echo "      Install it with: sudo apt-get install -y cpio unzip"
        echo "      Or rerun: make dev-env"
        exit 1
    fi
done

if [ ! -f "${DEFCONFIG_SRC}" ]; then
    echo "[ERR] Missing Buildroot defconfig scaffold: ${DEFCONFIG_SRC}"
    exit 1
fi

mkdir -p "${BUILDROOT_DIR}/configs"
cp -f "${DEFCONFIG_SRC}" "${DEFCONFIG_DST}"

echo "[INFO] Using sanitized PATH for Buildroot:"
echo "       ${PATH}"
echo "[INFO] Configuring Buildroot output directory: ${OUTPUT_DIR}"
make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${OUTPUT_DIR}" "${DEFCONFIG_NAME}"

echo "[INFO] Building Linux AMP artifacts with Buildroot"
echo "[INFO] Using parallel jobs: ${JOBS}"
make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${OUTPUT_DIR}" -j"${JOBS}"

echo "[OK] Expected artifacts:"
echo "     ${OUTPUT_DIR}/images/Image"
echo "     ${OUTPUT_DIR}/images/rootfs.cpio"
