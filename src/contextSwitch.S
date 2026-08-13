.section .text
.align 4

.global contextSwitch
.type contextSwitch, @function

contextSwitch:
    # Čuvamo kontekst stare niti.
    sd ra, 0(a0)
    sd sp, 8(a0)

    # Učitavamo kontekst nove niti.
    ld ra, 0(a1)
    ld sp, 8(a1)

    ret

.size contextSwitch, .-contextSwitch