#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/buildroot"
BUILDROOT_DIR="${WS_DIR}/buildroot"
OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"

# Ensure output directory exists with proper permissions (use sudo if needed)
if [ ! -d "${OUTPUT_DIR}" ]; then
    mkdir -p "${OUTPUT_DIR}"
    chmod 777 "${OUTPUT_DIR}"
elif [ ! -w "${OUTPUT_DIR}" ]; then
    # Directory exists but we can't write to it - use sudo to fix permissions
    sudo chmod 777 "${OUTPUT_DIR}"
fi

EXTERNAL_DIR="${WS_DIR}/external"
DEFCONFIG_NAME="risc5_eval_linux_amp_defconfig"
DEFCONFIG_SRC="${WS_DIR}/${DEFCONFIG_NAME}"
DEFCONFIG_DST="${BUILDROOT_DIR}/configs/${DEFCONFIG_NAME}"
APP_SRC="${ROOT_DIR}/apps/linux-hart0/src/amp_hart0_app.c"
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

if [ ! -d "${EXTERNAL_DIR}" ] || [ ! -f "${DEFCONFIG_SRC}" ]; then
    echo "[INFO] Buildroot scaffold is incomplete; seeding repo-local Buildroot assets"
    bash "${ROOT_DIR}/tools/scripts/setup_buildroot_deps.sh"
fi

if [ ! -d "${EXTERNAL_DIR}" ] || [ ! -f "${DEFCONFIG_SRC}" ]; then
    echo "[ERR] Missing Buildroot scaffold under ${WS_DIR}"
    echo "      Expected:"
    echo "        - ${EXTERNAL_DIR}"
    echo "        - ${DEFCONFIG_SRC}"
    exit 1
fi

mkdir -p "${BUILDROOT_DIR}/configs"
cp -f "${DEFCONFIG_SRC}" "${DEFCONFIG_DST}"

echo "[INFO] Using sanitized PATH for Buildroot:"
echo "       ${PATH}"
echo "[INFO] Configuring Buildroot output directory: ${OUTPUT_DIR}"
AMP_HART0_APP_SRC="${APP_SRC}" \
make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${OUTPUT_DIR}" "${DEFCONFIG_NAME}"

echo "[INFO] Building Linux AMP artifacts with Buildroot"
echo "[INFO] Using parallel jobs: ${JOBS}"
AMP_HART0_APP_SRC="${APP_SRC}" \
make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${OUTPUT_DIR}" -j"${JOBS}"

echo "[OK] Expected artifacts:"
echo "     ${OUTPUT_DIR}/images/Image"
echo "     ${OUTPUT_DIR}/images/rootfs.cpio"
