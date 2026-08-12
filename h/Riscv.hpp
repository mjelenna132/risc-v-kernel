//
// Created by jelena on 8/12/26.
//



#ifndef OS_PROJECT_RISCV_HPP
#define OS_PROJECT_RISCV_HPP

#include "../lib/hw.h"

// Početak prekidne rutine napisan u asembleru.
extern "C" void supervisorTrap();

// C++ deo koji obrađuje sistemski poziv.
extern "C" uint64 handleSupervisorTrap(uint64 code, uint64 argument);

class Riscv {
public:
    // Čita razlog prekida.
    static uint64 readScause() {
        uint64 value;
        asm volatile("csrr %0, scause" : "=r"(value));
        return value;
    }

    // Čita adresu instrukcije koja je izazvala prekid.
    static uint64 readSepc() {
        uint64 value;
        asm volatile("csrr %0, sepc" : "=r"(value));
        return value;
    }

    // Upisuje adresu nastavka izvršavanja.
    static void writeSepc(uint64 value) {
        asm volatile("csrw sepc, %0" : : "r"(value));
    }

    // Postavlja adresu prekidne rutine.
    static void writeStvec(uint64 value) {
        asm volatile("csrw stvec, %0" : : "r"(value));
    }
};

#endif // OS_PROJECT_RISCV_HPP

