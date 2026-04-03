#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/zephyr"
APP_DIR="${ROOT_DIR}/apps/zephyr-hart1/zephyr"
OUT_DIR="${ROOT_DIR}/apps/zephyr-hart1/build/bin"
LAYOUT_TOOL="${ROOT_DIR}/tools/scripts/amp_layout.py"
PRISTINE_MODE="${ZEPHYR_PRISTINE:-always}"
TARGET_MODE="${ZEPHYR_HART1_TARGET:-qemu}"
GENERATOR="${ZEPHYR_CMAKE_GENERATOR:-Ninja}"

case "${TARGET_MODE}" in
    qemu)
        BOARD="qemu_riscv64"
        ;;
    hw|real|real-hw|hifive_unmatched)
        BOARD="hifive_unmatched"
        ;;
    *)
        echo "[ERR] Unknown Zephyr Hart1 target: ${TARGET_MODE}"
        echo "      Use one of: qemu, hw"
        exit 1
        ;;
esac

BUILD_DIR="${APP_DIR}/build-${TARGET_MODE}"
OVERLAY_DIR="${ROOT_DIR}/apps/zephyr-hart1/build/generated"
OVERLAY_FILE="${OVERLAY_DIR}/${BOARD}.overlay"
SYSROOT_CMAKE_ARG=()

if [ -z "${ZEPHYR_TOOLCHAIN_VARIANT:-}" ]; then
    if command -v riscv64-unknown-elf-gcc >/dev/null 2>&1; then
        export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
        export CROSS_COMPILE="$(dirname "$(command -v riscv64-unknown-elf-gcc)")/riscv64-unknown-elf-"
        echo "[INFO] Using Zephyr cross-compile toolchain: ${CROSS_COMPILE}"
    fi
fi

if [ "${ZEPHYR_TOOLCHAIN_VARIANT:-}" = "cross-compile" ] && [ -z "${SYSROOT_DIR:-}" ]; then
    if [ -d "/usr/riscv64-linux-gnu" ]; then
        export SYSROOT_DIR="/usr/riscv64-linux-gnu"
        echo "[INFO] Using Zephyr sysroot: ${SYSROOT_DIR}"
    fi
fi

if [ -n "${SYSROOT_DIR:-}" ]; then
    SYSROOT_CMAKE_ARG=(-DSYSROOT_DIR="${SYSROOT_DIR}")
fi

OBJCOPY="${CROSS_COMPILE:-}objcopy"

if ! command -v west >/dev/null 2>&1; then
    echo "[ERR] west not found. Run: make zephyr-setup"
    exit 1
fi

if [ ! -d "${WS_DIR}/.west" ]; then
    echo "[ERR] Zephyr workspace not initialized at ${WS_DIR}"
    echo "      Run: make zephyr-setup"
    exit 1
fi

cd "${WS_DIR}"

if command -v ninja >/dev/null 2>&1; then
    export PATH="/usr/bin:/bin:${PATH}"
fi

CURRENT_WEST_GENERATOR="$(west config build.generator 2>/dev/null || true)"
if [ "${CURRENT_WEST_GENERATOR}" != "${GENERATOR}" ]; then
    west config --local build.generator "${GENERATOR}"
fi

if [ "${PRISTINE_MODE}" = "never" ] && [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    echo "[INFO] Zephyr build directory is not initialized; switching to pristine configure"
    PRISTINE_MODE="always"
fi

if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    CACHE_GENERATOR="$(grep '^CMAKE_GENERATOR:INTERNAL=' "${BUILD_DIR}/CMakeCache.txt" | sed 's/^CMAKE_GENERATOR:INTERNAL=//')"

    if [ -n "${CACHE_GENERATOR}" ] && [ "${CACHE_GENERATOR}" != "${GENERATOR}" ]; then
        echo "[INFO] Zephyr build generator changed (${CACHE_GENERATOR} -> ${GENERATOR}); switching to pristine configure"
        PRISTINE_MODE="always"
    fi
fi

echo "[INFO] Building Zephyr Hart1 target: ${BOARD} (${TARGET_MODE})"
echo "[INFO] Using CMake generator: ${GENERATOR}"
echo "[INFO] Using build directory: ${BUILD_DIR}"
mkdir -p "${OVERLAY_DIR}"

if [ "${TARGET_MODE}" = "qemu" ]; then
    python3 "${LAYOUT_TOOL}" zephyr-overlay --target qemu > "${OVERLAY_FILE}"
else
    python3 "${LAYOUT_TOOL}" zephyr-overlay --target hw > "${OVERLAY_FILE}"
fi

west build -p "${PRISTINE_MODE}" -b "${BOARD}" "${APP_DIR}" -d "${BUILD_DIR}" -- -DDTC_OVERLAY_FILE="${OVERLAY_FILE}" "${SYSROOT_CMAKE_ARG[@]}"

mkdir -p "${OUT_DIR}"
cp -f "${BUILD_DIR}/zephyr/zephyr.elf" "${OUT_DIR}/app1.elf"

if [ -f "${BUILD_DIR}/zephyr/zephyr.bin" ]; then
    cp -f "${BUILD_DIR}/zephyr/zephyr.bin" "${OUT_DIR}/app1.bin"
elif command -v "${OBJCOPY}" >/dev/null 2>&1; then
    "${OBJCOPY}" -O binary "${BUILD_DIR}/zephyr/zephyr.elf" "${OUT_DIR}/app1.bin"
else
    echo "[ERR] Could not find zephyr.bin and ${OBJCOPY} is unavailable to create app1.bin"
    exit 1
fi

echo "[OK] Zephyr Hart1 artifacts:"
echo "     ${BUILD_DIR}/zephyr/zephyr.elf"
if [ -f "${BUILD_DIR}/zephyr/zephyr.bin" ]; then
    echo "     ${BUILD_DIR}/zephyr/zephyr.bin"
else
    echo "     ${OUT_DIR}/app1.bin generated from zephyr.elf"
fi
echo "[OK] Loader-compatible app1 artifacts:"
echo "     ${OUT_DIR}/app1.bin"
echo "     ${OUT_DIR}/app1.elf"
