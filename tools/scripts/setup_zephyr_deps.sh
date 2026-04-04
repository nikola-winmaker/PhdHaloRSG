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

install_zephyr_python_requirements() {
    local requirements_file="$1"

    if [ ! -f "${requirements_file}" ]; then
        echo "[ERR] Requirements file not found: ${requirements_file}"
        return 1
    fi

    if command -v pipx >/dev/null 2>&1 && command -v west >/dev/null 2>&1; then
        echo "[INFO] Installing Zephyr Python requirements into west via pipx inject..."
        # Use pipx inject to add requirements to the west environment
        pipx inject west -r "${requirements_file}" 2>/dev/null || true
    else
        echo "[ERR] pipx and west are required to install Zephyr Python requirements"
        exit 1
    fi
}

mkdir -p "${WS_DIR}"

if [ ! -d "${WS_DIR}/.west" ]; then
    echo "[INFO] Initializing west workspace in ${WS_DIR} (Zephyr ${ZEPHYR_REF})"
    west init -m https://github.com/zephyrproject-rtos/zephyr --mr "${ZEPHYR_REF}" "${WS_DIR}"
fi

cd "${WS_DIR}"

cd "${WS_DIR}"

# Use lean RISC-V-only west.yml from app (BEFORE west update)
if [ -f "${ROOT_DIR}/apps/zephyr-hart1/zephyr/west.yml" ]; then
    echo "[INFO] Using minimal RISC-V-focused west.yml from app"
    cp "${ROOT_DIR}/apps/zephyr-hart1/zephyr/west.yml" zephyr/west.yml
fi

echo "[INFO] Updating Zephyr modules..."
west update
west zephyr-export

install_zephyr_python_requirements zephyr/scripts/requirements.txt

echo "[OK] Zephyr workspace ready: ${WS_DIR}"
