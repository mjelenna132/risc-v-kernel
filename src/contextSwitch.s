# 0 "src/contextSwitch.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/riscv64-linux-gnu/include/stdc-predef.h" 1 3
# 0 "<command-line>" 2
# 1 "src/contextSwitch.S"
.section .text
.align 4

.global contextSwitch
.type contextSwitch, @function

contextSwitch:
    # Čuvamo kontekst stare niti.
    sd ra, 0(a0)
    # uint64 zauzima 8 bajtova
    sd sp, 8(a0)

    # Učitavamo kontekst nove niti.
    ld ra, 0(a1)
    ld sp, 8(a1)

    ret

.size contextSwitch, .-contextSwitch
