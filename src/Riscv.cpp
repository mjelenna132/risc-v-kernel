//
// Created by jelena on 8/12/26.
//

#include "../h/Riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/TCB.hpp"
#include "../lib/console.h"
#include "../h/Semaphore.hpp"

extern "C" uint64 handleSupervisorTrap(
    uint64 code,
    uint64 argument1,
    uint64 argument2,
    uint64 argument3,
    uint64 argument4
) {
    uint64 cause = Riscv::readScause();

    // Prekid od tajmera
    if (cause == 0x8000000000000001UL) {
        // Čuvamo mesto na kome prekinuta nit treba da nastavi.
        uint64 volatile sepc = Riscv::readSepc();
        uint64 volatile sstatus = Riscv::readSstatus();

        uint64 mask = 1UL << 1;

        // Brišemo zahtev za tajmerski prekid.
        __asm__ volatile(
            "csrc sip, %0"
            :
            : "r"(mask)
        );

        // Ažuriramo uspavane niti i vremenski odsečak.
        // Ovde može doći do promene tekuće niti.
        // SPP je 0 ako je prekid stigao iz korisničkog režima.
        bool fromUserMode = (sstatus & (1UL << 8)) == 0;

        _thread::timerTick(fromUserMode);

        // Kada se ova nit ponovo izabere, vraćamo njene podatke.
        Riscv::writeSstatus(sstatus);
        Riscv::writeSepc(sepc);

        // Tajmerski prekid ne menja prethodnu vrednost registra a0.
        return code;
    }

    // Prekid od konzole
    if (cause == 0x8000000000000009UL) {
        console_handler();

        // Ne menjamo vrednost registra a0
        return code;
    }

    // Obrađujemo sistemske pozive iz korisničkog i sistemskog režima
    if (cause != 8 && cause != 9) {
        return code;
    }

    // Posle ecall nastavljamo od sledeće instrukcije
    uint64 sepc = Riscv::readSepc() + 4;
    uint64 sstatus = Riscv::readSstatus();

    uint64 result = 0;

    if (code == 0x01) {
        result = (uint64)
            MemoryAllocator::mem_alloc(argument1);
    }
    else if (code == 0x02) {
        result = (uint64)
            MemoryAllocator::mem_free((void*)argument1);
    }
    else if (code == 0x11) {
        result = (uint64)_thread::create(
            (_thread**)argument1,
            (_thread::Body)argument2,
            (void*)argument3,
            (uint64*)argument4
        );
    }
    else if (code == 0x12) {
        result = (uint64)_thread::exit();
    }
    else if (code == 0x13) {
        _thread::dispatch();
        result = 0;
    }
    else if (code == 0x31) {
        // argument1 sadrži broj perioda spavanja iz registra a1.
        result = (uint64)_thread::sleep(argument1);
    }
    else if (code == 0x41) {
        // Učitavanje znaka preko gotove biblioteke
        result = (uint64)__getc();
    }
    else if (code == 0x42) {
        // Ispis znaka preko gotove biblioteke
        __putc((char)argument1);
        result = 0;
    }
    else if (code == 0x21) {
        // sem_open(handle, init)
        result = (uint64)_sem::open(
            (_sem**)argument1,
            (unsigned)argument2
        );
    }
    else if (code == 0x22) {
        // sem_close(handle)
        result = (uint64)_sem::close(
            (_sem*)argument1
        );
    }
    else if (code == 0x23) {
        // sem_wait(handle)
        _sem* semaphore = (_sem*)argument1;

        result = semaphore != nullptr
            ? (uint64)semaphore->wait()
            : (uint64)-1;
    }
    else if (code == 0x24) {
        // sem_signal(handle)
        _sem* semaphore = (_sem*)argument1;

        result = semaphore != nullptr
            ? (uint64)semaphore->signal()
            : (uint64)-1;
    }
    else if (code == 0x25) {
        _sem* semaphore = (_sem*)argument1;

        if (semaphore == nullptr) {
            result = (uint64)-1;
        }
        else {
            result = (uint64)semaphore->waitN(
                (unsigned)argument2
            );
        }
    }
    else if (code == 0x26) {
        _sem* semaphore = (_sem*)argument1;

        if (semaphore == nullptr) {
            result = (uint64)-1;
        }
        else {
            result = (uint64)semaphore->signalN(
                (unsigned)argument2
            );
        }
    }
    else {
        result = (uint64)-1;
    }

    // Vraćamo podatke niti koja sada nastavlja izvršavanje
    Riscv::writeSstatus(sstatus);
    Riscv::writeSepc(sepc);

    return result;
}

void Riscv::popSppSpie()
{
    __asm__ volatile(
        // Nastavljamo iza poziva ove funkcije
        "csrw sepc, ra\n"

        // SPP = 0: korisnički režim
        "li t0, 0x100\n"
        "csrc sstatus, t0\n"

        // Dozvoljavamo prekide
        "li t0, 0x20\n"
        "csrs sstatus, t0\n"

        // Prelazimo u korisnički režim
        "sret\n"
    );
}