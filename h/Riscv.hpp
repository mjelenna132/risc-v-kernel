//
// Created by jelena on 8/12/26.
//



#ifndef OS_PROJECT_RISCV_HPP
#define OS_PROJECT_RISCV_HPP

#include "../lib/hw.h"

// Početak prekidne rutine napisan u asembleru.
extern "C" void supervisorTrap();

// C++ deo koji obrađuje sistemski poziv.
extern "C" uint64 handleSupervisorTrap(
    uint64 code,
    uint64 argument1,
    uint64 argument2,
    uint64 argument3,
    uint64 argument4
);


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

    static uint64 readSstatus() {
        uint64 value;
        asm volatile("csrr %0, sstatus" : "=r"(value));
        return value;
    }

    static void writeSstatus(uint64 value) {
        asm volatile("csrw sstatus, %0" : : "r"(value));
    }
    // Prelazak iz sistemskog u korisnički režim
    static void popSppSpie() __attribute__((naked));

    // Čeka da se izlazni bafer konzole isprazni, a zatim trajno
    // zaustavlja emulator RISC-V procesora. Koristi se i za
    // regularno gašenje jezgra i za gašenje nakon fatalne greške.
    static void haltMachine();
};


#endif // OS_PROJECT_RISCV_HPP

