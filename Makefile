.PHONY: all clean help qemu-hart0-mmode-help qemu-hart0-mmode-prompt cmake build hart0 hart0-qemu-preload hart0-direct-mmode opensbi-setup opensbi-fw-dynamic opensbi-fw-jump hart1 hart2 hart3 hart4 apps apps-clean build-all qemu qemu-fast qemu-linux-amp qemu-hart0-loader qemu-hart0-preload qemu-hart0-preload-run qemu-hart0-mmode qemu-hart0-mmode-run amp-clean-build dev-env bootstrap-env setup-all verify-toolchain verify-toolchain-full wsl-setup-help zephyr-setup zephyr-hart1 zephyr-hart1-fast zephyr-hart1-hw freertos-setup freertos-hart2 freertos-hart2-smode buildroot-setup buildroot-linux buildroot-linux-fast apps-zephyr apps-zephyr-fast apps-zephyr-hw build-all-zephyr build-all-zephyr-fast build-all-zephyr-hw qemu-zephyr

# Default target
all: cmake build

# Display help
help:
	@echo "RISC-V HiFive Unmatched Multi-Hart Build System"
	@echo "================================================"
	@echo ""
	@echo "Targets:"
	@echo "  make cmake      - Generate CMake build configuration"
	@echo "  make build      - Build all hart images (hart0-4)"
	@echo "  make hart0      - Build Hart 0 (Bare-metal Boot Coordinator)"
	@echo "  make hart0-qemu-preload - Build Hart 0 with QEMU preload mode enabled"
	@echo "  make hart0-direct-mmode - Build Hart 0 for direct M-mode CLINT/MSIP release"
	@echo "  make opensbi-fw-dynamic - Build OpenSBI FW_DYNAMIC for the Hart4 Linux bridge"
	@echo "  make opensbi-fw-jump - Build OpenSBI FW_JUMP for the Hart4 Linux bridge"
	@echo "  make hart1      - Build Hart 1 (Zephyr RTOS)"
	@echo "  make hart2      - Build Hart 2 (FreeRTOS)"
	@echo "  make hart3      - Build Hart 3 (Bare-metal App 1)"
	@echo "  make hart4      - Build Hart 4 (Bare-metal App 2)"
	@echo "  make apps       - Build app1..app4 from apps/* folders"
	@echo "  make apps-zephyr - Build app1 from Zephyr + app2..app4"
	@echo "  make apps-clean - Clean app1..app4 build artifacts"
	@echo "  make build-all  - Build Hart 0 and app1..app4"
	@echo "  make qemu       - Build apps and start all harts directly in QEMU"
	@echo "  make qemu-fast  - Incremental Zephyr rebuild, then start all harts directly in QEMU"
	@echo "  make qemu-linux-amp - Launch Linux on Hart 0 with Zephyr/FreeRTOS/BM peers on Harts 1-3"
	@echo "  make qemu-hart0-loader - Boot Hart0 in QEMU and wait for UART payloads over PTY"
	@echo "  make qemu-hart0-preload - Boot Hart0 in QEMU with peer payloads preloaded in RAM"
	@echo "  make qemu-hart0-preload-run - Run the preload QEMU flow without rebuilding artifacts"
	@echo "  make qemu-hart0-mmode - Boot Hart0 in direct M-mode and release Zephyr/FreeRTOS/BM peers"
	@echo "  make qemu-hart0-mmode-run - Run the direct M-mode QEMU flow without rebuilding artifacts"
	@echo "  make qemu-hart0-mmode-help - Show the numbered 'what changed?' prompt for fast reruns"
	@echo "  make qemu-hart0-mmode-prompt - Ask what changed, then run the matching command"
	@echo "  make amp-clean-build - Clean rebuild Hart0, Zephyr, FreeRTOS, bare-hart3, Linux, and OpenSBI bridge"
	@echo "  make dev-env    - Install host tools needed for Zephyr/FreeRTOS/QEMU development"
	@echo "  make bootstrap-env - One-file install + dependency init + verification flow"
	@echo "  make setup-all  - Install host tools, initialize all dependencies, and verify the setup"
	@echo "  make verify-toolchain - Verify required tools and dependency workspaces"
	@echo "  make verify-toolchain-full - Verify tools/deps and run smoke builds"
	@echo "  make wsl-setup-help - Show the Windows PowerShell helper for WSL Ubuntu + QEMU setup"
	@echo "  make opensbi-setup - Initialize the OpenSBI dependency in deps/opensbi"
	@echo "  make zephyr-setup - Initialize Zephyr workspace in deps/zephyr"
	@echo "  make zephyr-hart1 - Build Zephyr app for Hart 1 for QEMU"
	@echo "  make zephyr-hart1-fast - Incremental Zephyr build for Hart 1 for QEMU"
	@echo "  make zephyr-hart1-hw - Build Zephyr app for Hart 1 for real HiFive Unmatched HW"
	@echo "  make freertos-setup - Initialize FreeRTOS workspace in deps/freertos"
	@echo "  make freertos-hart2 - Build Hart 2 through the FreeRTOS workspace path"
	@echo "  make freertos-hart2-smode - Build Hart 2 OpenSBI S-mode bring-up variant"
	@echo "  make buildroot-setup - Initialize Buildroot workspace in deps/buildroot"
	@echo "  make buildroot-linux - Build Linux AMP artifacts through Buildroot with a sanitized PATH"
	@echo "  make buildroot-linux-fast - Rebuild only the Linux app/rootfs image using the existing Buildroot output"
	@echo "  make build-all-zephyr - Build Hart0 + Zephyr app1 + app2..app4"
	@echo "  make qemu-zephyr - build-all-zephyr then run QEMU"
	@echo "  make clean      - Clean build artifacts"
	@echo ""

qemu-hart0-mmode-help:
	@echo "Linux AMP / direct M-mode quick prompt"
	@echo "======================================"
	@echo ""
	@echo "Pick the smallest rebuild that matches what changed:"
	@echo "  1. Clean build all + run"
	@echo "     make qemu-hart0-mmode"
	@echo ""
	@echo "  2. APP1 (Zephyr app) changed"
	@echo "     make zephyr-hart1-fast"
	@echo "     make qemu-hart0-mmode-run"
	@echo ""
	@echo "  3. APP2 (FreeRTOS app) changed"
	@echo "     make -C apps/freertos-hart2 app2 FREERTOS_KERNEL_DIR=$$(pwd)/deps/freertos/FreeRTOS-Kernel"
	@echo "     make qemu-hart0-mmode-run"
	@echo ""
	@echo "  4. APP3 (bare-metal hart3 app) changed"
	@echo "     make -C apps/bare-hart3 app3"
	@echo "     make qemu-hart0-mmode-run"
	@echo ""
	@echo "  5. Linux / Buildroot app changed"
	@echo "     make buildroot-linux-fast"
	@echo "     make qemu-hart0-mmode-run"
	@echo ""
	@echo "  6. Hart0 / OpenSBI / memory layout / boot flow changed"
	@echo "     make qemu-hart0-mmode"
	@echo ""
	@echo "  7. Nothing changed, just rerun QEMU"
	@echo "     make qemu-hart0-mmode-run"
	@echo ""

qemu-hart0-mmode-prompt:
	@echo "Linux AMP / direct M-mode quick prompt"
	@echo "======================================"
	@echo ""
	@echo "  1. Clean build all + run"
	@echo "  2. APP1 (Zephyr app) changed"
	@echo "  3. APP2 (FreeRTOS app) changed"
	@echo "  4. APP3 (bare-metal hart3 app) changed"
	@echo "  5. Linux / Buildroot app changed"
	@echo "  6. Hart0 / OpenSBI / memory layout / boot flow changed"
	@echo "  7. Nothing changed, just rerun QEMU"
	@echo ""
	@read -p "Enter choice [1-7]: " choice; \
	case "$$choice" in \
		1) echo "Running: make qemu-hart0-mmode"; $(MAKE) qemu-hart0-mmode ;; \
		2) echo "Running: make zephyr-hart1-fast && make qemu-hart0-mmode-run"; $(MAKE) zephyr-hart1-fast && $(MAKE) qemu-hart0-mmode-run ;; \
		3) echo "Running: make -C apps/freertos-hart2 app2 FREERTOS_KERNEL_DIR=$(CURDIR)/deps/freertos/FreeRTOS-Kernel && make qemu-hart0-mmode-run"; \
			$(MAKE) -C apps/freertos-hart2 app2 FREERTOS_KERNEL_DIR=$(CURDIR)/deps/freertos/FreeRTOS-Kernel && $(MAKE) qemu-hart0-mmode-run ;; \
		4) echo "Running: make -C apps/bare-hart3 app3 && make qemu-hart0-mmode-run"; $(MAKE) -C apps/bare-hart3 app3 && $(MAKE) qemu-hart0-mmode-run ;; \
		5) echo "Running: make buildroot-linux-fast && make qemu-hart0-mmode-run"; $(MAKE) buildroot-linux-fast && $(MAKE) qemu-hart0-mmode-run ;; \
		6) echo "Running: make qemu-hart0-mmode"; $(MAKE) qemu-hart0-mmode ;; \
		7) echo "Running: make qemu-hart0-mmode-run"; $(MAKE) qemu-hart0-mmode-run ;; \
		*) echo "Invalid choice: $$choice"; echo "Use a number from 1 to 7."; exit 1 ;; \
	esac

# Generate CMake build system
cmake:
	@mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release ..

# Build all targets
build:
	cd build && cmake --build . --config Release

# Build individual harts
hart0:
	$(MAKE) -f boot/hart0/Makefile hart0

hart0-qemu-preload:
	$(MAKE) -f boot/hart0/Makefile clean hart0 EXTRA_CFLAGS=-DHART0_QEMU_PRELOAD

hart0-direct-mmode:
	$(MAKE) -f boot/hart0/Makefile clean hart0 EXTRA_CFLAGS="-DHART0_DIRECT_MMODE -DHART0_QEMU_PRELOAD"

opensbi-fw-dynamic:
	FW_MODE=dynamic bash ./tools/scripts/build_opensbi_amp.sh

opensbi-fw-jump:
	FW_MODE=jump bash ./tools/scripts/build_opensbi_amp.sh

hart1: cmake
	cd build && cmake --build . --target hart1.elf

hart2: cmake
	cd build && cmake --build . --target hart2.elf

hart3: cmake
	cd build && cmake --build . --target hart3.elf

hart4: cmake
	cd build && cmake --build . --target hart4.elf

# Build all app binaries used by Hart0 serial loader
apps:
	for d in apps/zephyr-hart1 apps/freertos-hart2 apps/bare-hart3 apps/bare-hart4; do \
		(cd $$d && make clean && make); \
	done

apps-clean:
	for d in apps/zephyr-hart1 apps/freertos-hart2 apps/bare-hart3 apps/bare-hart4; do \
		(cd $$d && make clean); \
	done

dev-env:
	bash ./tools/scripts/setup_dev_env.sh --with-deps

bootstrap-env:
	bash ./tools/scripts/bootstrap_env.sh

setup-all:
	bash ./tools/scripts/setup_all.sh --verify-full

verify-toolchain:
	bash ./tools/scripts/verify_toolchain.sh

verify-toolchain-full:
	bash ./tools/scripts/verify_toolchain.sh --smoke-builds --buildroot-defconfig

wsl-setup-help:
	@echo "Run this from Windows PowerShell as Administrator:"
	@echo "  powershell -ExecutionPolicy Bypass -File tools/scripts/setup_wsl_ubuntu_qemu.ps1"

opensbi-setup:
	bash ./tools/scripts/setup_opensbi_deps.sh

zephyr-setup:
	bash ./tools/scripts/setup_zephyr_deps.sh

zephyr-hart1:
	ZEPHYR_HART1_TARGET=qemu bash ./tools/scripts/build_zephyr_hart1.sh

zephyr-hart1-fast:
	ZEPHYR_HART1_TARGET=qemu ZEPHYR_PRISTINE=never bash ./tools/scripts/build_zephyr_hart1.sh

zephyr-hart1-hw:
	ZEPHYR_HART1_TARGET=hw bash ./tools/scripts/build_zephyr_hart1.sh

freertos-setup:
	bash ./tools/scripts/setup_freertos_deps.sh

buildroot-setup:
	bash ./tools/scripts/setup_buildroot_deps.sh

buildroot-linux:
	bash ./tools/scripts/build_buildroot_linux_amp.sh

buildroot-linux-fast:
	bash ./tools/scripts/build_buildroot_linux_amp_fast.sh

apps-zephyr: zephyr-hart1
	for d in apps/freertos-hart2 apps/bare-hart3 apps/bare-hart4; do \
		(cd $$d && make clean && make); \
	done

apps-zephyr-fast: zephyr-hart1-fast
	for d in apps/freertos-hart2 apps/bare-hart3 apps/bare-hart4; do \
		(cd $$d && make clean && make); \
	done

apps-zephyr-hw: zephyr-hart1-hw
	for d in apps/freertos-hart2 apps/bare-hart3 apps/bare-hart4; do \
		(cd $$d && make clean && make); \
	done

# Convenience target: Hart0 + all apps
build-all: hart0 apps

build-all-zephyr: hart0 apps-zephyr

build-all-zephyr-fast: hart0 apps-zephyr-fast

build-all-zephyr-hw: hart0 apps-zephyr-hw

# use qemu-fast after editing Hart 1 source like main.c
# use regular qemu if you changed Zephyr config or memory overlay, like app.overlay or prj.conf

qemu: apps-zephyr
	bash ./tools/scripts/run_qemu_multihart.sh

qemu-zephyr: apps-zephyr
	bash ./tools/scripts/run_qemu_multihart.sh

qemu-fast: apps-zephyr-fast
	bash ./tools/scripts/run_qemu_multihart.sh

qemu-linux-amp: hart0 zephyr-hart1 freertos-hart2
	for d in apps/bare-hart3; do \
		(cd $$d && make clean && make); \
	done
	bash ./tools/scripts/run_qemu_linux_amp.sh

qemu-hart0-loader: hart0
	bash ./tools/scripts/run_qemu_hart0_loader.sh

qemu-hart0-preload: hart0-qemu-preload zephyr-hart1 freertos-hart2 buildroot-linux
	for d in apps/bare-hart3; do \
		(cd $$d && make clean && make); \
	done
	bash ./tools/scripts/run_qemu_hart0_preload.sh

qemu-hart0-preload-run:
	bash ./tools/scripts/run_qemu_hart0_preload.sh

qemu-hart0-mmode: hart0-direct-mmode apps-zephyr buildroot-linux opensbi-fw-jump
	bash ./tools/scripts/run_qemu_hart0_mmode.sh

qemu-hart0-mmode-run:
	bash ./tools/scripts/run_qemu_hart0_mmode.sh

amp-clean-build: hart0-direct-mmode zephyr-hart1 buildroot-linux opensbi-fw-jump
	make -C apps/freertos-hart2 clean app2 FREERTOS_KERNEL_DIR=$(CURDIR)/deps/freertos/FreeRTOS-Kernel
	make -C apps/bare-hart3 clean app3

# Clean build directory
clean:
	rm -rf build/
