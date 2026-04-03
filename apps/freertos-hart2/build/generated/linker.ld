MEMORY {
  APP2_RAM (rwx) : ORIGIN = 0x80280000, LENGTH = 0x00040000
}

ENTRY(_start)

SECTIONS {
  . = ORIGIN(APP2_RAM);

  .text.init : {
    *(.text.init)
  } > APP2_RAM AT > APP2_RAM

  .text : {
    *(.text)
    *(.text.*)
  } > APP2_RAM AT > APP2_RAM

  .rodata : {
    *(.rodata)
    *(.rodata.*)
  } > APP2_RAM AT > APP2_RAM

  .data : {
    __data_start = .;
    *(.data)
    *(.data.*)
    __data_end = .;
  } > APP2_RAM AT > APP2_RAM

  .bss : {
    __bss_start = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    __bss_end = .;
  } > APP2_RAM AT > APP2_RAM

  .stack : {
    . = ALIGN(16);
    __stack_bottom = .;
    . += 0x4000;
    __stack_top = .;
  } > APP2_RAM

  .heap : {
    . = ALIGN(16);
    __heap_start = .;
    . = ORIGIN(APP2_RAM) + LENGTH(APP2_RAM);
    __heap_end = .;
  } > APP2_RAM
}
