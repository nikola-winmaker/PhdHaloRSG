# App 1 Makefile delegates to the Zephyr workspace build so root-level
# targets like `make qemu` build the same Hart 1 image as `make zephyr-hart1`.

ROOT_DIR := $(abspath ../..)
ZEPHYR_SCRIPT := $(ROOT_DIR)/tools/scripts/build_zephyr_hart1.sh
ZEPHYR_APP_BUILD_DIR := zephyr/build
APP_BUILD_DIR := build

.PHONY: all app1 clean

all: app1

app1:
	$(ZEPHYR_SCRIPT)

clean:
	-rm -rf $(APP_BUILD_DIR) $(ZEPHYR_APP_BUILD_DIR)
