#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
WS_DIR="${ROOT_DIR}/deps/buildroot"
BUILDROOT_DIR="${WS_DIR}/buildroot"
BUILDROOT_REF="${BUILDROOT_REF:-2024.02.9}"
EXTERNAL_DIR="${WS_DIR}/external"
DEFCONFIG_PATH="${WS_DIR}/risc5_eval_linux_amp_defconfig"
APP_BUILDROOT_DIR="${ROOT_DIR}/apps/linux-hart4/buildroot"
APP_ROOTFS_OVERLAY_DIR="${ROOT_DIR}/apps/linux-hart4/rootfs-overlay"

seed_buildroot_scaffold() {
    local board_dir="${EXTERNAL_DIR}/board/risc5_eval"
    local overlay_dir="${board_dir}/rootfs-overlay/etc/init.d"

    mkdir -p "${overlay_dir}"

    cat > "${EXTERNAL_DIR}/external.desc" <<'EOF'
name: RISC5_EVAL
desc: RISC5 Eval Linux AMP external tree
EOF

    cat > "${EXTERNAL_DIR}/Config.in" <<'EOF'
comment "RISC5 Eval external options"
EOF

    cat > "${EXTERNAL_DIR}/external.mk" <<'EOF'
# RISC5 Eval Buildroot external tree
EOF

    install -m 0644 "${APP_BUILDROOT_DIR}/device_table.txt" "${board_dir}/device_table.txt"
    install -m 0755 "${APP_BUILDROOT_DIR}/post-build.sh" "${board_dir}/post-build.sh"
    install -m 0755 \
        "${APP_ROOTFS_OVERLAY_DIR}/etc/init.d/S90amp-hart0-app" \
        "${overlay_dir}/S90amp-hart0-app"

    cat > "${DEFCONFIG_PATH}" <<'EOF'
BR2_riscv=y
BR2_RISCV_64=y
BR2_TOOLCHAIN_BUILDROOT_GLIBC=y
BR2_TARGET_GENERIC_HOSTNAME="risc5-amp"
BR2_TARGET_GENERIC_ISSUE="RISC5 Eval Linux AMP"
BR2_TARGET_GENERIC_GETTY_PORT="ttyS0"
BR2_TARGET_ROOTFS_CPIO=y
BR2_ROOTFS_OVERLAY="$(BR2_EXTERNAL_RISC5_EVAL_PATH)/board/risc5_eval/rootfs-overlay"
BR2_ROOTFS_POST_BUILD_SCRIPT="$(BR2_EXTERNAL_RISC5_EVAL_PATH)/board/risc5_eval/post-build.sh"
BR2_ROOTFS_DEVICE_TABLE="$(BR2_EXTERNAL_RISC5_EVAL_PATH)/board/risc5_eval/device_table.txt"
BR2_LINUX_KERNEL=y
BR2_LINUX_KERNEL_CUSTOM_VERSION=y
BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE="6.6.32"
BR2_LINUX_KERNEL_USE_ARCH_DEFAULT_CONFIG=y
BR2_LINUX_KERNEL_IMAGE=y
EOF
}

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

echo "[INFO] Seeding Buildroot project scaffold under ${WS_DIR}"
seed_buildroot_scaffold

echo "[OK] Buildroot workspace ready: ${WS_DIR}"
echo "[INFO] Project defconfig scaffold: ${WS_DIR}/risc5_eval_linux_amp_defconfig"
