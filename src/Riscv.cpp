//
// Created by jelena on 8/12/26.
//

#include "../h/Riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/TCB.hpp"
#include "../lib/console.h"

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
        uint64 mask = 1UL << 1;

        // Brišemo zahtev za prekid
        __asm__ volatile("csrc sip, %0" : : "r"(mask));

        // Ne menjamo vrednost registra a0
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
    else if (code == 0x41) {
        // Učitavanje znaka preko gotove biblioteke
        result = (uint64)__getc();
    }
    else if (code == 0x42) {
        // Ispis znaka preko gotove biblioteke
        __putc((char)argument1);
        result = 0;
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