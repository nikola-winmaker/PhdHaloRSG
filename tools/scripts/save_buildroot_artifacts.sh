#!/usr/bin/env bash
# Save Buildroot artifacts to version control directory
# This allows other users to skip the long Buildroot build and use pre-built artifacts

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILDROOT_OUTPUT_DIR="${BUILDROOT_OUTPUT_DIR:-${HOME}/risc5_buildroot_output}"
ARTIFACTS_DIR="${ROOT_DIR}/artifacts/buildroot/images"

echo "[INFO] Saving Buildroot artifacts..."

# Check if the build artifacts exist
if [ ! -f "${BUILDROOT_OUTPUT_DIR}/images/Image" ]; then
    echo "[ERR] Linux kernel image not found at: ${BUILDROOT_OUTPUT_DIR}/images/Image"
    echo "      You need to build Buildroot first: make buildroot-linux"
    exit 1
fi

if [ ! -f "${BUILDROOT_OUTPUT_DIR}/images/rootfs.cpio" ]; then
    echo "[ERR] Linux rootfs not found at: ${BUILDROOT_OUTPUT_DIR}/images/rootfs.cpio"
    echo "      You need to build Buildroot first: make buildroot-linux"
    exit 1
fi

# Create artifacts directory if needed
mkdir -p "${ARTIFACTS_DIR}"

# Copy artifacts
echo "[INFO] Copying Image (kernel)..."
cp -v "${BUILDROOT_OUTPUT_DIR}/images/Image" "${ARTIFACTS_DIR}/"

echo "[INFO] Copying rootfs.cpio (filesystem)..."
cp -v "${BUILDROOT_OUTPUT_DIR}/images/rootfs.cpio" "${ARTIFACTS_DIR}/"

echo "[OK] Buildroot artifacts saved to: ${ARTIFACTS_DIR}"
echo ""
echo "Next steps:"
echo "  1. Review changes:"
echo "     git status"
echo "  2. Stage the artifacts:"
echo "     git add ${ARTIFACTS_DIR}/"
echo "  3. Commit:"
echo "     git commit -m 'Update Buildroot Linux kernel and rootfs artifacts'"
echo "  4. Push:"
echo "     git push"
echo ""
echo "Other users can now skip the Buildroot build and run:"
echo "  make qemu-hart0-mmode-run"
