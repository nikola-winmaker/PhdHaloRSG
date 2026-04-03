#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
INSTALL_DEPS=0
APT_GET="${APT_GET:-apt-get}"
SUDO=""
PIPX_BIN_DIR="${HOME}/.local/bin"

usage() {
    cat <<'EOF'
Usage: ./tools/scripts/setup_dev_env.sh [--with-deps]

Installs the local development toolchain used by this repo on Debian/Ubuntu:
  - build-essential
  - cmake
  - ninja-build
  - git
  - qemu-system-misc
  - gcc-riscv64-unknown-elf
  - binutils-riscv64-unknown-elf
  - python3 / pip / venv / pipx
  - device-tree-compiler
  - cpio / unzip

Also ensures:
  - west is installed
  - pyelftools is available in the west environment

Options:
  --with-deps   Also initialize OpenSBI, Zephyr, FreeRTOS, and Buildroot repo dependencies
  -h, --help    Show this help text
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --with-deps)
            INSTALL_DEPS=1
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "[ERR] Unknown argument: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

if command -v sudo >/dev/null 2>&1 && [ "${EUID}" -ne 0 ]; then
    SUDO="sudo"
fi

if ! command -v "${APT_GET}" >/dev/null 2>&1; then
    echo "[ERR] ${APT_GET} not found. This script currently supports Debian/Ubuntu-style systems."
    exit 1
fi

echo "[INFO] Installing OS packages..."
${SUDO} "${APT_GET}" update
${SUDO} "${APT_GET}" install -y \
    binutils-riscv64-unknown-elf \
    binutils-riscv64-linux-gnu \
    build-essential \
    cmake \
    cpio \
    device-tree-compiler \
    g++-riscv64-linux-gnu \
    gcc-riscv64-unknown-elf \
    gcc-riscv64-linux-gnu \
    git \
    ninja-build \
    pipx \
    python3 \
    python3-pip \
    python3-venv \
    qemu-system-misc \
    unzip

if ! command -v pipx >/dev/null 2>&1; then
    echo "[ERR] pipx is still not available after package install"
    exit 1
fi

case ":${PATH}:" in
    *":${PIPX_BIN_DIR}:"*) ;;
    *) export PATH="${PIPX_BIN_DIR}:${PATH}" ;;
esac

echo "[INFO] Ensuring west is installed..."
if ! command -v west >/dev/null 2>&1; then
    pipx install west
    case ":${PATH}:" in
        *":${PIPX_BIN_DIR}:"*) ;;
        *) export PATH="${PIPX_BIN_DIR}:${PATH}" ;;
    esac
fi

echo "[INFO] Ensuring pyelftools is available for west..."
if ! pipx inject west pyelftools >/dev/null 2>&1; then
    echo "[INFO] pyelftools may already be present in the west environment"
fi

echo "[INFO] Tool versions:"
cmake --version | head -n 1
ninja --version
qemu-system-riscv64 --version | head -n 1
riscv64-unknown-elf-gcc --version | head -n 1
riscv64-linux-gnu-gcc --version | head -n 1
west --version

if [ "${INSTALL_DEPS}" -eq 1 ]; then
    echo "[INFO] Initializing project dependencies..."
    "${ROOT_DIR}/tools/scripts/setup_opensbi_deps.sh"
    "${ROOT_DIR}/tools/scripts/setup_zephyr_deps.sh"
    "${ROOT_DIR}/tools/scripts/setup_freertos_deps.sh"
    "${ROOT_DIR}/tools/scripts/setup_buildroot_deps.sh"
fi

echo "[OK] Development environment setup complete"
