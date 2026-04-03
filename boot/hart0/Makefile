# Hart 0 (S7) Coordinator Build
# Supports two boot modes:
#   1) OpenSBI payload mode using SBI HSM to start S-mode payloads
#   2) Direct M-mode mode where Hart 0 releases parked secondary harts via CLINT MSIP

CC	:= riscv64-unknown-elf-gcc
LD	:= riscv64-unknown-elf-gcc
#riscv64-unknown-elf-ld
AR	:= riscv64-unknown-elf-ar

# Directories
BOOT_DIR	:= boot/hart0
OPENSBI_DIR	:= ../../deps/opensbi
BUILD_DIR	:= $(BOOT_DIR)/build
BIN_DIR		:= $(BUILD_DIR)/bin
GEN_DIR		:= $(BUILD_DIR)/generated
AMP_LAYOUT_JSON  := config/amp_layout.json
AMP_LAYOUT_TOOL  := tools/scripts/amp_layout.py
AMP_LAYOUT_HDR   := $(GEN_DIR)/amp_layout.h

# Optimized for size + debugging
CFLAGS	:= -march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany
CFLAGS	+= -O2 -g -Wall
CFLAGS	+= -ffreestanding -fno-common
CFLAGS	+= -I.
CFLAGS	+= -Iinclude
CFLAGS	+= -Ideps/opensbi/include
CFLAGS	+= -I$(GEN_DIR)
CFLAGS	+= -DHIFIVE_UNMATCHED
CFLAGS	+= $(EXTRA_CFLAGS)

LDFLAGS	:= -static -nostdlib
#--gc-sections
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/hart0.map

# Source files for Hart 0
HART0_SRC	:= $(BOOT_DIR)/start.S \
		   $(BOOT_DIR)/main.c \
		   $(BOOT_DIR)/uart.c \
		   $(BOOT_DIR)/hart_boot.c \
		   $(BOOT_DIR)/partition.c \
		   $(BOOT_DIR)/ipc.c

HART0_OBJ	:= $(patsubst $(BOOT_DIR)/%.c,$(BUILD_DIR)/%.o,$(patsubst $(BOOT_DIR)/%.S,$(BUILD_DIR)/%.o,$(HART0_SRC)))

# Linker script
ifeq ($(findstring -DHART0_DIRECT_MMODE,$(EXTRA_CFLAGS)),-DHART0_DIRECT_MMODE)
HART0_LD	:= $(BOOT_DIR)/linker_mmode.ld
else
HART0_LD	:= $(BOOT_DIR)/linker.ld
endif
HART0_ELF	:= $(BIN_DIR)/hart0.elf
HART0_BIN	:= $(BIN_DIR)/hart0.bin

.PHONY: all clean hart0 distclean help

all: hart0

# Build Hart 0 ELF (standalone M-mode coordinator)
hart0: $(HART0_ELF)

$(AMP_LAYOUT_HDR): $(AMP_LAYOUT_JSON) $(AMP_LAYOUT_TOOL)
	@mkdir -p $(GEN_DIR)
	@python3 $(AMP_LAYOUT_TOOL) c-header > $@

# Build Hart 0 ELF - calls into OpenSBI via SBI ecalls
$(HART0_ELF): $(HART0_OBJ) $(HART0_LD)
	@mkdir -p $(BIN_DIR) $(BUILD_DIR)
	@echo "Linking Hart 0 coordinator (calls OpenSBI via SBI ecalls)..."
	$(LD) $(LDFLAGS) -T$(HART0_LD) \
		$(HART0_OBJ) \
		$(LDFLAGS) \
		-lgcc \
		-o $@
	@echo "Hart 0 ELF: $@"
	@riscv64-unknown-elf-size $@

$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.S $(AMP_LAYOUT_HDR)
	@mkdir -p $(dir $@)
	@echo "Assembling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.c $(AMP_LAYOUT_HDR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)

distclean: clean
	@cd $(OPENSBI_DIR) && make O=build clean || true

help:
	@echo "Hart 0 Coordinator Build"
	@echo "========================"
	@echo "Default build: OpenSBI payload using SBI HSM"
	@echo "Direct mode:  EXTRA_CFLAGS=-DHART0_DIRECT_MMODE for CLINT/MSIP M-mode release"
	@echo ""
	@echo "make hart0      - Build Hart 0 ELF"
	@echo "make clean      - Clean Hart 0 build artifacts"
	@echo "make distclean  - Clean Hart 0 + OpenSBI build dir"
