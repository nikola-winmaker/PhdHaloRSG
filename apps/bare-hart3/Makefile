# App 3 Makefile (built from apps/bare-hart3)
CROSS_COMPILE ?= riscv64-unknown-elf-
ROOT_DIR := $(abspath ../..)
AMP_LAYOUT_TOOL := $(ROOT_DIR)/tools/scripts/amp_layout.py
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

USE_HALO ?= 0

ifeq ($(USE_HALO),1)
	DEPS_DIR := deps/halo/codegen/riscv64_h3_baremetal
	DEPS_INC := -I$(DEPS_DIR)/include
	DEPS_SRCS := $(wildcard $(DEPS_DIR)/src/*.c)
	DEPS_OBJDIR := halo_codegen
else
	DEPS_DIR := deps/classical
	DEPS_INC := -I$(DEPS_DIR)/include
	DEPS_SRCS := $(wildcard $(DEPS_DIR)/src/*.c)
	DEPS_OBJDIR := classical
endif

DEPS_INC += -Isrc -I$(ROOT_DIR)/src -I$(ROOT_DIR)/src/common
COMMON_SRCS := $(wildcard $(ROOT_DIR)/src/common/*.c)

CFLAGS = -march=rv64imafdc -mabi=lp64d -mcmodel=medany -nostdlib -nostartfiles -ffreestanding -Wall -g $(DEPS_INC) -DUSE_HALO=$(USE_HALO)
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
GEN_DIR = $(BUILD_DIR)/generated
LINKER_SCRIPT = $(GEN_DIR)/linker.ld
LDFLAGS = -T $(LINKER_SCRIPT) -nostdlib

.PHONY: all app3 clean

all: app3


app3: $(BIN_DIR)/app3.elf $(BIN_DIR)/app3.bin

$(LINKER_SCRIPT): $(AMP_LAYOUT_TOOL) $(ROOT_DIR)/config/amp_layout.json
	@mkdir -p $(GEN_DIR)
	python3 $(AMP_LAYOUT_TOOL) bare-linker > $@

$(BUILD_DIR)/src/start.o: src/start.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# Pattern rule for all .c files (main, uart, and codegen)
$(BUILD_DIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@



# Pattern rule for dependency .c files in src subdir
$(BUILD_DIR)/$(DEPS_OBJDIR)/src/%.o: $(DEPS_DIR)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for shared common sources
$(BUILD_DIR)/common/%.o: $(ROOT_DIR)/src/common/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

APP3_OBJS := $(BUILD_DIR)/src/main.o \
	$(patsubst $(DEPS_DIR)/src/%.c,$(BUILD_DIR)/$(DEPS_OBJDIR)/src/%.o,$(DEPS_SRCS)) \
	$(patsubst $(ROOT_DIR)/src/common/%.c,$(BUILD_DIR)/common/%.o,$(COMMON_SRCS))

$(BIN_DIR)/app3.elf: $(BUILD_DIR)/src/start.o $(APP3_OBJS) $(LINKER_SCRIPT)
	@mkdir -p $(BIN_DIR)
	$(LD) $(LDFLAGS) $(BUILD_DIR)/src/start.o $(APP3_OBJS) -o $@
	@echo "✓ Built App3 ELF"
	@$(OBJDUMP) -h $@

$(BIN_DIR)/app3.bin: $(BIN_DIR)/app3.elf
	$(OBJCOPY) -O binary $< $@
	@echo "✓ Built App3 Binary"

clean:
	rm -rf $(BUILD_DIR)
