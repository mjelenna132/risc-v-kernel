.section .text
.align 4

.global supervisorTrap
.type supervisorTrap, @function

supervisorTrap:
    # Čuvamo povratnu adresu i održavamo poravnanje steka.
    addi sp, sp, -16
    sd ra, 8(sp)

    # a0 i a1 već sadrže kod i argument sistemskog poziva.
    call handleSupervisorTrap

    # U a0 je sada povratna vrednost.
    ld ra, 8(sp)
    addi sp, sp, 16

    # Povratak na instrukciju čija je adresa u sepc.
    sret

.size supervisorTrap, .-supervisorTrap