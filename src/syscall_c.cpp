//
// Created by jelena on 8/12/26.
//

#include "../h/syscall_c.hpp"

void* mem_alloc(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    // Pretvaranje bajtova u blokove, uz zaokruživanje naviše.
    size_t blockCount = size / MEM_BLOCK_SIZE;

    if (size % MEM_BLOCK_SIZE != 0) {
        blockCount++;
    }

    // a0 = kod poziva, a1 = argument.
    register uint64 a0 asm("a0") = 0x01;
    register uint64 a1 asm("a1") = blockCount;

    // Prelazak u sistemski režim.
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1)
                 : "memory");

    // Povratna vrednost se nalazi u a0.
    return (void*)a0;
}

int mem_free(void* ptr) {
    if (ptr == nullptr) {
        return -1;
    }

    // a0 = kod poziva, a1 = pokazivač.
    register uint64 a0 asm("a0") = 0x02;
    register uint64 a1 asm("a1") = (uint64)ptr;

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1)
                 : "memory");

    return (int)a0;
}