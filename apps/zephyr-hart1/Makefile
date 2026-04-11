# App 1 Makefile delegates to the Zephyr workspace build so root-level
# targets like `make qemu` build the same Hart 1 image as `make zephyr-hart1`.

ROOT_DIR := $(abspath ../..)
ZEPHYR_SCRIPT := $(ROOT_DIR)/tools/scripts/build_zephyr_hart1.sh
ZEPHYR_APP_BUILD_DIR := zephyr/build
APP_BUILD_DIR := build
USE_HALO ?= 0

ifeq ($(USE_HALO),1)
	DEPS_DIR := $(ROOT_DIR)/apps/zephyr-hart1/deps/halo/codegen/riscv64_h1_zephyr
else
	DEPS_DIR := $(ROOT_DIR)/apps/zephyr-hart1/deps/classical
endif

.PHONY: all app1 clean

all: app1

app1:
	@echo "[INFO] Zephyr Hart1 deps dir: $(DEPS_DIR)"
	USE_HALO=$(USE_HALO) $(ZEPHYR_SCRIPT)

clean:
	-rm -rf $(APP_BUILD_DIR) $(ZEPHYR_APP_BUILD_DIR)
