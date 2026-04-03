#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
SMOKE_BUILDS=0
BUILDROOT_DEFCONFIG=0
FAILURES=0

usage() {
    cat <<'EOF'
Usage: ./tools/scripts/verify_toolchain.sh [options]

Verify required host tools, dependency workspaces, and optionally run smoke builds.

Options:
  --smoke-builds       Build Hart0, Zephyr Hart1, FreeRTOS Hart2, and OpenSBI FW_DYNAMIC
  --buildroot-defconfig  Validate Buildroot by generating the project defconfig in /tmp
  -h, --help           Show this help text
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --smoke-builds)
            SMOKE_BUILDS=1
            ;;
        --buildroot-defconfig)
            BUILDROOT_DEFCONFIG=1
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

note_ok() {
    echo "[OK] $1"
}

note_fail() {
    echo "[ERR] $1"
    FAILURES=$((FAILURES + 1))
}

check_cmd() {
    local tool="$1"
    local version_cmd="$2"

    if command -v "${tool}" >/dev/null 2>&1; then
        note_ok "${tool}: $(eval "${version_cmd}" 2>/dev/null | head -n 1)"
    else
        note_fail "Missing command: ${tool}"
    fi
}

check_file() {
    local path="$1"
    local label="$2"

    if [ -e "${path}" ]; then
        note_ok "${label}: ${path}"
    else
        note_fail "Missing ${label}: ${path}"
    fi
}

note_skip() {
    echo "[INFO] $1"
}

run_smoke() {
    local label="$1"
    shift

    echo "[INFO] Smoke test: ${label}"
    if "$@"; then
        note_ok "${label}"
    else
        note_fail "${label}"
    fi
}

has_buildroot_scaffold() {
    [ -d "${ROOT_DIR}/deps/buildroot/external" ] && [ -f "${ROOT_DIR}/deps/buildroot/risc5_eval_linux_amp_defconfig" ]
}

echo "[INFO] Verifying host tools"
check_cmd git "git --version"
check_cmd make "make --version"
check_cmd python3 "python3 --version"
check_cmd cmake "cmake --version"
check_cmd ninja "ninja --version"
check_cmd west "west --version"
check_cmd dtc "dtc --version"
check_cmd cpio "cpio --version"
check_cmd unzip "unzip -v"
check_cmd qemu-system-riscv64 "qemu-system-riscv64 --version"
check_cmd riscv64-unknown-elf-gcc "riscv64-unknown-elf-gcc --version"
check_cmd riscv64-unknown-elf-objcopy "riscv64-unknown-elf-objcopy --version"
check_cmd riscv64-unknown-elf-objdump "riscv64-unknown-elf-objdump --version"
check_cmd riscv64-linux-gnu-gcc "riscv64-linux-gnu-gcc --version"

echo "[INFO] Verifying Python helpers"
if python3 -c "import elftools" >/dev/null 2>&1; then
    note_ok "Python module: elftools"
elif find "${HOME}/.local/share/pipx/venvs/west" -maxdepth 4 -type d -name elftools >/dev/null 2>&1; then
    note_ok "Python module: elftools (via pipx west venv)"
else
    note_fail "Python module missing: elftools"
fi

echo "[INFO] Verifying dependency workspaces"
check_file "${ROOT_DIR}/deps/opensbi/README.md" "OpenSBI tree"
check_file "${ROOT_DIR}/deps/zephyr/.west/config" "Zephyr west workspace"
check_file "${ROOT_DIR}/deps/zephyr/zephyr/west.yml" "Zephyr source tree"
check_file "${ROOT_DIR}/deps/freertos/FreeRTOS-Kernel/README.md" "FreeRTOS kernel"
check_file "${ROOT_DIR}/deps/buildroot/buildroot/Makefile" "Buildroot tree"
if has_buildroot_scaffold; then
    note_ok "Buildroot external scaffold: ${ROOT_DIR}/deps/buildroot/external"
    note_ok "Buildroot defconfig: ${ROOT_DIR}/deps/buildroot/risc5_eval_linux_amp_defconfig"
else
    note_skip "Buildroot external scaffold is not present in this repo; skipping defconfig verification"
fi
check_file "${ROOT_DIR}/config/amp_layout.json" "AMP layout config"

if [ "${SMOKE_BUILDS}" -eq 1 ]; then
    echo "[INFO] Running smoke builds"
    run_smoke "Hart0 bare-metal build" make -C "${ROOT_DIR}" -f boot/hart0/Makefile clean hart0
    run_smoke "FreeRTOS Hart2 build" bash "${ROOT_DIR}/tools/scripts/build_freertos_hart2.sh"
    run_smoke "Zephyr Hart1 build" env ZEPHYR_HART1_TARGET=qemu ZEPHYR_PRISTINE=always bash "${ROOT_DIR}/tools/scripts/build_zephyr_hart1.sh"
    run_smoke "OpenSBI FW_DYNAMIC build" env FW_MODE=dynamic bash "${ROOT_DIR}/tools/scripts/build_opensbi_amp.sh"
fi

if [ "${BUILDROOT_DEFCONFIG}" -eq 1 ]; then
    BUILDROOT_DIR="${ROOT_DIR}/deps/buildroot/buildroot"
    EXTERNAL_DIR="${ROOT_DIR}/deps/buildroot/external"
    DEFCONFIG_NAME="risc5_eval_linux_amp_defconfig"
    DEFCONFIG_DST="${BUILDROOT_DIR}/configs/${DEFCONFIG_NAME}"
    VERIFY_OUT="/tmp/risc5_buildroot_verify"

    echo "[INFO] Validating Buildroot defconfig"
    if [ -d "${BUILDROOT_DIR}" ] && has_buildroot_scaffold; then
        cp -f "${ROOT_DIR}/deps/buildroot/${DEFCONFIG_NAME}" "${DEFCONFIG_DST}"
        rm -rf "${VERIFY_OUT}"
        if make -C "${BUILDROOT_DIR}" BR2_EXTERNAL="${EXTERNAL_DIR}" O="${VERIFY_OUT}" "${DEFCONFIG_NAME}"; then
            note_ok "Buildroot defconfig"
        else
            note_fail "Buildroot defconfig"
        fi
    else
        note_skip "Buildroot external scaffold is not present in this repo; skipping defconfig validation"
    fi
fi

if [ "${FAILURES}" -ne 0 ]; then
    echo "[ERR] Verification finished with ${FAILURES} failure(s)"
    exit 1
fi

echo "[OK] Verification finished successfully"
