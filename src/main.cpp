//
// Created by jelena on 8/12/26.
//
#include "../h/Riscv.hpp"
#include "../h/syscall_c.hpp"
#include "../lib/console.h"

static void printText(const char* text) {
    while (*text != '\0') {
        __putc(*text);
        text++;
    }
}

int main() {
    Riscv::writeStvec((uint64)&supervisorTrap);

    bool success = true;

    void* first = mem_alloc(100);
    void* second = mem_alloc(200);

    if (first == nullptr || second == nullptr || first == second) {
        success = false;
    }

    // Provera korisničkog prostora.
    if (first != nullptr) {
        char* data = (char*)first;

        for (int i = 0; i < 100; i++) {
            data[i] = 'A';
        }
    }

    // Provera oslobađanja i dvostrukog oslobađanja.
    if (first != nullptr) {
        if (mem_free(first) != 0) {
            success = false;
        }

        if (mem_free(first) >= 0) {
            success = false;
        }
    }

    if (second != nullptr && mem_free(second) != 0) {
        success = false;
    }

    // Posle spajanja treba ponovo da se koristi početak heap-a.
    void* merged = mem_alloc(300);

    if (merged == nullptr || merged != first) {
        success = false;
    }

    if (merged != nullptr && mem_free(merged) != 0) {
        success = false;
    }

    // Prevelik zahtev mora biti odbijen.
    if (mem_alloc((size_t)-1) != nullptr) {
        success = false;
    }

    if (success) {
        printText("Svi testovi alokatora su prosli\n");
    } else {
        printText("Test alokatora nije prosao\n");
    }

    *(volatile uint32*)0x100000 = 0x5555;
    return 0;
}