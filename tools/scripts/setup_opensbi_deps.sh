#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
OPENSBI_DIR="${ROOT_DIR}/deps/opensbi"
OPENSBI_REMOTE="${OPENSBI_REMOTE:-https://github.com/riscv-software-src/opensbi.git}"
OPENSBI_REF="${OPENSBI_REF:-}"

gitlink_commit() {
    if git -C "${ROOT_DIR}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        git -C "${ROOT_DIR}" ls-tree HEAD deps/opensbi 2>/dev/null | awk '{print $3}'
    fi
}

if [ -z "${OPENSBI_REF}" ]; then
    OPENSBI_REF="$(gitlink_commit || true)"
fi

if [ -d "${OPENSBI_DIR}/.git" ]; then
    echo "[INFO] OpenSBI repository already present at ${OPENSBI_DIR}"
    if [ -n "${OPENSBI_REF}" ]; then
        echo "[INFO] Checking out pinned OpenSBI ref: ${OPENSBI_REF}"
        git -C "${OPENSBI_DIR}" fetch --tags origin
        git -C "${OPENSBI_DIR}" checkout "${OPENSBI_REF}"
    fi
elif [ -f "${OPENSBI_DIR}/README.md" ] && [ -f "${OPENSBI_DIR}/include/sbi/sbi_types.h" ]; then
    echo "[INFO] OpenSBI source snapshot already present at ${OPENSBI_DIR}"
else
    mkdir -p "$(dirname "${OPENSBI_DIR}")"
    echo "[INFO] Cloning OpenSBI into ${OPENSBI_DIR}"
    git clone "${OPENSBI_REMOTE}" "${OPENSBI_DIR}"

    if [ -n "${OPENSBI_REF}" ]; then
        echo "[INFO] Checking out pinned OpenSBI ref: ${OPENSBI_REF}"
        git -C "${OPENSBI_DIR}" checkout "${OPENSBI_REF}"
    fi
fi

echo "[OK] OpenSBI dependency ready: ${OPENSBI_DIR}"
