//
// Created by jelena on 8/12/26.
//
#include "../h/Riscv.hpp"
#include "../h/MemoryAllocator.hpp"

extern "C" uint64 handleSupervisorTrap(uint64 code, uint64 argument) {
    uint64 cause = Riscv::readScause();

    // 8 je ecall iz korisničkog, a 9 iz sistemskog režima.
    if (cause == 8 || cause == 9) {
        // Prelazimo preko instrukcije ecall.
        uint64 sepc = Riscv::readSepc();
        Riscv::writeSepc(sepc + 4);

        if (code == 0x01) {
            // argument je broj blokova.
            return (uint64)MemoryAllocator::mem_alloc(argument);
        }

        if (code == 0x02) {
            // argument je pokazivač na memoriju.
            return (uint64)MemoryAllocator::mem_free((void*)argument);
        }
    }

    return (uint64)-1;
}