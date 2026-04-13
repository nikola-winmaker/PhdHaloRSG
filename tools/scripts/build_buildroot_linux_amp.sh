#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/buildroot"
BUILDROOT_DIR="${WS_DIR}/buildroot"
OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"
ARTIFACT_DIR="${ROOT_DIR}/artifacts/buildroot/images"

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
APP_SRC="${ROOT_DIR}/apps/linux-hart4/src/linux_app.c"
APP_BUILDROOT_DIR="${ROOT_DIR}/apps/linux-hart4/buildroot"
BOARD_DIR="${EXTERNAL_DIR}/board/risc5_eval"
JOBS="${BUILDROOT_JOBS:-$(nproc)}"
USE_HALO="${USE_HALO:-0}"

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
mkdir -p "${BOARD_DIR}"
install -m 0755 "${APP_BUILDROOT_DIR}/post-build.sh" "${BOARD_DIR}/post-build.sh"

echo "[INFO] Using sanitized PATH for Buildroot:"
echo "       ${PATH}"
echo "[INFO] Configuring Buildroot output directory: ${OUTPUT_DIR}"
echo "[INFO] Linux Hart4 USE_HALO=${USE_HALO}"
AMP_HART4_APP_SRC="${APP_SRC}" \
USE_HALO="${USE_HALO}" \
make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${OUTPUT_DIR}" "${DEFCONFIG_NAME}"

echo "[INFO] Building Linux AMP artifacts with Buildroot"
echo "[INFO] Using parallel jobs: ${JOBS}"
AMP_HART4_APP_SRC="${APP_SRC}" \
USE_HALO="${USE_HALO}" \
make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${OUTPUT_DIR}" -j"${JOBS}"

mkdir -p "${ARTIFACT_DIR}"
cp "${OUTPUT_DIR}/images/Image" "${ARTIFACT_DIR}/Image"
cp "${OUTPUT_DIR}/images/rootfs.cpio" "${ARTIFACT_DIR}/rootfs.cpio"
cp "${OUTPUT_DIR}/images/rootfs.cpio" "${ARTIFACT_DIR}/rootfs.base.cpio"

echo "[INFO] Updated tracked artifacts:"
echo "     ${ARTIFACT_DIR}/Image"
echo "     ${ARTIFACT_DIR}/rootfs.cpio"
echo "     ${ARTIFACT_DIR}/rootfs.base.cpio"


echo "[OK] Expected artifacts:"
echo "     ${OUTPUT_DIR}/images/Image"
echo "     ${OUTPUT_DIR}/images/rootfs.cpio"
