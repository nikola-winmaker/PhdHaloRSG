# App 3 Makefile (built from apps/bare-hart3)
CROSS_COMPILE ?= riscv64-unknown-elf-
ROOT_DIR := $(abspath ../..)
AMP_LAYOUT_TOOL := $(ROOT_DIR)/tools/scripts/amp_layout.py
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

CFLAGS = -march=rv64imafdc -mabi=lp64d -mcmodel=medany -nostdlib -nostartfiles -ffreestanding -Wall -g
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

$(BUILD_DIR)/src/main.o: src/main.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/uart.o: src/uart.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/app3.elf: $(BUILD_DIR)/src/start.o $(BUILD_DIR)/src/main.o $(BUILD_DIR)/src/uart.o $(LINKER_SCRIPT)
	@mkdir -p $(BIN_DIR)
	$(LD) $(LDFLAGS) $(BUILD_DIR)/src/start.o $(BUILD_DIR)/src/main.o $(BUILD_DIR)/src/uart.o -o $@
	@echo "✓ Built App3 ELF"
	@$(OBJDUMP) -h $@

$(BIN_DIR)/app3.bin: $(BIN_DIR)/app3.elf
	$(OBJCOPY) -O binary $< $@
	@echo "✓ Built App3 Binary"

clean:
	rm -rf $(BUILD_DIR)
