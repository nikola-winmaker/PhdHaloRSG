#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "[ERR] Usage: $0 <target-dir>" >&2
    exit 1
fi

TARGET_DIR="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEFAULT_SRC="${SCRIPT_DIR}/../src/linux_app.c"
if [ -n "${ROOT_DIR:-}" ] && [ -f "${ROOT_DIR}/apps/linux-hart4/src/linux_app.c" ]; then
    DEFAULT_SRC="${ROOT_DIR}/apps/linux-hart4/src/linux_app.c"
fi
SRC="${AMP_HART4_APP_SRC:-${DEFAULT_SRC}}"
APP_DIR="$(cd "$(dirname "${SRC}")/.." && pwd)"
OUT="${TARGET_DIR}/usr/bin/linux_app"
USE_HALO="${USE_HALO:-0}"
DEPS_DIR="${APP_DIR}/deps/classical"

mkdir -p "${TARGET_DIR}/usr/bin"

if [ ! -f "${SRC}" ]; then
    echo "[ERR] Linux Hart 4 app source not found: ${SRC}" >&2
    exit 1
fi

if [ "${USE_HALO}" = "1" ]; then
    DEPS_DIR="${APP_DIR}/deps/halo/codegen/riscv64_h4_linux"
fi

DEPS_INC="${DEPS_DIR}/include"

if [ ! -d "${DEPS_INC}" ]; then
    echo "[ERR] Linux Hart 4 dependency include directory not found: ${DEPS_INC}" >&2
    echo "      USE_HALO=${USE_HALO}" >&2
    exit 1
fi

TARGET_GCC=""

for dir in \
    "${HOST_DIR}/bin" \
    "${HOST_DIR}/opt/ext-toolchain/bin"
do
    if [ -n "${TARGET_GCC}" ]; then
        break
    fi

    if [ ! -d "${dir}" ]; then
        continue
    fi

    TARGET_GCC="$(
        find "${dir}" -maxdepth 1 -type f \
            \( -name '*buildroot*linux*-gcc' \
            -o -name '*buildroot*linux*-gcc.br_real' \
            -o -name '*linux*-gcc' \
            -o -name '*linux*-gcc.br_real' \) \
            | sort | head -n 1
    )"
done

if [ -z "${TARGET_GCC}" ]; then
    echo "[ERR] Could not locate target GCC under ${HOST_DIR}" >&2
    exit 1
fi

"${TARGET_GCC}" -O2 -Wall -Wextra \
    -DUSE_HALO="${USE_HALO}" \
    -I"${APP_DIR}/src" \
    -I"${DEPS_INC}" \
    "${SRC}" \
    "${DEPS_DIR}"/src/*.c \
    -o "${OUT}"
chmod 0755 "${OUT}"

echo "[INFO] Installed Linux Hart 4 app from ${SRC}: ${OUT}"
echo "[INFO] Linux Hart 4 USE_HALO=${USE_HALO}"
echo "[INFO] Linux Hart 4 deps dir: ${DEPS_DIR}"
