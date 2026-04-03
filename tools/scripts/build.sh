#!/bin/bash
# HiFive Unmatched Multi-Hart Build & Flash Script
# Requires: CMake, RISC-V toolchain, OpenOCD

set -e  # Exit on error

PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)
BUILD_DIR="$PROJECT_ROOT/build"
BINARIES_DIR="$BUILD_DIR"

echo "=========================================="
echo "RISC-V HiFive Unmatched Build System"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

function print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

function print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

function print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check for required tools
function check_tools() {
    print_info "Checking prerequisites..."
    
    if ! command -v riscv64-unknown-elf-gcc &> /dev/null; then
        print_error "riscv64-unknown-elf-gcc not found. Install RISC-V toolchain."
        exit 1
    fi
    
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found. Install CMake 3.16+."
        exit 1
    fi
    
    print_info "Tools found: riscv64-unknown-elf-gcc, cmake"
}

# Configure CMake
function cmake_configure() {
    print_info "Configuring CMake build system..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake -DCMAKE_BUILD_TYPE=Release "$PROJECT_ROOT"
    cd "$PROJECT_ROOT"
    
    print_info "CMake configuration complete"
}

# Build target
function build_target() {
    local target=$1
    print_info "Building $target..."
    
    cd "$BUILD_DIR"
    cmake --build . --target "$target" --config Release
    cd "$PROJECT_ROOT"
    
    if [ -f "$BINARIES_DIR/$target" ]; then
        print_info "✓ $target built successfully"
        ls -lh "$BINARIES_DIR/$target"*
    else
        print_error "Build failed: $target not found"
        exit 1
    fi
}

# Flash all harts
function flash_all() {
    print_warn "Flash functionality requires OpenOCD and FTDI debugger"
    print_info "Flashing Hart 0 to 0x20000000..."
    
    # Example OpenOCD command (adjust for your setup)
    # openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg \
    #         -f board/sifive-hifive-unmatched.cfg \
    #         -c "init; flash write_image erase $BINARIES_DIR/hart0.bin 0x20000000; reset halt; exit"
    
    print_warn "Manual flashing required. Use OpenOCD or Segger tools."
}

# Main
function main() {
    local cmd=${1:-all}
    
    case "$cmd" in
        check)
            check_tools
            ;;
        cmake)
            check_tools
            cmake_configure
            ;;
        hart0|hart1|hart2|hart3|hart4)
            check_tools
            cmake_configure
            build_target "${cmd}.elf"
            ;;
        all)
            check_tools
            cmake_configure
            for hart in hart0.elf hart3.elf hart4.elf; do
                build_target "$hart"
            done
            print_info "Hart 0, 3, 4 built successfully"
            print_warn "Hart 1 (Zephyr) and Hart 2 (FreeRTOS) require separate setup"
            ;;
        flash)
            flash_all
            ;;
        clean)
            print_info "Cleaning build directory..."
            rm -rf "$BUILD_DIR"
            print_info "Clean complete"
            ;;
        *)
            echo "Usage: $0 {check|cmake|hart0|hart3|hart4|all|flash|clean}"
            exit 1
            ;;
    esac
}

main "$@"
