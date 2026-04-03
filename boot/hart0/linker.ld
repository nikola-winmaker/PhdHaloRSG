/* Linker script for Hart 0 (S7 core) - Boot Coordinator
   For QEMU virt machine with default OpenSBI firmware:
   - OpenSBI occupies 0x8000_0000+
   - Hart 0 payload is placed at 0x8020_0000
*/

MEMORY
{
    /* ROM (typically flash) */
    FLASH (rx)   : ORIGIN = 0x20000000, LENGTH = 512M
    
    /* RAM for Hart 0 payload (kept above OpenSBI) */
    HART0_RAM (rwx) : ORIGIN = 0x80200000, LENGTH = 256K
    
    /* ITIM (Instruction Tightly Integrated Memory) */
    ITIM (rx)    : ORIGIN = 0x08000000, LENGTH = 8K
    
    /* DTIM (Data Tightly Integrated Memory) */
    DTIM (rwx)   : ORIGIN = 0x0C000000, LENGTH = 16K
}

SECTIONS
{
    .text : {
        KEEP(*(.text.init))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > HART0_RAM
    
    _etext = .;
    
    .data : {
        _sdata = .;
        *(.data .data.*)
        _edata = .;
    } > HART0_RAM

    /* RISC-V global pointer anchor for small data accesses. */
    PROVIDE(__global_pointer$ = _sdata + 0x800);
    
    _sidata = _sdata;
    
    .bss : {
        _sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        _ebss = .;
    } > HART0_RAM
    
    /* Stack for Hart 0 */
    .stack : {
        . = ALIGN(16);
        _stack_end = .;
        . += 4K;
        _stack_start = .;
    } > HART0_RAM
    
    /DISCARD/ : { *(.note.GNU-stack) }
}
