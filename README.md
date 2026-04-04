# RISC-V HiFive Unmatched Multi-Hart Project

## Overview

This project scaffolds a heterogeneous multi-hart firmware stack for the **SiFive HiFive Unmatched** development board, featuring:

- **Hart 0 (S7)**: Bare-metal boot coordinator and direct M-mode QEMU entry point
- **Hart 1 (U74)**: Zephyr RTOS
- **Hart 2 (U74)**: FreeRTOS
- **Hart 3 (U74)**: Bare-metal application 1
- **Hart 4 / Linux side**: Buildroot Linux artifacts plus OpenSBI hand-off support for the AMP flow

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


### Dockerized Dev Environment

Docker workflow:

```bash
docker compose build dev
```

Inside the Docker container shell run:

```bash
make bootstrap-env
```

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


### VS Code Tasks

`.vscode/tasks.json` is set up around the direct M-mode QEMU workflow. The available tasks currently map to these commands:

| VS Code task label | Command |
|--------------------|---------|
| `Setup environment` | `bash tools/scripts/bootstrap_env.sh` |
| `Clean build all & Run` | `make qemu-hart0-mmode` |
| `Hart1 Zephyr build` | `make zephyr-hart1-fast` |
| `Hart2 FreeRTOS build` | `make -C apps/freertos-hart2 app2 FREERTOS_KERNEL_DIR=${workspaceFolder}/deps/freertos/FreeRTOS-Kernel` |
| `Hart3 BM build` | `make -C apps/bare-hart3 app3` |
| `Hart4 Linux build` | `make buildroot-linux-fast` |
| `OpenSBI Build` | `make qemu-hart0-mmode` |
| `QEMU Sys Run` | `make qemu-hart0-mmode-run` |

### CLI Equivalents

If you are working from the terminal instead of the VS Code task runner, these are the matching commands:

```bash
make qemu-hart0-mmode
make zephyr-hart1-fast
make -C apps/freertos-hart2 app2 FREERTOS_KERNEL_DIR=$(pwd)/deps/freertos/FreeRTOS-Kernel
make -C apps/bare-hart3 app3
make buildroot-linux-fast
make qemu-hart0-mmode-run
```

### Verification Only

```bash
make verify-toolchain
make verify-toolchain-full
make docker-verify
```

## Architecture Highlights

- **Hart 0 (S7)**: Runs in Machine Mode (M-mode), orchestrates hart wake-up via CLINT, and drives the QEMU direct M-mode flow
- **Hart 1-3 (U74)**: Run heterogeneous Zephyr, FreeRTOS, and bare-metal workloads
- **Linux AMP side**: Buildroot artifacts and OpenSBI support are used by the QEMU/Linux hand-off flow documented in the task runner
- **Inter-hart Communication**: CLINT (timer interrupts), PLIC (external interrupts), shared memory
- **Memory Isolation**: Each hart has private 256KB region; shared peripherals at fixed addresses

## References

- [SiFive HiFive Unmatched Documentation](https://sifive.cdn.prismic.io/sifive/1a82e1f0-2b36-4403-9e88-a4f5fbb8e79f_hifive-unmatched-getting-started-guide.pdf)
- [Zephyr RISC-V Support](https://docs.zephyrproject.org/latest/boards/riscv/index.html)
- [FreeRTOS RISC-V Port](https://www.freertos.org/RTOS-RISC-V.html)
- [RISC-V ISA Manual](https://riscv.org/specifications/)
