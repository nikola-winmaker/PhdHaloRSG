#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
RUN_HOST_SETUP=1
RUN_VERIFY=1
VERIFY_FULL=0

usage() {
    cat <<'EOF'
Usage: ./tools/scripts/setup_all.sh [options]

Install the host toolchain, initialize all repo dependencies, and verify the setup.

Options:
  --skip-host-tools   Do not install OS packages / pipx tools
  --skip-verify       Do not run verification at the end
  --verify-full       Run verification with smoke builds enabled
  -h, --help          Show this help text

Environment:
  APT_GET             Override the package manager command used by setup_dev_env.sh
  ZEPHYR_REF          Override the Zephyr ref used by setup_zephyr_deps.sh
  FREERTOS_REF        Override the FreeRTOS ref used by setup_freertos_deps.sh
  BUILDROOT_REF       Override the Buildroot ref used by setup_buildroot_deps.sh
  OPENSBI_REF         Override the OpenSBI ref used by setup_opensbi_deps.sh
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --skip-host-tools)
            RUN_HOST_SETUP=0
            ;;
        --skip-verify)
            RUN_VERIFY=0
            ;;
        --verify-full)
            VERIFY_FULL=1
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

if [ "${RUN_HOST_SETUP}" -eq 1 ]; then
    echo "[STEP] Installing host development tools"
    "${ROOT_DIR}/tools/scripts/setup_dev_env.sh"
fi

echo "[STEP] Preparing OpenSBI"
"${ROOT_DIR}/tools/scripts/setup_opensbi_deps.sh"

echo "[STEP] Preparing Zephyr workspace"
"${ROOT_DIR}/tools/scripts/setup_zephyr_deps.sh"

echo "[STEP] Preparing FreeRTOS workspace"
"${ROOT_DIR}/tools/scripts/setup_freertos_deps.sh"

echo "[STEP] Preparing Buildroot workspace"
"${ROOT_DIR}/tools/scripts/setup_buildroot_deps.sh"

if [ "${RUN_VERIFY}" -eq 1 ]; then
    echo "[STEP] Verifying toolchain and dependencies"
    if [ "${VERIFY_FULL}" -eq 1 ]; then
        "${ROOT_DIR}/tools/scripts/verify_toolchain.sh" --smoke-builds --buildroot-defconfig
    else
        "${ROOT_DIR}/tools/scripts/verify_toolchain.sh"
    fi
fi

echo "[OK] Repository setup complete"
