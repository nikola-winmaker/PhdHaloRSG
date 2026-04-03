param(
    [string]$Distro = "Ubuntu",
    [string]$RepoWindowsPath = "",
    [switch]$SkipVerify,
    [switch]$SkipSmokeBuilds,
    [switch]$SkipBuildrootDefconfig
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[STEP] $Message" -ForegroundColor Cyan
}

function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Green
}

function Require-Admin {
    $currentIdentity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentIdentity)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw "Run this PowerShell script as Administrator."
    }
}

function Convert-ToWslPath {
    param([Parameter(Mandatory = $true)][string]$WindowsPath)

    $resolved = (Resolve-Path -LiteralPath $WindowsPath).Path
    $drive = $resolved.Substring(0, 1).ToLowerInvariant()
    $rest = $resolved.Substring(2).Replace("\", "/")
    return "/mnt/$drive$rest"
}

function Invoke-WslBash {
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [string]$TargetDistro = $Distro
    )

    & wsl.exe -d $TargetDistro -- bash -lc $Command
    if ($LASTEXITCODE -ne 0) {
        throw "WSL command failed: $Command"
    }
}

Require-Admin

if ([string]::IsNullOrWhiteSpace($RepoWindowsPath)) {
    $RepoWindowsPath = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

$repoWslPath = Convert-ToWslPath -WindowsPath $RepoWindowsPath
$bootstrapPath = "$repoWslPath/tools/scripts/.bootstrap_env_embedded.sh"

$embeddedBootstrap = @'
#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${1:?missing repo root}"
APT_GET="${APT_GET:-apt-get}"
SUDO=""

RUN_VERIFY=1
RUN_SMOKE_BUILDS=1
RUN_BUILDROOT_DEFCONFIG=1

ZEPHYR_REF="${ZEPHYR_REF:-v3.7.0}"
FREERTOS_REF="${FREERTOS_REF:-main}"
BUILDROOT_REF="${BUILDROOT_REF:-2024.02.9}"
OPENSBI_REF="${OPENSBI_REF:-}"
OPENSBI_REMOTE="${OPENSBI_REMOTE:-https://github.com/riscv-software-src/opensbi.git}"

ZEPHYR_WS_DIR="${ROOT_DIR}/deps/zephyr"
FREERTOS_WS_DIR="${ROOT_DIR}/deps/freertos"
BUILDROOT_WS_DIR="${ROOT_DIR}/deps/buildroot"
OPENSBI_DIR="${ROOT_DIR}/deps/opensbi"

FAILURES=0

if command -v sudo >/dev/null 2>&1 && [ "${EUID}" -ne 0 ]; then
    SUDO="sudo"
fi

step() { echo "[STEP] $1"; }
info() { echo "[INFO] $1"; }
ok() { echo "[OK] $1"; }
err() { echo "[ERR] $1"; }
note_ok() { echo "[OK] $1"; }
note_fail() { echo "[ERR] $1"; FAILURES=$((FAILURES + 1)); }

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

run_smoke() {
    local label="$1"
    shift
    info "Smoke test: ${label}"
    if "$@"; then
        note_ok "${label}"
    else
        note_fail "${label}"
    fi
}

gitlink_commit() {
    if git -C "${ROOT_DIR}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        git -C "${ROOT_DIR}" ls-tree HEAD deps/opensbi 2>/dev/null | awk '{print $3}'
    fi
}

install_host_tools() {
    if ! command -v "${APT_GET}" >/dev/null 2>&1; then
        err "${APT_GET} not found. This bootstrap currently supports Debian/Ubuntu-style systems."
        exit 1
    fi

    step "Installing host development packages inside WSL Ubuntu"
    ${SUDO} "${APT_GET}" update
    ${SUDO} "${APT_GET}" install -y \
        binutils-riscv64-unknown-elf \
        binutils-riscv64-linux-gnu \
        build-essential \
        cmake \
        cpio \
        device-tree-compiler \
        g++-riscv64-linux-gnu \
        gcc-riscv64-linux-gnu \
        gcc-riscv64-unknown-elf \
        git \
        ninja-build \
        pipx \
        python3 \
        python3-pip \
        python3-venv \
        qemu-system-misc \
        unzip

    if ! command -v pipx >/dev/null 2>&1; then
        err "pipx is still unavailable after package install"
        exit 1
    fi

    info "Ensuring west is installed"
    if ! command -v west >/dev/null 2>&1; then
        pipx install west
    fi

    info "Ensuring pyelftools is available in the west environment"
    if ! pipx inject west pyelftools >/dev/null 2>&1; then
        info "pyelftools may already be present in the west environment"
    fi

    ok "Host tools installed"
}

setup_opensbi() {
    if [ -z "${OPENSBI_REF}" ]; then
        OPENSBI_REF="$(gitlink_commit || true)"
    fi

    if [ -d "${OPENSBI_DIR}/.git" ]; then
        info "OpenSBI repository already present at ${OPENSBI_DIR}"
        if [ -n "${OPENSBI_REF}" ]; then
            info "Checking out pinned OpenSBI ref: ${OPENSBI_REF}"
            git -C "${OPENSBI_DIR}" fetch --tags origin
            git -C "${OPENSBI_DIR}" checkout "${OPENSBI_REF}"
        fi
    elif [ -f "${OPENSBI_DIR}/README.md" ] && [ -f "${OPENSBI_DIR}/include/sbi/sbi_types.h" ]; then
        info "OpenSBI source snapshot already present at ${OPENSBI_DIR}"
    else
        mkdir -p "$(dirname "${OPENSBI_DIR}")"
        info "Cloning OpenSBI into ${OPENSBI_DIR}"
        git clone "${OPENSBI_REMOTE}" "${OPENSBI_DIR}"
        if [ -n "${OPENSBI_REF}" ]; then
            git -C "${OPENSBI_DIR}" checkout "${OPENSBI_REF}"
        fi
    fi
}

setup_zephyr() {
    if ! command -v west >/dev/null 2>&1; then
        err "west not found. Install host tools first."
        exit 1
    fi

    mkdir -p "${ZEPHYR_WS_DIR}"

    if [ ! -d "${ZEPHYR_WS_DIR}/.west" ]; then
        info "Initializing west workspace in ${ZEPHYR_WS_DIR} (${ZEPHYR_REF})"
        west init -m https://github.com/zephyrproject-rtos/zephyr --mr "${ZEPHYR_REF}" "${ZEPHYR_WS_DIR}"
    fi

    (
        cd "${ZEPHYR_WS_DIR}"
        info "Updating Zephyr modules"
        west update
        west zephyr-export
        info "Installing Zephyr Python requirements"
        python3 -m pip install -r zephyr/scripts/requirements.txt
    )
}

setup_freertos() {
    local kernel_dir="${FREERTOS_WS_DIR}/FreeRTOS-Kernel"
    mkdir -p "${FREERTOS_WS_DIR}"

    if [ ! -d "${kernel_dir}/.git" ]; then
        info "Cloning FreeRTOS-Kernel into ${kernel_dir} (${FREERTOS_REF})"
        git clone --depth 1 --branch "${FREERTOS_REF}" https://github.com/FreeRTOS/FreeRTOS-Kernel.git "${kernel_dir}"
    else
        info "Updating FreeRTOS-Kernel in ${kernel_dir}"
        git -C "${kernel_dir}" fetch --depth 1 origin "${FREERTOS_REF}"
        git -C "${kernel_dir}" checkout "${FREERTOS_REF}"
        git -C "${kernel_dir}" pull --ff-only origin "${FREERTOS_REF}"
    fi
}

setup_buildroot() {
    local buildroot_dir="${BUILDROOT_WS_DIR}/buildroot"
    mkdir -p "${BUILDROOT_WS_DIR}"

    if [ ! -d "${buildroot_dir}/.git" ]; then
        info "Cloning Buildroot into ${buildroot_dir} (${BUILDROOT_REF})"
        git clone --depth 1 --branch "${BUILDROOT_REF}" https://github.com/buildroot/buildroot.git "${buildroot_dir}"
    else
        info "Updating Buildroot in ${buildroot_dir}"
        git -C "${buildroot_dir}" fetch --depth 1 origin "${BUILDROOT_REF}"
        git -C "${buildroot_dir}" checkout "${BUILDROOT_REF}"
        git -C "${buildroot_dir}" pull --ff-only origin "${BUILDROOT_REF}"
    fi
}

verify_env() {
    step "Verifying host tools"
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

    step "Verifying Python helpers"
    if python3 -c "import elftools" >/dev/null 2>&1; then
        note_ok "Python module: elftools"
    elif find "${HOME}/.local/share/pipx/venvs/west" -maxdepth 4 -type d -name elftools >/dev/null 2>&1; then
        note_ok "Python module: elftools (via pipx west venv)"
    else
        note_fail "Python module missing: elftools"
    fi

    step "Verifying dependency workspaces"
    check_file "${OPENSBI_DIR}/README.md" "OpenSBI tree"
    check_file "${ZEPHYR_WS_DIR}/.west/config" "Zephyr west workspace"
    check_file "${ZEPHYR_WS_DIR}/zephyr/west.yml" "Zephyr source tree"
    check_file "${FREERTOS_WS_DIR}/FreeRTOS-Kernel/README.md" "FreeRTOS kernel"
    check_file "${BUILDROOT_WS_DIR}/buildroot/Makefile" "Buildroot tree"
    check_file "${BUILDROOT_WS_DIR}/risc5_eval_linux_amp_defconfig" "Buildroot defconfig"
    check_file "${ROOT_DIR}/config/amp_layout.json" "AMP layout config"

    if [ "${RUN_SMOKE_BUILDS}" -eq 1 ]; then
        step "Running smoke builds"
        run_smoke "Hart0 bare-metal build" make -C "${ROOT_DIR}" -f boot/hart0/Makefile clean hart0
        run_smoke "FreeRTOS Hart2 build" bash "${ROOT_DIR}/tools/scripts/build_freertos_hart2.sh"
        run_smoke "Zephyr Hart1 build" env ZEPHYR_HART1_TARGET=qemu ZEPHYR_PRISTINE=always bash "${ROOT_DIR}/tools/scripts/build_zephyr_hart1.sh"
        run_smoke "OpenSBI FW_DYNAMIC build" env FW_MODE=dynamic bash "${ROOT_DIR}/tools/scripts/build_opensbi_amp.sh"
    fi

    if [ "${RUN_BUILDROOT_DEFCONFIG}" -eq 1 ]; then
        local buildroot_dir="${BUILDROOT_WS_DIR}/buildroot"
        local external_dir="${BUILDROOT_WS_DIR}/external"
        local defconfig_name="risc5_eval_linux_amp_defconfig"
        local defconfig_dst="${buildroot_dir}/configs/${defconfig_name}"
        local verify_out="/tmp/risc5_buildroot_verify"

        step "Validating Buildroot defconfig"
        if [ -d "${buildroot_dir}" ] && [ -d "${external_dir}" ]; then
            cp -f "${BUILDROOT_WS_DIR}/${defconfig_name}" "${defconfig_dst}"
            rm -rf "${verify_out}"
            if make -C "${buildroot_dir}" BR2_EXTERNAL="${external_dir}" O="${verify_out}" "${defconfig_name}"; then
                note_ok "Buildroot defconfig"
            else
                note_fail "Buildroot defconfig"
            fi
        else
            note_fail "Buildroot directories are not ready for defconfig validation"
        fi
    fi

    if [ "${FAILURES}" -ne 0 ]; then
        err "Verification finished with ${FAILURES} failure(s)"
        exit 1
    fi

    ok "Verification finished successfully"
}

main() {
    install_host_tools

    step "Initializing dependencies"
    setup_opensbi
    setup_zephyr
    setup_freertos
    setup_buildroot
    ok "Dependencies initialized"

    if [ "${RUN_VERIFY}" -eq 1 ]; then
        verify_env
    fi

    ok "Environment bootstrap complete"
}

for arg in "$@"; do
    case "${arg}" in
        --skip-verify)
            RUN_VERIFY=0
            ;;
        --skip-smoke-builds)
            RUN_SMOKE_BUILDS=0
            ;;
        --skip-buildroot-defconfig)
            RUN_BUILDROOT_DEFCONFIG=0
            ;;
    esac
done

main
'@

Write-Step "Enabling WSL and Virtual Machine Platform features"
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart | Out-Host
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart | Out-Host

Write-Step "Installing WSL with distro '$Distro'"
wsl.exe --install -d $Distro
if ($LASTEXITCODE -ne 0) {
    throw "wsl --install failed."
}

Write-Step "Ensuring WSL 2 is the default"
wsl.exe --set-default-version 2
if ($LASTEXITCODE -ne 0) {
    throw "wsl --set-default-version 2 failed."
}

Write-Step "Checking distro status"
wsl.exe -l -v | Out-Host

Write-Step "Writing embedded Ubuntu bootstrap into the repo"
$escapedBootstrap = $embeddedBootstrap.Replace("'", "'\"'\"'")
Invoke-WslBash -Command "cat > '$bootstrapPath' <<'EOF'
$escapedBootstrap
EOF
chmod +x '$bootstrapPath'"

$bootstrapArgs = @()
if ($SkipVerify) {
    $bootstrapArgs += "--skip-verify"
}
if ($SkipSmokeBuilds) {
    $bootstrapArgs += "--skip-smoke-builds"
}
if ($SkipBuildrootDefconfig) {
    $bootstrapArgs += "--skip-buildroot-defconfig"
}

$argString = ""
if ($bootstrapArgs.Count -gt 0) {
    $argString = " " + ($bootstrapArgs -join " ")
}

Write-Step "Running embedded bootstrap inside WSL Ubuntu"
Invoke-WslBash -Command "cd '$repoWslPath' && '$bootstrapPath' '$repoWslPath'$argString"

Write-Info "WSL Ubuntu + repo environment are ready."
Write-Info "Open Ubuntu with: wsl -d $Distro"
Write-Info "Then enter the repo: cd '$repoWslPath'"
