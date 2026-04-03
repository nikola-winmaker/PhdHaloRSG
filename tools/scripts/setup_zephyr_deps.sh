#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/zephyr"
ZEPHYR_REF="${ZEPHYR_REF:-v3.7.0}"
PIPX_BIN_DIR="${HOME}/.local/bin"

if [ -x "${PIPX_BIN_DIR}/west" ]; then
    case ":${PATH}:" in
        *":${PIPX_BIN_DIR}:"*) ;;
        *) export PATH="${PIPX_BIN_DIR}:${PATH}" ;;
    esac
fi

if ! command -v west >/dev/null 2>&1; then
    echo "[ERR] west not found. Install with: pip3 install --user west"
    exit 1
fi

mkdir -p "${WS_DIR}"

if [ ! -d "${WS_DIR}/.west" ]; then
    echo "[INFO] Initializing west workspace in ${WS_DIR} (Zephyr ${ZEPHYR_REF})"
    west init -m https://github.com/zephyrproject-rtos/zephyr --mr "${ZEPHYR_REF}" "${WS_DIR}"
fi

cd "${WS_DIR}"
echo "[INFO] Updating Zephyr modules..."
west update
west zephyr-export

if command -v python3 >/dev/null 2>&1; then
    echo "[INFO] Installing Zephyr Python requirements..."
    python3 -m pip install -r zephyr/scripts/requirements.txt
fi

echo "[OK] Zephyr workspace ready: ${WS_DIR}"
