#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "[ERR] Usage: $0 <target-dir>" >&2
    exit 1
fi

TARGET_DIR="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DEFAULT_SRC="${APP_DIR}/src/linux_app.c"
if [ -n "${ROOT_DIR:-}" ] && [ -f "${ROOT_DIR}/apps/linux-hart4/src/linux_app.c" ]; then
    DEFAULT_SRC="${ROOT_DIR}/apps/linux-hart4/src/linux_app.c"
fi
SRC="${AMP_HART4_APP_SRC:-${DEFAULT_SRC}}"
OUT="${TARGET_DIR}/usr/bin/linux_app"

mkdir -p "${TARGET_DIR}/usr/bin"

if [ ! -f "${SRC}" ]; then
    echo "[ERR] Linux Hart 4 app source not found: ${SRC}" >&2
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

"${TARGET_GCC}" -O2 -Wall -Wextra "${SRC}" -o "${OUT}"
chmod 0755 "${OUT}"

echo "[INFO] Installed Linux Hart 4 app from ${SRC}: ${OUT}"
