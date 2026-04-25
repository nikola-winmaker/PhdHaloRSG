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
PhdHaloRSG/
├── boot/                  # Hart 0 boot coordinator (S7 core)
│   └── hart0/             # Boot code, linker, and startup for Hart 0
│
├── apps/
│   ├── zephyr-hart1/      # Hart 1 Zephyr RTOS app (Zephyr build integration)
│   ├── freertos-hart2/    # Hart 2 FreeRTOS app (standalone Makefile)
│   ├── bare-hart3/        # Hart 3 bare-metal app
│   └── linux-hart4/       # Hart 4 Linux app (Buildroot integration)
│
├── src/                   # Common platform code, device tree, and shared logic
│   ├── common/            # Shared C sources
│   ├── device-tree/       # Platform device tree(s)
│   └── platform/          # Platform headers & constants
│
├── configs/               # Toolchain and build configuration
├── tools/
│   ├── scripts/           # Build, flash, and setup scripts
│   └── toolchain/         # Toolchain helper files
├── halo_dist/             # HALO models, protocols, and generated code
├── deps/                  # External dependencies (Zephyr, FreeRTOS, Buildroot, OpenSBI)
├── artifacts/             # Build artifacts (e.g., Linux images)
├── docs/                  # Documentation and diagrams
├── build/                 # Build output (generated)
├── Makefile               # Top-level build orchestration
├── CMakeLists.txt         # Root CMake configuration
└── README.md              # This file
```


## Memory Layout (from src/memory_layout.h)

HiFive Unmatched provides 2GB of RAM (0x80000000–0xFFFFFFFF). This project uses a compact multi-hart layout:

| Hart | Core | Start Address | Size    | Purpose           |
|------|------|---------------|---------|-------------------|
| 0    | S7   | 0x80000000    | 1MB     | Boot coordinator  |
| 1    | U74  | 0x80240000    | 256KB   | Zephyr RTOS       |
| 2    | U74  | 0x80280000    | 256KB   | FreeRTOS          |
| 3    | U74  | 0x802C0000    | 256KB   | Bare-metal app 1  |
| 4    | U74  | 0x80300000    | 256KB   | Bare-metal app 2  |
| -    | -    | 0x80340000    | 768KB   | Shared IPC region |

**Alternate Linux AMP region:**
| Region | Start Address | Size    | Purpose         |
|--------|--------------|---------|-----------------|
| Linux  | 0x80400000   | 124MB   | Linux AMP image |

**IPC subregions and protocol buffers** are defined for inter-hart communication (see src/memory_layout.h for details).

---

## VS Code Tasks & CLI Equivalents

The project provides VS Code tasks (see .vscode/tasks.json) for all major build and utility flows:

| Task Label              | Command (CLI Equivalent) |
|-------------------------|--------------------------|
| Setup environment       | sudo bash tools/scripts/bootstrap_env.sh |
| Clean build all & Run   | make qemu-hart0-mmode    |
| Hart1 Zephyr build      | make zephyr-hart1-fast   |
| Hart2 FreeRTOS build    | make -C apps/freertos-hart2 app2 FREERTOS_KERNEL_DIR=... |
| Hart3 BM build          | make -C apps/bare-hart3 app3 |
| Hart4 Linux build       | make buildroot-linux-fast|
| OpenSBI Build           | make qemu-hart0-mmode    |
| QEMU Sys Run            | make qemu-hart0-mmode-run|
| Halo: Compose           | halo compose ...          |
| Halo: Generate          | sudo bash tools/scripts/halo_generate_all.sh |

Inputs for build mode (HALO/Classical) and output selection are supported in the VS Code UI.

---

## Tools & Scripts

- **tools/scripts/bootstrap_env.sh**: Main environment bootstrapper. Installs host packages, initializes dependencies (Zephyr, FreeRTOS, Buildroot, OpenSBI), verifies toolchain, and can run smoke builds. Supports options to skip steps (see --help).
- **tools/scripts/halo_generate_all.sh**: Runs `halo generate` for all platforms, populating each app's deps/halo directory.
- **tools/scripts/build.sh**: Checks prerequisites, configures CMake, builds targets (hart0, hart3, hart4), and provides a flash helper (manual OpenOCD usage).

Other scripts are available for building individual harts, setting up dependencies, and running QEMU (see tools/scripts/ for details).

---

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
| `Clean build all & Run` | `make qemu-hart0-mmode USE_HALO=0 or USE_HALO=1` |
| `Hart1 Zephyr build` | `make zephyr-hart1-fast USE_HALO=0  or USE_HALO=1` |
| `Hart2 FreeRTOS build` | `make apps-freertos USE_HALO=0 or USE_HALO=1` |
| `Hart3 BM build` | `make apps-freertos USE_HALO=0  or USE_HALO=1` |
| `Hart4 Linux build` | `make buildroot-linux-fast USE_HALO=0  or USE_HALO=1` |
| `OpenSBI Build` | `make qemu-hart0-mmode USE_HALO=0 or USE_HALO=1` |
| `QEMU Sys Run` | To run manually:
	- PTY mode:
		1. `python3 tools/console/qemu_apps_console.py`
		2. `echo pty > .out_select`
		3. `make qemu-hart0-mmode-run OUT_SELECT=pty`
	- STDIO mode:
		1. `python3 tools/console/qemu_apps_console.py`
		2. `echo stdio > .out_select`
		3. `make qemu-hart0-mmode-run OUT_SELECT=stdio` |

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
