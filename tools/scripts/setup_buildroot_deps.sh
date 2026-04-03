#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/buildroot"
BUILDROOT_DIR="${WS_DIR}/buildroot"
BUILDROOT_REF="${BUILDROOT_REF:-2024.02.9}"

mkdir -p "${WS_DIR}"

if [ ! -d "${BUILDROOT_DIR}/.git" ]; then
    echo "[INFO] Cloning Buildroot into ${BUILDROOT_DIR} (${BUILDROOT_REF})"
    git clone --depth 1 --branch "${BUILDROOT_REF}" https://github.com/buildroot/buildroot.git "${BUILDROOT_DIR}"
else
    echo "[INFO] Updating Buildroot in ${BUILDROOT_DIR}"
    git -C "${BUILDROOT_DIR}" fetch --depth 1 origin "${BUILDROOT_REF}"
    git -C "${BUILDROOT_DIR}" checkout "${BUILDROOT_REF}"
    git -C "${BUILDROOT_DIR}" pull --ff-only origin "${BUILDROOT_REF}"
fi

echo "[OK] Buildroot workspace ready: ${WS_DIR}"
echo "[INFO] Project defconfig scaffold: ${WS_DIR}/risc5_eval_linux_amp_defconfig"
