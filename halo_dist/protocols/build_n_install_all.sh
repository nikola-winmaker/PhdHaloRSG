#!/usr/bin/env bash
# Build and install all HALO protocol generators

set -e

BUILD_WHEELS=false
INSTALL_EDITABLE=false

for arg in "$@"; do
    case "$arg" in
        --build-wheels)
            BUILD_WHEELS=true
            ;;
        --install-editable)
            INSTALL_EDITABLE=true
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: $0 [--build-wheels|--install-editable]"
            exit 1
            ;;
    esac
done

if [ "$BUILD_WHEELS" = false ] && [ "$INSTALL_EDITABLE" = false ]; then
    # Preserve legacy behavior: editable install when no flags are provided.
    INSTALL_EDITABLE=true
fi

echo "================================================"
echo "HALO Protocol Generators - Build & Install All"
echo "================================================"
echo ""

if [ "$BUILD_WHEELS" = true ]; then
    ACTION_LABEL="Building wheels"
else
    ACTION_LABEL="Installing editable packages"
fi

echo "Mode: $ACTION_LABEL"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROTOCOL_GENERATORS_DIR="$SCRIPT_DIR"

# Discover all protocol generator packages
PROTOCOL_PACKAGES=()
for dir in "$PROTOCOL_GENERATORS_DIR"/halo_proto_*/; do
    if [ -f "$dir/pyproject.toml" ]; then
        PROTOCOL_PACKAGES+=("$dir")
    fi
done
for dir in "$PROTOCOL_GENERATORS_DIR"/halo-proto-*/; do
    if [ -f "$dir/pyproject.toml" ]; then
        PROTOCOL_PACKAGES+=("$dir")
    fi
done

if [ ${#PROTOCOL_PACKAGES[@]} -eq 0 ]; then
    echo "No protocol generator packages found"
    exit 0
fi

echo "Found ${#PROTOCOL_PACKAGES[@]} protocol generator(s):"
for pkg in "${PROTOCOL_PACKAGES[@]}"; do
    pkg_name=$(basename "$pkg")
    echo "  - $pkg_name"
done
echo ""

SUCCESS_COUNT=0
FAIL_COUNT=0

if [ "$BUILD_WHEELS" = true ]; then
    echo "Ensuring wheel build backend is available..."
    python -m pip install --upgrade build
    echo ""
fi

for pkg in "${PROTOCOL_PACKAGES[@]}"; do
    pkg_name=$(basename "$pkg")
    echo "$ACTION_LABEL: $pkg_name"
    echo "-----------------------------------"
    
    pushd "$pkg" > /dev/null
    
    if [ "$BUILD_WHEELS" = true ]; then
        cmd=(python -m build --wheel)
        ok_msg="[OK] $pkg_name wheel built successfully"
        fail_msg="[FAIL] $pkg_name wheel build failed"
    else
        cmd=(python -m pip install -e .)
        ok_msg="[OK] $pkg_name installed successfully"
        fail_msg="[FAIL] $pkg_name installation failed"
    fi

    if "${cmd[@]}"; then
        echo "$ok_msg"
        ((SUCCESS_COUNT++))
    else
        echo "$fail_msg"
        ((FAIL_COUNT++))
    fi
    
    popd > /dev/null
    echo ""
done

echo "================================================"
if [ "$BUILD_WHEELS" = true ]; then
    echo "Wheel Build Summary:"
else
    echo "Installation Summary:"
fi
echo "  Success: $SUCCESS_COUNT"
echo "  Failed:  $FAIL_COUNT"
echo "================================================"

if [ $FAIL_COUNT -gt 0 ]; then
    exit 1
fi
