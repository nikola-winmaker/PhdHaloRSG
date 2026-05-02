#!/bin/bash
set -e

OUTPUT_DIRS=(
  "$PWD/apps/bare-hart3/deps/halo"
  "$PWD/apps/freertos-hart2/deps/halo"
  "$PWD/apps/zephyr-hart1/deps/halo"
  "$PWD/apps/linux-hart4/deps/halo"
)

fix_output_ownership() {
  if [[ -n "${SUDO_UID:-}" && -n "${SUDO_GID:-}" ]]; then
    chown -R "$SUDO_UID:$SUDO_GID" "${OUTPUT_DIRS[@]}"
  fi
}

trap fix_output_ownership EXIT

halo generate -i "$PWD/deps/halo/hc_out" -o "${OUTPUT_DIRS[0]}" --platform riscv64_h3_baremetal
halo generate -i "$PWD/deps/halo/hc_out" -o "${OUTPUT_DIRS[1]}" --platform riscv64_h2_freertos
halo generate -i "$PWD/deps/halo/hc_out" -o "${OUTPUT_DIRS[2]}" --platform riscv64_h1_zephyr
halo generate -i "$PWD/deps/halo/hc_out" -o "${OUTPUT_DIRS[3]}" --platform riscv64_h4_linux
