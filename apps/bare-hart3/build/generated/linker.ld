MEMORY {
  APP3_RAM (rwx) : ORIGIN = 0x802C0000, LENGTH = 0x00040000
}

ENTRY(_start)

SECTIONS {
  .text.init : { *(.text.init) } > APP3_RAM AT > APP3_RAM
  .text : { *(.text) *(.text.*) } > APP3_RAM AT > APP3_RAM
  .rodata : { *(.rodata) *(.rodata.*) } > APP3_RAM AT > APP3_RAM
  .data : { *(.data) *(.data.*) } > APP3_RAM AT > APP3_RAM
  .bss : { *(.bss) *(.bss.*) *(COMMON) } > APP3_RAM AT > APP3_RAM
  .stack : { . = ALIGN(16); } > APP3_RAM
}
