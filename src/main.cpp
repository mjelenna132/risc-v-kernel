#include "../h/Riscv.hpp"
#include "../h/TCB.hpp"
#include "../h/syscall_c.hpp"
#include "../h/KernelConsole.hpp"
#include "../lib/hw.h"

extern void userMain();

static void userMainWrapper(void*)
{
    userMain();
}

int main()
{
    // Inicijalizaciju obavljamo sa isključenim prekidima.
    asm volatile("csrci sstatus, 2" ::: "memory");

    Riscv::writeStvec((uint64)&supervisorTrap);

    // TCB početne sistemske main niti.
    _thread::initialize();

    // Pripremamo bafere, semafore i izlaznu sistemsku nit.
    if (KernelConsole::initialize() < 0) {
        *(volatile uint32*)0x100000 = 0x5555;
        return -1;
    }

    thread_t userThread = nullptr;

    if (thread_create(
            &userThread,
            userMainWrapper,
            nullptr
        ) < 0) {

        *(volatile uint32*)0x100000 = 0x5555;
        return -1;
        }

    // Čekamo sve korisničke niti, uključujući one
    // koje je userMain napravio pre svog završetka.
    while (_thread::hasActiveUserThreads()) {
        // Omogućavamo prekide i kada samo main ostane spreman.
        asm volatile("csrsi sstatus, 2" ::: "memory");

        thread_dispatch();

        // Provere stanja ponovo radimo sa isključenim prekidima.
        asm volatile("csrci sstatus, 2" ::: "memory");
    }

    // Čeka da se izlazni bafer isprazni i da kontroler potvrdi da
    // je poslednji znak zaista preuzet, a zatim gasi emulator.
    Riscv::haltMachine();

    return 0;
}