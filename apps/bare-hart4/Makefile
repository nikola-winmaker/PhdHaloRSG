# App 4 Makefile (built from apps/bare-hart4)
CROSS_COMPILE ?= riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

CFLAGS = -march=rv64imafdc -mabi=lp64d -mcmodel=medany -nostdlib -nostartfiles -ffreestanding -Wall -g
LDFLAGS = -T linker.ld -nostdlib

BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin

.PHONY: all app4 clean

all: app4

app4: $(BIN_DIR)/app4.elf $(BIN_DIR)/app4.bin

$(BUILD_DIR)/src/start.o: src/start.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/main.o: src/main.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/uart.o: src/uart.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/app4.elf: $(BUILD_DIR)/src/start.o $(BUILD_DIR)/src/main.o $(BUILD_DIR)/src/uart.o
	@mkdir -p $(BIN_DIR)
	$(LD) $(LDFLAGS) $^ -o $@
	@echo "✓ Built App4 ELF"
	@$(OBJDUMP) -h $@

$(BIN_DIR)/app4.bin: $(BIN_DIR)/app4.elf
	$(OBJCOPY) -O binary $< $@
	@echo "✓ Built App4 Binary"

clean:
	rm -rf $(BUILD_DIR)
