# Setup Guide: RISC-V HiFive Unmatched Multi-Hart Build

## Prerequisites Checklist

## Fastest Path

For a Debian/Ubuntu host, the repo now provides a single setup entry point:

```bash
cd risc5_eval
make bootstrap-env
```

The new single-file bootstrap lives at `tools/scripts/bootstrap_env.sh`, so if you want to copy just one setup script into another repo, use that file.

The older wrapper still works too:

```bash
cd risc5_eval
make setup-all
```

This will:
1. install the host packages used by the project
2. initialize OpenSBI, Zephyr, FreeRTOS, and Buildroot under `deps/`
3. verify the toolchain and dependency workspaces
4. run smoke builds for Hart 0, Zephyr Hart 1, FreeRTOS Hart 2, OpenSBI, and Buildroot defconfig

If you only want to re-check the environment later:

```bash
make verify-toolchain
make verify-toolchain-full
```

## Docker Workflow

For a reproducible build environment shared across every clone, the repo now ships with a Docker/Compose setup.

Build the image:

```bash
make docker-build
```

Initialize repo dependencies inside the container:

```bash
make docker-bootstrap
```

Open a shell in the container:

```bash
make docker-shell
```

Then run the normal repo commands from inside that shell:

```bash
make build-all
make qemu
make verify-toolchain
```

How it works:
1. `docker/Dockerfile` installs the pinned Ubuntu-based toolchain used by this repo.
2. `compose.yml` bind-mounts the checkout at `/workspace`.
3. Named volumes keep the container home directory and Buildroot output cache across runs.
4. The existing repo scripts remain the source of truth, so Docker and non-Docker flows stay aligned.

If you need to verify the repo from outside the shell, use:

```bash
make docker-verify
```

## Windows Host With WSL Ubuntu

If your main machine is Windows and you want to run QEMU from Ubuntu under WSL2, use the bundled PowerShell helper from an Administrator PowerShell window:

```powershell
powershell -ExecutionPolicy Bypass -File tools/scripts/setup_wsl_ubuntu_qemu.ps1
```

What it does:
1. enables the Windows WSL and Virtual Machine Platform features
2. installs the `Ubuntu` WSL distro
3. sets WSL 2 as default
4. writes and runs an embedded Ubuntu bootstrap from inside the repo

Useful options:

```powershell
# Use a different distro name already registered in WSL
powershell -ExecutionPolicy Bypass -File tools/scripts/setup_wsl_ubuntu_qemu.ps1 -Distro Ubuntu-24.04

# Skip verification at the end
powershell -ExecutionPolicy Bypass -File tools/scripts/setup_wsl_ubuntu_qemu.ps1 -SkipVerify

# Keep verification but skip smoke builds
powershell -ExecutionPolicy Bypass -File tools/scripts/setup_wsl_ubuntu_qemu.ps1 -SkipSmokeBuilds
```

After the script completes:

```powershell
wsl -d Ubuntu
```

Then inside Ubuntu:

```bash
cd /mnt/d/PhdWork/PhdThesis/risc5_eval
make qemu
```

- [ ] RISC-V toolchain installed and in PATH:
  ```bash
  riscv64-unknown-elf-gcc --version
  riscv64-unknown-elf-ld --version
  riscv64-unknown-elf-objcopy --version
  ```
- [ ] CMake 3.16+ installed
  ```bash
  cmake --version
  ```
- [ ] Git installed for cloning dependencies
- [ ] (Optional) Zephyr SDK installed for Hart 1 development
- [ ] (Optional) FreeRTOS source for Hart 2 development

## Clone & Link Dependencies

### Option 1: Using Git Submodules (Recommended)

```bash
cd risc5_eval

# Initialize git repo if not already done
git init

# Add Zephyr as submodule
git submodule add https://github.com/zephyrproject-rtos/zephyr.git external/zephyr

# Add FreeRTOS as submodule
git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel.git external/freertos-kernel

# Add SiFive SDK (optional reference)
git submodule add https://github.com/sifive/freedom-u-sdk.git external/freedom-u-sdk

# Update submodules
git submodule update --init --recursive
```

### Option 2: Direct Clones

```bash
# Create external directory
mkdir -p external

# Clone Zephyr
git clone https://github.com/zephyrproject-rtos/zephyr.git external/zephyr

# Clone FreeRTOS
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git external/freertos-kernel

# Clone SiFive SDK (reference)
git clone https://github.com/sifive/freedom-u-sdk.git external/freedom-u-sdk
```

## Build System: CMake Integration

### Configure Build System

```bash
cd risc5_eval
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

This will:
1. Detect RISC-V toolchain (assumes `riscv64-unknown-elf-*` in PATH)
2. Generate build files for all harts
3. Set up linker scripts and compilation flags

### Build Individual Harts

```bash
# Build Hart 0 (Boot Coordinator, S7 core)
make hart0

# Build Hart 3 (Bare-metal app 1, U74 core)
make hart3

# Build Hart 4 (Bare-metal app 2, U74 core)
make hart4
```

### Build All (Using Makefile)

```bash
cd risc5_eval
make all           # Full build
make clean         # Clean artifacts
make help          # Show available targets
```

## Hart-Specific Build Instructions

### Hart 0: Bare-Metal Boot Coordinator

**Status**: ✅ Ready to build
**Location**: `boot/hart0/`

The boot coordinator:
1. Initializes Hart 0 (S7 core) in Machine Mode
2. Sets up UART console for debugging
3. Wakes up other harts via CLINT memory-mapped registers
4. Coordinates inter-hart synchronization

**Build**:
```bash
cd risc5_eval
make hart0
# Output: build/hart0.elf, build/hart0.bin
```

**Verify**:
```bash
riscv64-unknown-elf-objdump -d build/hart0.elf | head -50
```

---

### Hart 1: Zephyr RTOS

**Status**: ⚠️ Requires Zephyr setup
**Location**: `apps/zephyr-hart1/`

**Prerequisites**:
1. Install Zephyr SDK: [Zephyr Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
2. Clone Zephyr source:
   ```bash
   git clone https://github.com/zephyrproject-rtos/zephyr.git external/zephyr
   ```

**Build Hart 1 (Zephyr)**:

Option A: Using Zephyr's own build system:
```bash
cd apps/zephyr-hart1
west build -b hifive_unmatched -d build
```

Option B: Or configure Zephyr west manifest to include this project.

**Note**: Zephyr may not have native HiFive Unmatched U74 support. You may need to:
1. Create a custom board definition in `zephyr/boards/riscv/hifive_unmatched_hart1/`
2. Define device tree and KConfig for Hart 1's memory region (0x80040000)
3. Create Zephyr CMakeLists.txt integration

See `apps/zephyr-hart1/prj.conf` for configuration hints.

---

### Hart 2: FreeRTOS

**Status**: ⚠️ Requires FreeRTOS setup
**Location**: `apps/freertos-hart2/`

**Prerequisites**:
1. Clone FreeRTOS:
   ```bash
   git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git external/freertos-kernel
   ```

**Build Hart 2 (FreeRTOS)**:

Create `apps/freertos-hart2/main.c` or `app_main.c` with your application logic:
```c
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

void app_task(void *pvParameters) {
    // Your task code
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {
    xTaskCreate(app_task, "App", 1024, NULL, 1, NULL);
    vTaskStartScheduler();
    return 0;
}
```

Build:
```bash
cd apps/freertos-hart2
make -f ../../configs/freertos.makefile
# or configure CMakeLists.txt to link FreeRTOS
```

**Note**: FreeRTOS RISC-V port requires:
- Linker script for Hart 2 memory (0x80080000–0x800BFFFF)
- FreeRTOS kernel configuration (FreeRTOSConfig.h)
- CLINT timer setup for tick interrupt (Hart 2's MTIMECMP2)

---

### Harts 3 : Bare-Metal Application

**Status**: ✅ Ready to build
**Locations**: `apps/bare-hart3/`

These are minimal bare-metal skeletons. Customize `main.c` with your application logic.

**Build Hart 3**:
```bash
cd risc5_eval
make hart3
# Output: build/hart3.elf, build/hart3.bin
```

---

## Device Tree Configuration

The device tree (`src/device-tree/hifive-unmatched.dts`) defines:
- Hart cores (S7, U74 x4)
- CLINT (inter-hart interrupts, timers)
- PLIC (platform interrupt controller)
- UART0 console
- Memory regions

**To customize**:
1. Edit `src/device-tree/hifive-unmatched.dts`
2. Recompile device tree binary (if needed for bootloader):
   ```bash
   dtc -I dts -O dtb -o hifive-unmatched.dtb hifive-unmatched.dts
   ```

---

## Platform Configuration

The platform header (`src/platform/platform.h`) defines:
- Hart memory regions
- Peripheral base addresses (CLINT, PLIC, UART)
- Helper macros for reading hart ID, MTIME, WFI

**Example Usage**:
```c
#include "platform.h"

uint32_t my_hart = read_hart_id();
uint64_t current_time = read_mtime();

// Wait for interrupt
wfi();
```

---

## Build Artifact Locations

After building, check:
- `build/hart0.elf` — Hart 0 executable
- `build/hart0.bin` — Hart 0 binary image
- `build/hart3.elf` — Hart 3 executable
- `build/hart3.bin` — Hart 3 binary image
- `build/hart4.elf` — Hart 4 executable
- `build/hart4.bin` — Hart 4 binary image

---

## Flashing to HiFive Unmatched

### Using OpenOCD

1. Install OpenOCD with RISC-V support:
   ```bash
   # On macOS:
   brew install openocd

   # On Linux (Ubuntu):
   sudo apt-get install openocd

   # Or build from source:
   git clone https://github.com/riscv/riscv-openocd.git
   cd riscv-openocd && ./bootstrap && ./configure && make install
   ```

2. Connect HiFive Unmatched via USB to host

3. Flash Hart 0 to QSPI flash:
   ```bash
   openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg \
           -f board/sifive-hifive-unmatched.cfg \
           -c "init; flash write_image erase build/hart0.bin 0x20000000; exit"
   ```

4. View serial output:
   ```bash
   screen /dev/ttyUSB0 115200
   # or picocom /dev/ttyUSB0 -b 115200
   ```

---

## Next Immediate Steps

- [ ] **Verify toolchain**: `make hart0` should succeed
- [ ] **Test Hart 0 console**: Expect UART output "Hart 0 Boot Coordinator"
- [ ] **Implement hart wake-up**: Add CLINT-based hart wake logic in `boot/hart0/main.c`
- [ ] **Set up Zephyr** (Hart 1): Clone Zephyr, create board definition for Hart 1
- [ ] **Set up FreeRTOS** (Hart 2): Clone FreeRTOS, configure kernel, link in CMakeLists
- [ ] **Customize Harts 3 **: Add application logic to `apps/bare-hart3/main.c`
- [ ] **Integrate inter-hart communication**: Shared memory regions, IPC mechanisms
- [ ] **Test on hardware**: Flash all harts, verify boot sequence and console output

---

## Troubleshooting

**Q: CMake can't find RISC-V toolchain**
- A: Ensure `riscv64-unknown-elf-gcc` is in your PATH: `which riscv64-unknown-elf-gcc`

**Q: Linker errors about undefined symbols**
- A: Check that linker scripts have correct memory regions and match hart memory allocation in platform.h

**Q: Hart doesn't wake up after flashing**
- A: Verify Hart 0 boot code implements CLINT-based hart wake-up (example: write to CLINT_MSIP* register for IPI)

**Q: Build succeeds but ELF won't flash**
- A: Ensure OpenOCD config matches your FTDI adapter. Try with `-c "init; reset halt"`

---

## References

- SiFive HiFive Unmatched: https://www.sifive.com/boards/hifive-unmatched
- RISC-V ISA: https://riscv.org/specifications/
- Zephyr Docs: https://docs.zephyrproject.org/
- FreeRTOS: https://www.freertos.org/
- Device Tree: https://devicetree.org/
