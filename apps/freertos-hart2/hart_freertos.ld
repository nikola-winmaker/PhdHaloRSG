/**
 * Linker script for Hart 2 (U74 - FreeRTOS)
 * Base: 0x80600000, Size: 5 MB
 */

MEMORY
{
  DRAM (rwx) : ORIGIN = 0x80600000, LENGTH = 5M
}

SECTIONS
{
  . = 0x80600000;

  /* Entry point and early setup */
  .text.init : {
    *(.text.init)
    *(.text .text.*)
    *(.rodata .rodata.*)
  } >DRAM

  . = ALIGN(0x1000);

  /* Initialized data */
  .data : {
    *(.data .data.*)
    *(.sdata .sdata.*)
  } >DRAM

  /* Global pointer for small data access */
  . = ALIGN(0x10);
  .sdata : {
    __global_pointer$ = . + 0x800;
    *(.sdata .sdata.*)
  } >DRAM

  /* Uninitialized data */
  .bss : {
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    *(COMMON)
  } >DRAM

  /* Stack (grows downward from end) */
  .stack : {
    . = ALIGN(0x1000);
    _stack_end = .;
    . += 16K;
    _stack_start = .;
  } >DRAM

  /* Heap space for FreeRTOS allocations */
  .heap : {
    . = ALIGN(0x1000);
    _heap_start = .;
    . += 256K;
    _heap_end = .;
  } >DRAM

  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
}

_end = .;
