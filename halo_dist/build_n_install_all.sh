#!/usr/bin/env bash
# Build/install all HALO protocol and platform packages under halo_dist.

set -euo pipefail

BUILD_WHEELS=false
INSTALL_EDITABLE=false
INSTALL_WHEELS=false

for arg in "$@"; do
    case "$arg" in
        --build-wheels)
            BUILD_WHEELS=true
            ;;
        --install-editable)
            INSTALL_EDITABLE=true
            ;;
        --install-wheels)
            INSTALL_WHEELS=true
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: $0 [--build-wheels] [--install-editable] [--install-wheels]"
            exit 1
            ;;
    esac
done

if [ "$BUILD_WHEELS" = false ] && [ "$INSTALL_EDITABLE" = false ] && [ "$INSTALL_WHEELS" = false ]; then
    # Preserve legacy behavior when called without flags.
    INSTALL_EDITABLE=true
fi

PYTHON_BIN=""
if command -v python3 >/dev/null 2>&1; then
    PYTHON_BIN="python3"
elif command -v python >/dev/null 2>&1; then
    PYTHON_BIN="python"
else
    echo "[ERR] Python interpreter not found (expected 'python3' or 'python')"
    exit 1
fi

SUDO=""
if [ "${EUID}" -ne 0 ] && command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
fi

PIP_COMMON_FLAGS=(--break-system-packages --ignore-installed)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CORE_WHEEL=""
mapfile -t CORE_WHEELS < <(find "${SCRIPT_DIR}" -maxdepth 1 -type f -name "halo-*.whl" | sort)
if [ ${#CORE_WHEELS[@]} -gt 0 ]; then
    CORE_WHEEL="${CORE_WHEELS[-1]}"
fi

SEARCH_ROOTS=()
[ -d "${SCRIPT_DIR}/platforms" ] && SEARCH_ROOTS+=("${SCRIPT_DIR}/platforms")
[ -d "${SCRIPT_DIR}/protocols" ] && SEARCH_ROOTS+=("${SCRIPT_DIR}/protocols")

if [ ${#SEARCH_ROOTS[@]} -eq 0 ]; then
    echo "No HALO package roots found under ${SCRIPT_DIR}"
    exit 0
fi

mapfile -t PACKAGES < <(
    find "${SEARCH_ROOTS[@]}" -mindepth 1 -maxdepth 2 -type f -name pyproject.toml \
        -exec dirname {} \; | sort
)

if [ ${#PACKAGES[@]} -eq 0 ]; then
    echo "No HALO protocol/platform packages found under ${SCRIPT_DIR}/platforms or ${SCRIPT_DIR}/protocols"
    exit 0
fi

echo "================================================"
echo "HALO Packages - Build & Install"
echo "================================================"
echo "Found ${#PACKAGES[@]} package(s):"
for pkg in "${PACKAGES[@]}"; do
    echo "  - $(basename "${pkg}")"
done
if [ -n "${CORE_WHEEL}" ]; then
    echo "Core wheel in halo_dist: $(basename "${CORE_WHEEL}")"
fi
if [ "$INSTALL_EDITABLE" = true ] || [ "$INSTALL_WHEELS" = true ]; then
    echo ""
    echo "Planned install targets:"

    if [ "$INSTALL_EDITABLE" = true ]; then
        echo "  Editable mode:"
        if [ -n "${CORE_WHEEL}" ]; then
            echo "    - core HALO wheel: $(basename "${CORE_WHEEL}")"
        fi
        for pkg in "${PACKAGES[@]}"; do
            echo "    - $(basename "${pkg}")"
        done
    fi

    if [ "$INSTALL_WHEELS" = true ]; then
        echo "  Wheel mode:"
        if [ -n "${CORE_WHEEL}" ]; then
            echo "    - core HALO: $(basename "${CORE_WHEEL}")"
        fi
        for pkg in "${PACKAGES[@]}"; do
            echo "    - $(basename "${pkg}")"
        done
    fi
fi
echo ""

if [ "$BUILD_WHEELS" = true ]; then
    echo "Ensuring wheel build backend is available..."
    "${PYTHON_BIN}" -m pip install "${PIP_COMMON_FLAGS[@]}" --upgrade build
    echo ""
fi

SUCCESS_COUNT=0
FAIL_COUNT=0

run_for_all() {
    local mode="$1"
    local ok_suffix="$2"
    local fail_suffix="$3"

    for pkg in "${PACKAGES[@]}"; do
        local pkg_name
        pkg_name="$(basename "${pkg}")"

        echo "${mode}: ${pkg_name}"
        echo "-----------------------------------"
        pushd "${pkg}" >/dev/null

        local -a cmd
        if [ "${mode}" = "Building wheels" ]; then
            cmd=("${PYTHON_BIN}" -m build --wheel)
        elif [ "${mode}" = "Installing editable" ]; then
            cmd=(${SUDO} "${PYTHON_BIN}" -m pip install "${PIP_COMMON_FLAGS[@]}" -e .)
        else
            local wheel_target=""

            if compgen -G "dist/*.whl" >/dev/null; then
                wheel_target="dist/*.whl"
            else
                local normalized_pkg
                normalized_pkg="${pkg_name//-/_}"
                mapfile -t root_wheels < <(find "${SCRIPT_DIR}" -maxdepth 1 -type f -name "*.whl" | sort)

                for w in "${root_wheels[@]}"; do
                    local wb
                    wb="$(basename "${w}")"
                    if [[ "${wb}" == "${pkg_name}"* ]] || [[ "${wb}" == "${normalized_pkg}"* ]]; then
                        wheel_target="${w}"
                        break
                    fi
                done
            fi

            if [ -z "${wheel_target}" ]; then
                echo "[FAIL] ${pkg_name} wheel not found in dist/ or ${SCRIPT_DIR}"
                FAIL_COUNT=$((FAIL_COUNT + 1))
                popd >/dev/null
                echo ""
                continue
            fi

            cmd=(${SUDO} "${PYTHON_BIN}" -m pip install "${PIP_COMMON_FLAGS[@]}" --force-reinstall ${wheel_target})
        fi

        if "${cmd[@]}"; then
            echo "[OK] ${pkg_name} ${ok_suffix}"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo "[FAIL] ${pkg_name} ${fail_suffix}"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi

        popd >/dev/null
        echo ""
    done
}

if [ "$BUILD_WHEELS" = true ]; then
    run_for_all "Building wheels" "wheel built successfully" "wheel build failed"
fi

if [ "$INSTALL_EDITABLE" = true ]; then
    if [ -n "${CORE_WHEEL}" ]; then
        echo "Installing core HALO wheel: $(basename "${CORE_WHEEL}")"
        echo "-----------------------------------"
        if ${SUDO} "${PYTHON_BIN}" -m pip install "${PIP_COMMON_FLAGS[@]}" --force-reinstall "${CORE_WHEEL}"; then
            echo "[OK] core HALO wheel installed successfully"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo "[FAIL] core HALO wheel install failed"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
        echo ""
    fi

    run_for_all "Installing editable" "installed successfully" "installation failed"
fi

if [ "$INSTALL_WHEELS" = true ]; then
    if [ -n "${CORE_WHEEL}" ]; then
        echo "Installing core HALO wheel: $(basename "${CORE_WHEEL}")"
        echo "-----------------------------------"
        if ${SUDO} "${PYTHON_BIN}" -m pip install "${PIP_COMMON_FLAGS[@]}" --force-reinstall "${CORE_WHEEL}"; then
            echo "[OK] core HALO wheel installed successfully"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo "[FAIL] core HALO wheel install failed"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
        echo ""
    fi

    run_for_all "Installing wheels" "wheel installed successfully" "wheel install failed"
fi

echo "================================================"
echo "HALO package action summary:"
echo "  Success: ${SUCCESS_COUNT}"
echo "  Failed:  ${FAIL_COUNT}"
echo "================================================"

if [ "${FAIL_COUNT}" -gt 0 ]; then
    exit 1
fi
