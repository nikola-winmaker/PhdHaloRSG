#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/buildroot"
BUILDROOT_DIR="${WS_DIR}/buildroot"
OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"

if [ ! -d "${OUTPUT_DIR}" ]; then
    mkdir -p "${OUTPUT_DIR}"
    chmod 777 "${OUTPUT_DIR}"
elif [ ! -w "${OUTPUT_DIR}" ]; then
    sudo chmod 777 "${OUTPUT_DIR}"
fi

EXTERNAL_DIR="${WS_DIR}/external"
POST_BUILD_SCRIPT="${EXTERNAL_DIR}/board/risc5_eval/post-build.sh"
APP_SRC="${ROOT_DIR}/apps/linux-hart4/src/linux_app.c"
APP_BUILDROOT_DIR="${ROOT_DIR}/apps/linux-hart4/buildroot"
DEPS_DIR="${ROOT_DIR}/apps/linux-hart4/deps/classical"
TARGET_DIR="${OUTPUT_DIR}/target"
HOST_DIR="${OUTPUT_DIR}/host"
ARTIFACT_DIR="${ROOT_DIR}/artifacts/buildroot/images"
ARTIFACT_ROOTFS="${ARTIFACT_DIR}/rootfs.cpio"
ARTIFACT_ROOTFS_BASE="${ARTIFACT_DIR}/rootfs.base.cpio"
INIT_SCRIPT_SOURCE="${EXTERNAL_DIR}/board/risc5_eval/rootfs-overlay/etc/init.d/S90amp-hart0-app"
USE_HALO="${USE_HALO:-0}"

if [ "${USE_HALO}" = "1" ]; then
    DEPS_DIR="${ROOT_DIR}/apps/linux-hart4/deps/halo/codegen/riscv64_h4_linux"
fi

DEPS_INC="${DEPS_DIR}/include"

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
    if [ ! -f "${ARTIFACT_ROOTFS}" ] && [ ! -f "${ARTIFACT_ROOTFS_BASE}" ]; then
        echo "[ERR] Buildroot output is not configured at ${OUTPUT_DIR}"
        echo "      No tracked rootfs artifact found at ${ARTIFACT_ROOTFS}"
        exit 1
    fi

    if ! command -v riscv64-linux-gnu-gcc >/dev/null 2>&1; then
        echo "[ERR] Missing host tool: riscv64-linux-gnu-gcc"
        echo "      Install it with: sudo apt-get install -y gcc-riscv64-linux-gnu"
        echo "      Or rerun: make dev-env"
        exit 1
    fi

    if [ ! -d "${DEPS_INC}" ]; then
        echo "[ERR] Linux Hart 4 dependency include directory not found: ${DEPS_INC}"
        echo "      USE_HALO=${USE_HALO}"
        exit 1
    fi

    if [ ! -f "${INIT_SCRIPT_SOURCE}" ]; then
        echo "[ERR] Init script overlay not found: ${INIT_SCRIPT_SOURCE}"
        exit 1
    fi

    TMP_DIR="$(mktemp -d)"
    trap 'rm -rf "${TMP_DIR}"' EXIT

    OVERLAY_DIR="${TMP_DIR}/overlay"
    OVERLAY_CPIO="${TMP_DIR}/rootfs-overlay.cpio"
    mkdir -p "${OVERLAY_DIR}/usr/bin"
    mkdir -p "${OVERLAY_DIR}/etc/init.d"

    echo "[INFO] Buildroot output is not configured at ${OUTPUT_DIR}"
    echo "[INFO] Rebuilding Linux app directly against the tracked rootfs artifact"

    riscv64-linux-gnu-gcc \
        -O2 -Wall -Wextra \
        -DUSE_HALO="${USE_HALO}" \
        -I"${DEPS_INC}" \
        -Wl,--dynamic-linker=/lib/ld-linux-riscv64-lp64d.so.1 \
        "${APP_SRC}" \
        "${DEPS_DIR}"/src/*.c \
        -o "${OVERLAY_DIR}/usr/bin/linux_app"
    chmod 0755 "${OVERLAY_DIR}/usr/bin/linux_app"

    cp "${INIT_SCRIPT_SOURCE}" "${OVERLAY_DIR}/etc/init.d/S90amp-hart0-app"
    chmod 0755 "${OVERLAY_DIR}/etc/init.d/S90amp-hart0-app"

    (
        cd "${OVERLAY_DIR}"
        find . -mindepth 1 -printf '%P\n' | LC_ALL=C sort | cpio -o -H newc --quiet > "${OVERLAY_CPIO}"
    )

    mkdir -p "${ARTIFACT_DIR}"

    if [ -f "${ARTIFACT_ROOTFS_BASE}" ]; then
        BASE_ROOTFS="${ARTIFACT_ROOTFS_BASE}"
    else
        BASE_ROOTFS="${ARTIFACT_ROOTFS}"
        cp "${ARTIFACT_ROOTFS}" "${ARTIFACT_ROOTFS_BASE}"
        echo "[INFO] Saved base rootfs artifact: ${ARTIFACT_ROOTFS_BASE}"
    fi

    cat "${BASE_ROOTFS}" "${OVERLAY_CPIO}" > "${ARTIFACT_ROOTFS}.tmp"
    mv "${ARTIFACT_ROOTFS}.tmp" "${ARTIFACT_ROOTFS}"

    echo "[INFO] Updated artifact: ${ARTIFACT_ROOTFS}"
    echo "[OK] Expected artifacts:"
    echo "     ${ARTIFACT_ROOTFS}"
    echo "     ${OVERLAY_DIR}/usr/bin/linux_app"
    echo "     ${OVERLAY_DIR}/etc/init.d/S90amp-hart0-app"
    exit 0
fi

if [ ! -d "${TARGET_DIR}" ] || [ ! -d "${HOST_DIR}" ]; then
    echo "[ERR] Buildroot output is incomplete at ${OUTPUT_DIR}"
    echo "      Run: make buildroot-linux"
    exit 1
fi

if [ ! -x "${POST_BUILD_SCRIPT}" ]; then
    echo "[ERR] Missing Linux app post-build script: ${POST_BUILD_SCRIPT}"
    echo "      Run: make buildroot-setup"
    exit 1
fi

install -m 0755 "${APP_BUILDROOT_DIR}/post-build.sh" "${POST_BUILD_SCRIPT}"

echo "[INFO] Fast Linux app/rootfs rebuild using existing Buildroot output"
echo "[INFO] Buildroot output directory: ${OUTPUT_DIR}"
echo "[INFO] Linux Hart4 USE_HALO=${USE_HALO}"

echo "[INFO] Rebuilding userspace app into target rootfs"
HOST_DIR="${HOST_DIR}" \
AMP_HART4_APP_SRC="${APP_SRC}" \
USE_HALO="${USE_HALO}" \
"${POST_BUILD_SCRIPT}" "${TARGET_DIR}"

echo "[INFO] Repacking rootfs.cpio with Buildroot (preserves device files)"
AMP_HART4_APP_SRC="${APP_SRC}" \
USE_HALO="${USE_HALO}" \
make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${OUTPUT_DIR}" rootfs-cpio

mkdir -p "${ARTIFACT_DIR}"
cp "${OUTPUT_DIR}/images/rootfs.cpio" "${ARTIFACT_DIR}/rootfs.cpio"
cp "${OUTPUT_DIR}/images/rootfs.cpio" "${ARTIFACT_DIR}/rootfs.base.cpio"

echo "[INFO] Updated artifact: ${ARTIFACT_DIR}/rootfs.cpio"
echo "[INFO] Updated base artifact: ${ARTIFACT_DIR}/rootfs.base.cpio"

echo "[OK] Expected artifacts:"
echo "     ${OUTPUT_DIR}/images/rootfs.cpio"
echo "     ${OUTPUT_DIR}/target/usr/bin/linux_app"