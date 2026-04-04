# RISC-V HiFive Unmatched Multi-Hart Project

## Overview

This project scaffolds a heterogeneous multi-hart firmware stack for the **SiFive HiFive Unmatched** development board, featuring:

- **Hart 0 (S7)**: Bare-metal boot coordinator
- **Hart 1 (U74)**: Zephyr RTOS
- **Hart 2 (U74)**: FreeRTOS
- **Hart 3 (U74)**: Bare-metal application 1
- **Hart 4 (U74)**: Bare-metal application 2

## Directory Structure

```
risc5_eval/
├── boot/
│   └── hart0/               # Hart 0 boot coordinator (S7 core)
│       ├── bootstrap.S      # RISC-V assembly entry point
│       ├── main.c           # Boot coordinator main loop
│       ├── linker.ld        # Memory layout for Hart 0
│       └── CMakeLists.txt
│
├── apps/
│   ├── zephyr-hart1/        # Hart 1 Zephyr RTOS
│   │   ├── src/
│   │   ├── prj.conf         # Zephyr configuration
│   │   └── CMakeLists.txt   # Zephyr build integration
│   │
│   ├── freertos-hart2/      # Hart 2 FreeRTOS
│   │   ├── src/
│   │   └── CMakeLists.txt   # FreeRTOS build integration
│   │
│   ├── bare-hart3/          # Hart 3 bare-metal app 1
│   │   ├── bootstrap.S
│   │   ├── main.c
│   │   ├── linker.ld
│   │   └── CMakeLists.txt
│   │
│   └── bare-hart4/          # Hart 4 bare-metal app 2
│       ├── bootstrap.S
│       ├── main.c
│       ├── linker.ld
│       └── CMakeLists.txt
│
├── src/
│   ├── common/              # Shared utilities
│   ├── device-tree/         # Platform device tree definition
│   │   └── hifive-unmatched.dts
│   └── platform/            # Platform headers & constants
│       └── platform.h
│
├── configs/
│   ├── riscv-toolchain.cmake # CMake RISC-V toolchain
│   └── build.cmake           # Build configuration
│
├── tools/
│   ├── scripts/             # Utility scripts (build, flash, debug)
│   │   ├── build.sh
│   │   └── flash.sh
│   └── toolchain/           # Toolchain files
│
├── build/                   # Build artifacts (generated)
├── docs/                    # Documentation
│   ├── architecture.md      # Design overview
│   └── hart-allocation.md   # Memory and hart management
│
├── CMakeLists.txt           # Root CMake configuration
├── Makefile                 # Convenience build targets
├── .gitignore
└── README.md
```

## Memory Layout

HiFive Unmatched provides 256MB of RAM (0x80000000–0x8FFFFFFF). Default allocation:

| Hart | Core | Start Address | Size    | Purpose                       |
|------|------|---------------|---------|-------------------------------|
| 0    | S7   | 0x8000_0000   | 256K    | Boot coordinator              |
| 1    | U74  | 0x8004_0000   | 256K    | Zephyr RTOS                   |
| 2    | U74  | 0x8008_0000   | 256K    | FreeRTOS                      |
| 3    | U74  | 0x800C_0000   | 256K    | Bare-metal app 1              |
| 4    | U74  | 0x8010_0000   | 256K    | Bare-metal app 2              |

## Quick Start

### One-Command Setup

```bash
make bootstrap-env
```

or, with the older wrapper entry point:

```bash
make setup-all
```

`make bootstrap-env` is the single-file bootstrap entry point. It installs the host packages used by the repo, initializes OpenSBI/Zephyr/FreeRTOS/Buildroot under `deps/`, and runs verification with smoke builds.

### Dockerized Dev Environment

If you want everyone who clones the repo to build with the same toolchain, use the pinned Docker workflow:

```bash
make docker-build
make docker-bootstrap
make docker-shell
```

From inside the container shell, run the normal repo targets such as:

```bash
make build-all
make qemu
make verify-toolchain
```

Notes:
- `compose.yml` mounts the repo into `/workspace`, so artifacts stay in your checkout.
- The container keeps a named home volume for `west`, `ccache`, pipx state, and Buildroot output cache.
- `make docker-bootstrap` reuses the existing bootstrap flow, but skips host package installation because the image already contains the toolchain.

### Windows + WSL Ubuntu For QEMU

If you are starting from Windows and want the QEMU workflow inside WSL Ubuntu, run this from an elevated PowerShell window:

```powershell
powershell -ExecutionPolicy Bypass -File tools/scripts/setup_wsl_ubuntu_qemu.ps1
```

That script:
- enables WSL + Virtual Machine Platform
- installs Ubuntu on WSL2
- runs an embedded Ubuntu bootstrap inside WSL
- verifies the toolchain and QEMU-oriented dependencies

### Prerequisites

1. **RISC-V Toolchain** (installed in PATH):
   ```bash
   riscv64-unknown-elf-gcc
   riscv64-unknown-elf-ld
   riscv64-unknown-elf-objcopy
   ```

2. **CMake** 3.16+

3. **Zephyr SDK** (for Hart 1 Zephyr build)

4. **FreeRTOS** sources (for Hart 2)

### Build Hart 0 (Boot Coordinator)

```bash
make hart0
```

### Build All Harts

```bash
make all
```

### Detailed Build Instructions

See [SETUP.md](SETUP.md) for step-by-step build and flashing instructions.

### Verification Only

```bash
make verify-toolchain
make verify-toolchain-full
make docker-verify
```

## Architecture Highlights

- **Hart 0 (S7)**: Runs in Machine Mode (M-mode), orchestrates hart wake-up via CLINT
- **Harts 1–4 (U74)**: Run heterogeneous RTOS/bare-metal workloads
- **Inter-hart Communication**: CLINT (timer interrupts), PLIC (external interrupts), shared memory
- **Memory Isolation**: Each hart has private 256KB region; shared peripherals at fixed addresses

## Next Steps

1. **Clone dependencies** (Zephyr, FreeRTOS SDK) — see [SETUP.md](SETUP.md#clone-dependencies)
2. **Test Hart 0 build** — confirm toolchain and linker scripts work
3. **Configure device tree** — adjust HART*_MEM_BASE if using custom memory layout
4. **Implement Hart 0 boot logic** — wake up other harts via CLINT
5. **Integrate Zephyr** — set up Hart 1 build with Zephyr manifest and board definition
6. **Integrate FreeRTOS** — configure Hart 2 with FreeRTOS kernel and device drivers
7. **Flash & debug** — use tools/scripts for flashing and serial console

## References

- [SiFive HiFive Unmatched Documentation](https://sifive.cdn.prismic.io/sifive/1a82e1f0-2b36-4403-9e88-a4f5fbb8e79f_hifive-unmatched-getting-started-guide.pdf)
- [Zephyr RISC-V Support](https://docs.zephyrproject.org/latest/boards/riscv/index.html)
- [FreeRTOS RISC-V Port](https://www.freertos.org/RTOS-RISC-V.html)
- [RISC-V ISA Manual](https://riscv.org/specifications/)
