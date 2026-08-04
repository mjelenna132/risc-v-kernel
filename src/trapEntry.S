.option norelax

# Okvir: svih 32 registra, pa sepc i sstatus. Svih 32 zato sto preotimanje moze
# da se desi i kad su zive vrednosti privremenih registara.
.equ .LFRAME_SIZE,      272
.equ .LFRAME_SEPC,      256
.equ .LFRAME_SSTATUS,   264

.equ .LSSTATUS_SPIE,    0x020
.equ .LSSTATUS_SPP,     0x100

.global trapEntry
.type trapEntry, @function
.align 4
trapEntry:
    addi sp, sp, -.LFRAME_SIZE
    .irp index, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    sd x\index, \index * 8(sp)
    .endr
    csrr t0, sepc
    sd   t0, .LFRAME_SEPC(sp)
    csrr t1, sstatus
    sd   t1, .LFRAME_SSTATUS(sp)

    mv a0, sp
    call handleSupervisorTrap

# sepc i sstatus se vracaju iz okvira, a NE iz istoimenih CSR-ova: obrada je u
# medjuvremenu mogla da promeni tekucu nit.
.global trapReturn
.type trapReturn, @function
trapReturn:
    ld t0, .LFRAME_SEPC(sp)
    csrw sepc, t0
    ld t1, .LFRAME_SSTATUS(sp)
    csrw sstatus, t1

    .irp index, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    ld x\index, \index * 8(sp)
    .endr
    addi sp, sp, .LFRAME_SIZE
    sret

# Prvo izvrsavanje nove niti. Na njenom steku se fabrikuje okvir prekidne rutine
# i kroz njega se "izlazi" u telo niti, kao da se nit vraca iz jezgra. Tako je
# prelazak u rezim rada niti sveden na jedno jedino sret u celom projektu.
.global startKernelThread
.type startKernelThread, @function
startKernelThread:
    li   t3, .LSSTATUS_SPP      # interna nit jezgra -> S-mod
    j    startThreadCommon

.global startUserThread
.type startUserThread, @function
startUserThread:
    li   t3, 0                  # korisnicka nit -> U-mod

startThreadCommon:
    addi sp, sp, -.LFRAME_SIZE

    # Okvir se prvo brise, inace bi smece sa neinicijalizovanog steka zavrsilo
    # u registrima nove niti (npr. tp, pa plic_claim puca).
    mv   t4, sp
    addi t5, sp, .LFRAME_SIZE
1:  sd   zero, 0(t4)
    addi t4, t4, 8
    bltu t4, t5, 1b

    sd   x2, 2 * 8(sp)          # sp nove niti
    sd   x4, 4 * 8(sp)          # tp = redni broj jezgra, isti za sve niti

    la   t0, threadWrapper
    sd   t0, .LFRAME_SEPC(sp)

    # SPP odredjuje rezim, SPIE postaje SIE posle sret: telo niti krece sa
    # dozvoljenim prekidima, da bi moglo da bude preoteto.
    csrr t1, sstatus
    li   t2, .LSSTATUS_SPP | .LSSTATUS_SPIE
    not  t2, t2
    and  t1, t1, t2
    or   t1, t1, t3
    ori  t1, t1, .LSSTATUS_SPIE
    sd   t1, .LFRAME_SSTATUS(sp)

    j    trapReturn
