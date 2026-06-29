# RISC-V HiFive Unmatched Multi-Hart Project

## Overview

This project scaffolds a heterogeneous multi-hart firmware stack featuring:

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

This project uses a compact multi-hart layout:

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


## Setup and Run Steps

1. Install Docker Desktop.
2. Run Docker Desktop.
   - Select **Personal use**, or skip the initial setup if needed.
3. Install Visual Studio Code.
4. In VS Code, install these extensions:
   - Docker
   - Dev Containers
   - Todo Tree
5. Clone the PhD Research Study repository from GitHub.
6. Open the repository in VS Code, then open a terminal in the repository root.
7. Build the development container image:

```bash
docker compose build dev
```

8. In VS Code, press `Ctrl+Shift+P` or, on macOS, `Cmd+Shift+P`; search for **Reopen in Container**, and run that command.
9. Inside the container, install these VS Code extensions:
   - Tasks
   - Todo Tree
10. Click **Setup environment** in the VS Code bottom bar.
11. Wait until all submodules and tools are fully installed.
12. From the bottom bar, click **Clean build all & Run**.
13. In the pop-up prompt at the top, select the console option, then select **Classical**.
14. You should see Linux boot in the console, followed by messages in the form `[APPx]`, where `x` is `1`, `2`, `3`, and `4`.

## References

- [Zephyr RISC-V Support](https://docs.zephyrproject.org/latest/boards/riscv/index.html)
- [FreeRTOS RISC-V Port](https://www.freertos.org/RTOS-RISC-V.html)
- [RISC-V ISA Manual](https://riscv.org/specifications/)
