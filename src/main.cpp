#include "../h/Riscv.hpp"
#include "../h/TCB.hpp"
#include "../h/syscall_c.hpp"
#include "../lib/hw.h"

extern void userMain();

static volatile bool userMainFinished = false;

// Ova funkcija pokreće javne testove u korisničkoj niti
static void userMainWrapper(void*)
{
    userMain();
    userMainFinished = true;
}

int main()
{
    // Postavljamo prekidnu rutinu
    Riscv::writeStvec((uint64)&supervisorTrap);

    // Glavna nit već postoji, pa samo pravimo njen TCB
    _thread::initialize();

    thread_t userThread = nullptr;

    // Pokrećemo javne testove kao korisničku nit
    if (thread_create(
            &userThread,
            userMainWrapper,
            nullptr
        ) < 0) {
        *(volatile uint32*)0x100000 = 0x5555;
        return -1;
        }

    // Main čeka da se userMain završi
    while (!userMainFinished) {
        thread_dispatch();
    }

    // Gasimo emulator
    *(volatile uint32*)0x100000 = 0x5555;

    return 0;
}