//
// Created by jelena on 8/12/26.
//

#include "../h/Riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/TCB.hpp"
#include "../h/KernelConsole.hpp"
#include "../h/Semaphore.hpp"

// Ispisuje poruku o fatalnoj grešci direktno preko konzole jezgra
// (isti mehanizam koji koristi i sistemski poziv putc).
static void reportFault(uint64 cause)
{
    static const char message[] =
        "\n[JEZGRO] Nit prekinuta zbog nedozvoljene operacije, scause=";

    for (unsigned i = 0; message[i] != '\0'; i++) {
        KernelConsole::put(message[i]);
    }

    char digits[20];
    unsigned count = 0;

    do {
        digits[count++] = (char)('0' + cause % 10);
        cause /= 10;
    } while (cause != 0);

    while (count > 0) {
        KernelConsole::put(digits[--count]);
    }

    KernelConsole::put('\n');
}

extern "C" uint64 handleSupervisorTrap(
    uint64 code,
    uint64 argument1,
    uint64 argument2,
    uint64 argument3,
    uint64 argument4
)
{
    uint64 cause = Riscv::readScause();

    // Prekid od tajmera.
    if (cause == 0x8000000000000001UL) {
        volatile uint64 sepc = Riscv::readSepc();
        volatile uint64 sstatus = Riscv::readSstatus();

        uint64 mask = 1UL << 1;

        // Brišemo zahtev za tajmerski prekid.
        asm volatile(
            "csrc sip, %0"
            :
            : "r"(mask)
            : "memory"
        );

        // Preotimanje dozvoljavamo ako je prekinut korisnički kod.
        bool fromUserMode = (sstatus & (1UL << 8)) == 0;

        _thread::timerTick(fromUserMode);

        Riscv::writeSepc(sepc);
        Riscv::writeSstatus(sstatus);

        // Čuvamo prethodnu vrednost registra a0.
        return code;
    }

    // Prekid od konzole.
    if (cause == 0x8000000000000009UL) {
        uint64 sepc = Riscv::readSepc();
        uint64 sstatus = Riscv::readSstatus();

        KernelConsole::handleInterrupt();

        Riscv::writeSepc(sepc);
        Riscv::writeSstatus(sstatus);

        return code;
    }

    // Ilegalna instrukcija (2), nedozvoljeno čitanje (5) ili
    // nedozvoljen upis (7): umesto da se ista instrukcija zauvek
    // iznova izvršava (sepc se ovde ne pomera), ovo tretiramo kao
    // fatalnu grešku i zaustavljamo ceo program, isto kao pri
    // regularnom gašenju jezgra.
    if (cause == 2 || cause == 5 || cause == 7) {
        reportFault(cause);
        Riscv::haltMachine();

        // haltMachine() se nikad ne vraća (zaustavlja emulator).
        return code;
    }

    // Sistemski pozivi iz korisničkog i sistemskog režima.
    if (cause != 8 && cause != 9) {
        return code;
    }

    // Čuvamo podatke na steku niti kroz promenu konteksta.
    volatile uint64 sepc = Riscv::readSepc() + 4;
    volatile uint64 sstatus = Riscv::readSstatus();

    // cause 9: poziv je došao iz sistemskog režima.
    volatile bool fromSupervisor = (cause == 9);

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
    else if (code == 0x21) {
        result = (uint64)_sem::open(
            (_sem**)argument1,
            (unsigned)argument2
        );
    }
    else if (code == 0x22) {
        result = (uint64)_sem::close(
            (_sem*)argument1
        );
    }
    else if (code == 0x23) {
        _sem* semaphore = (_sem*)argument1;

        result = semaphore != nullptr
            ? (uint64)semaphore->wait()
            : (uint64)-1;
    }
    else if (code == 0x24) {
        _sem* semaphore = (_sem*)argument1;

        result = semaphore != nullptr
            ? (uint64)semaphore->signal()
            : (uint64)-1;
    }
    else if (code == 0x25) {
        _sem* semaphore = (_sem*)argument1;

        result = semaphore != nullptr
            ? (uint64)semaphore->waitN((unsigned)argument2)
            : (uint64)-1;
    }
    else if (code == 0x26) {
        _sem* semaphore = (_sem*)argument1;

        result = semaphore != nullptr
            ? (uint64)semaphore->signalN((unsigned)argument2)
            : (uint64)-1;
    }
    else if (code == 0x31) {
        result = (uint64)_thread::sleep(argument1);
    }
    else if (code == 0x41) {
        // Učitavanje znaka iz našeg ulaznog bafera.
        result = (uint64)(unsigned char)KernelConsole::get();
    }
    else if (code == 0x42) {
        // Smeštanje znaka u naš izlazni bafer.
        KernelConsole::put((char)argument1);
        result = 0;
    }
    else {
        result = (uint64)-1;
    }

    uint64 restoreStatus = sstatus;

    // Vraćamo pozivaoca u režim iz kog je izvršio ecall.
    if (fromSupervisor) {
        restoreStatus |= (1UL << 8);
    }
    else {
        restoreStatus &= ~(1UL << 8);
    }

    Riscv::writeSepc(sepc);
    Riscv::writeSstatus(restoreStatus);

    return result;
}

void Riscv::popSppSpie()
{
    asm volatile(
        // Nastavljamo iza poziva ove funkcije.
        "csrw sepc, ra\n"

        // SPP = 0: korisnički režim.
        "li t0, 0x100\n"
        "csrc sstatus, t0\n"

        // Omogućavamo prekide nakon sret.
        "li t0, 0x20\n"
        "csrs sstatus, t0\n"

        "sret\n"
    );
}

void Riscv::haltMachine()
{
    // Direktan poziv (ne sistemski poziv) jer se ovo poziva i iz
    // konteksta koji je već unutar jezgra (obrada izuzetka).
    while (!KernelConsole::outputFlushed()) {
        _thread::dispatch();
    }

    // Zaustavljanje emulatora RISC-V procesora (videti hw.h/uputstvo).
    *(volatile uint32*)0x100000 = 0x5555;
}