MEMORY {
  APP4_RAM (rwx) : ORIGIN = 0x80300000, LENGTH = 0x40000
}

ENTRY(_start)

SECTIONS {
  .text.init : { *(.text.init) } > APP4_RAM AT > APP4_RAM
  .text : { *(.text) *(.text.*) } > APP4_RAM AT > APP4_RAM
  .rodata : { *(.rodata) *(.rodata.*) } > APP4_RAM AT > APP4_RAM
  .data : { *(.data) *(.data.*) } > APP4_RAM AT > APP4_RAM
  .bss : { *(.bss) *(.bss.*) *(COMMON) } > APP4_RAM AT > APP4_RAM
  .stack : { . = ALIGN(16); } > APP4_RAM
}
