.section .text.init
.global _start
.extern main
.extern freertos_risc_v_trap_handler
.extern __bss_start
.extern __bss_end
.extern __stack_top

_start:
    la sp, __stack_top
    la t0, freertos_risc_v_trap_handler
    csrw mtvec, t0

    la t0, __bss_start
    la t1, __bss_end
1:
    bgeu t0, t1, 2f
    sd zero, 0(t0)
    addi t0, t0, 8
    j 1b
2:
    call main
3:
    wfi
    j 3b
