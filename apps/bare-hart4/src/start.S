.section .text.init
.global _start

_start:
    la sp, _stack_start
    jal _start_c
    j .

.section .bss
.align 16
_stack_start:
    .space 4096
