/* Linker script for direct M-mode Hart 0 firmware.
   In this mode QEMU/firmware boots Hart 0 directly in M-mode without OpenSBI.
   Keep the coordinator low in DRAM so app slots at 0x8024_0000+ stay unchanged.
*/

MEMORY
{
    FLASH (rx)        : ORIGIN = 0x20000000, LENGTH = 512M
    HART0_RAM (rwx)   : ORIGIN = 0x80000000, LENGTH = 256K
    ITIM (rx)         : ORIGIN = 0x08000000, LENGTH = 8K
    DTIM (rwx)        : ORIGIN = 0x0C000000, LENGTH = 16K
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

    PROVIDE(__global_pointer$ = _sdata + 0x800);

    _sidata = _sdata;

    .bss : {
        _sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        _ebss = .;
    } > HART0_RAM

    .stack : {
        . = ALIGN(16);
        _stack_end = .;
        . += 4K;
        _stack_start = .;
    } > HART0_RAM

    /DISCARD/ : { *(.note.GNU-stack) }
}
