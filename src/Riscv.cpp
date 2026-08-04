#include "../h/Riscv.hpp"

#include "../h/Console.hpp"
#include "../h/TCB.hpp"
#include "../h/syscall_dispatch.hpp"

extern "C" void trapEntry();

// Zaustavljanje emulatora.
static const uint64 EMULATOR_FINISHER_ADDR = 0x100000;
static const uint32 EMULATOR_FINISHER_VALUE = 0x5555;

// Pozicije u okviru prekidne rutine: x0-x31 redom, pa sepc i sstatus.
// Argumenti poziva su a0-a4, tj. x10-x14.
static const size_t FRAME_A0 = 10;
static const size_t FRAME_A1 = 11;
static const size_t FRAME_A2 = 12;
static const size_t FRAME_A3 = 13;
static const size_t FRAME_A4 = 14;
static const size_t FRAME_SEPC = 32;

// Povratak iz poziva ide na instrukciju IZA ecall-a, ne na nju samu.
static const uint64 ECALL_LENGTH = 4;

void Riscv::initTrap()
{
    w_stvec((uint64) trapEntry);

    // Tajmer (stize kao softverski prekid) i konzola. Registar sie vazi u SVIM
    // rezimima, za razliku od sstatus.SIE koji se u U-modu ignorise.
    w_sie(SIE_SSIE | SIE_SEIE);
}

static void printString(const char* text)
{
    for (const char* p = text; *p != '\0'; p++) { _Console::putcDirect(*p); }
}

static void printNumber(uint64 value, uint64 base)
{
    static const char digits[] = "0123456789abcdef";
    char buf[64];
    int i = 0;

    if (value == 0) { buf[i++] = '0'; }
    while (value != 0) {
        buf[i++] = digits[value % base];
        value /= base;
    }
    while (i > 0) { _Console::putcDirect(buf[--i]); }
}

void Riscv::halt()
{
    *(volatile uint32*) EMULATOR_FINISHER_ADDR = EMULATOR_FINISHER_VALUE;

    for (;;) {}
}

void Riscv::panic(const char* message)
{
    // Interna nit vise nece dobiti procesor, pa se zaostali ispis gura rucno.
    _Console::flushDirect();

    printString("\nKERNEL PANIC: ");
    printString(message);
    _Console::putcDirect('\n');

    halt();
}

// PLIC kaze koji uredjaj je prekinuo i trazi potvrdu da je prekid obradjen.
static void handleExternalInterrupt()
{
    int irq = plic_claim();

    if (irq == (int) CONSOLE_IRQ) { _Console::handleInterrupt(); }

    if (irq != 0) { plic_complete(irq); }
}

extern "C" void handleSupervisorTrap(uint64* frame)
{
    switch (Riscv::r_scause()) {
        case Riscv::ECALL_FROM_USER_MODE:
        case Riscv::ECALL_FROM_SYSTEM_MODE:
            // Argumenti i rezultat idu kroz okvir na steku, a ne kroz same
            // registre: obrada poziva sme da promeni tekucu nit.
            frame[FRAME_SEPC] += ECALL_LENGTH;
            frame[FRAME_A0] = (uint64) dispatchSyscall(frame[FRAME_A0],
                                                       frame[FRAME_A1],
                                                       frame[FRAME_A2],
                                                       frame[FRAME_A3],
                                                       frame[FRAME_A4]);
            break;

        case Riscv::TIMER_INTERRUPT:
            Riscv::mc_sip(Riscv::SIP_SSIP);
            TCB::onTimerTick();
            break;

        case Riscv::CONSOLE_INTERRUPT:
            handleExternalInterrupt();
            break;

        default:
            // Neobradjen izuzetak, npr. korisnicki kod je pokusao privilegovanu
            // instrukciju (to je i ono sto TEST 7 proverava).
            printString("\nscause=");
            printNumber(Riscv::r_scause(), 10);
            printString(" sepc=0x");
            printNumber(frame[FRAME_SEPC], 16);
            Riscv::panic("neocekivan izuzetak u korisnickom programu");
    }
}
