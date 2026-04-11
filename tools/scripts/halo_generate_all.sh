#!/bin/bash
set -e

halo generate -i "$PWD/deps/halo/hc_out" -o "$PWD/apps/bare-hart3/deps/halo" --platform riscv64_h3_baremetal
halo generate -i "$PWD/deps/halo/hc_out" -o "$PWD/apps/freertos-hart2/deps/halo" --platform riscv64_h2_freertos
halo generate -i "$PWD/deps/halo/hc_out" -o "$PWD/apps/zephyr-hart1/deps/halo" --platform riscv64_h1_zephyr
halo generate -i "$PWD/deps/halo/hc_out" -o "$PWD/apps/linux-hart4/deps/halo" --platform riscv64_h4_linux
