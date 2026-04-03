/**
 * Hart 0 Startup Code (Assembly)
 * OpenSBI payload entry startup (conservative)
 */

/**
 * Hart 0 Startup Code (Assembly)
 * Bare-metal M-mode startup for Hart 0 coordinator
 */

.section .text.init, "ax"
.global _start
.align 2

#ifdef HART0_DIRECT_MMODE
.extern g_hart_release_flags
.extern g_hart_release_entries
.extern g_hart_release_arg0
.extern g_hart_release_arg1
.extern g_hart_release_arg2
#endif

_start:
    /* Set global pointer for small-data accesses on every hart. */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

#ifdef HART0_DIRECT_MMODE
    /* In direct M-mode boot, every hart enters this image first. */
    csrr t0, mhartid
    beqz t0, .Lboot_hart0

    /* Secondary harts wait here until Hart 0 assigns an entrypoint. */
    slli t1, t0, 2
    la t2, g_hart_release_flags
    add t2, t2, t1
    sw zero, 0(t2)

    slli t3, t0, 3
    la t4, g_hart_release_entries
    add t4, t4, t3
    la t5, g_hart_release_arg0
    add s0, t5, t3
    la t5, g_hart_release_arg1
    add s1, t5, t3
    la t5, g_hart_release_arg2
    add s2, t5, t3

.Lwait_release:
    lw t5, 0(t2)
    beqz t5, .Lpark_wait

    /* Ack our software interrupt before jumping into the payload. */
    li t6, 0x02000000
    add t6, t6, t1
    sw zero, 0(t6)
    fence rw, rw
    ld a0, 0(s0)
    ld a1, 0(s1)
    ld a2, 0(s2)
    li a3, 0
    li a4, 0
    ld t5, 0(t4)
    sw zero, 0(t2)
    fence.i
    jr t5

.Lpark_wait:
    nop
    j .Lwait_release

.Lboot_hart0:
    li a0, 0
    li a1, 0
#endif

    /* Set up stack pointer. */
    la sp, _stack_start

    /* Enter C code. */
    j _start_c

    /* If _start_c returns, park forever. */
.Lpark:
    wfi
    j .Lpark

.align 2
