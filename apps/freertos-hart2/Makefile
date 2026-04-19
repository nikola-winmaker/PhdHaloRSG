# Hart 2 FreeRTOS app build.

ROOT_DIR := $(abspath ../..)
USE_HALO ?= 0

ifeq ($(USE_HALO),1)
	DEPS_DIR := deps/halo/codegen/riscv64_h2_freertos
	DEPS_INC := -I$(DEPS_DIR)/include
	DEPS_SRCS := $(wildcard $(DEPS_DIR)/src/*.c)
else
	DEPS_DIR := deps/classical
	DEPS_INC := -I$(DEPS_DIR)/include
	DEPS_SRCS := $(wildcard $(DEPS_DIR)/src/*.c)
endif

DEPS_INC += -Isrc -I$(ROOT_DIR)/src/common -I$(ROOT_DIR)/src/battery -I$(ROOT_DIR)/src/bms
COMMON_SRCS := $(wildcard $(ROOT_DIR)/src/common/*.c) \
	$(wildcard $(ROOT_DIR)/src/battery/*.c) \
	$(wildcard $(ROOT_DIR)/src/bms/*.c)

FREERTOS_KERNEL_DIR ?= $(ROOT_DIR)/deps/freertos/FreeRTOS-Kernel
OPEN_SBI_SMODE ?= 0
AMP_LAYOUT_TOOL := $(ROOT_DIR)/tools/scripts/amp_layout.py

CROSS_COMPILE ?= riscv64-unknown-elf-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

PORT_DIR := $(FREERTOS_KERNEL_DIR)/portable/GCC/RISC-V
PORT_EXT_DIR := $(PORT_DIR)/chip_specific_extensions/RISCV_MTIME_CLINT_no_extensions
HEAP_DIR := $(FREERTOS_KERNEL_DIR)/portable/MemMang

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
GEN_DIR := $(BUILD_DIR)/generated
LINKER_SCRIPT := $(GEN_DIR)/linker.ld

ifeq ($(OPEN_SBI_SMODE),1)
APP_SOURCES := src/main_smode.c src/start_smode.S $(DEPS_SRCS) $(COMMON_SRCS)
KERNEL_SOURCES :=
else
APP_SOURCES := src/main.c src/faults.c src/libc.c src/start.S $(DEPS_SRCS) $(COMMON_SRCS)
KERNEL_SOURCES := \
  $(FREERTOS_KERNEL_DIR)/tasks.c \
  $(FREERTOS_KERNEL_DIR)/list.c \
  $(FREERTOS_KERNEL_DIR)/queue.c \
  $(PORT_DIR)/port.c \
  $(PORT_DIR)/portASM.S \
  $(HEAP_DIR)/heap_4.c
endif

SOURCES := $(APP_SOURCES) $(KERNEL_SOURCES)
OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(filter %.c,$(SOURCES))) \
	$(patsubst %.S,$(OBJ_DIR)/%.o,$(filter %.S,$(SOURCES)))

ARCH_FLAGS := -march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany -msmall-data-limit=0

CFLAGS := $(ARCH_FLAGS) \
	-nostdlib -ffreestanding -fno-builtin -Wall -Wextra -g \
	-I. -Isrc $(DEPS_INC) -I$(FREERTOS_KERNEL_DIR)/include -I$(PORT_DIR) -DUSE_HALO=$(USE_HALO)

ifeq ($(OPEN_SBI_SMODE),1)
CFLAGS += -DOPEN_SBI_SMODE
endif

ASFLAGS := $(CFLAGS) -I$(PORT_EXT_DIR)
LDFLAGS := -nostdlib -Wl,-T,$(LINKER_SCRIPT) -Wl,-Map,$(BIN_DIR)/app2.map
LDLIBS := -lgcc

.PHONY: all app2 clean

all: app2

app2: $(BIN_DIR)/app2.elf $(BIN_DIR)/app2.bin

$(LINKER_SCRIPT): $(AMP_LAYOUT_TOOL) $(ROOT_DIR)/config/amp_layout.json
	@mkdir -p $(GEN_DIR)
	python3 $(AMP_LAYOUT_TOOL) freertos-linker > $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BIN_DIR)/app2.elf: $(OBJECTS) $(LINKER_SCRIPT)
	@mkdir -p $(BIN_DIR)
	$(CC) $(ARCH_FLAGS) $(OBJECTS) $(LDFLAGS) -o $@ $(LDLIBS)
	@$(OBJDUMP) -h $@

$(BIN_DIR)/app2.bin: $(BIN_DIR)/app2.elf
	$(OBJCOPY) -O binary $< $@

clean:
	-rm -rf $(BUILD_DIR)
