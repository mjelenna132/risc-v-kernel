#include "../h/Console.hpp"

#include "../h/CharBuffer.hpp"
#include "../h/Riscv.hpp"
#include "../h/Semaphore.hpp"
#include "../h/TCB.hpp"
#include "../h/syscall_abi.hpp"

// Medjusobno iskljucenje se ne obezbedjuje posebno: baferima se pristupa samo
// iz jezgra, dakle sa maskiranim prekidima. Semafori sluze za cekanje.
static CharBuffer inputBuffer;
static CharBuffer outputBuffer;

static _Semaphore* inputChars = nullptr;    // znakovi pristigli sa tastature
static _Semaphore* outputChars = nullptr;   // znakovi koji cekaju na ispis
static _Semaphore* outputSlots = nullptr;   // slobodna mesta u izlaznom baferu

void _Console::init()
{
    if (_Semaphore::open(&inputChars, 0) != 0
        || _Semaphore::open(&outputChars, 0) != 0
        || _Semaphore::open(&outputSlots, CharBuffer::CAPACITY) != 0) {
        Riscv::panic("neuspesna inicijalizacija konzole");
    }

    if (TCB::createKernelThread(&outputThreadBody, nullptr) == nullptr) {
        Riscv::panic("neuspesno pokretanje niti za ispis na konzolu");
    }
}

char _Console::getc()
{
    inputChars->wait(1);

    char c = 0;
    inputBuffer.get(c);
    return c;
}

void _Console::putc(char c)
{
    outputSlots->wait(1);
    outputBuffer.put(c);
    outputChars->signal(1);
}

char _Console::nextOutputChar()
{
    outputChars->wait(1);

    char c = 0;
    outputBuffer.peek(c);
    return c;
}

void _Console::outputCharSent()
{
    char sent;
    if (outputBuffer.get(sent)) { outputSlots->signal(1); }
}

void _Console::handleInterrupt()
{
    volatile uint8* status = (volatile uint8*) CONSOLE_STATUS;
    volatile char* rxData = (volatile char*) CONSOLE_RX_DATA;

    for (size_t i = 0; i < MAX_CHARS_PER_INTERRUPT
                       && (*status & CONSOLE_RX_STATUS_BIT) != 0; i++) {
        char c = *rxData;

        // Hardver kao proizvodjac ne moze da se blokira, pa se pri punom
        // baferu znak odbacuje.
        if (inputBuffer.put(c)) { inputChars->signal(1); }
    }
}

void _Console::flush()
{
    while (!outputBuffer.isEmpty()) { abiSyscall(SYS_THREAD_DISPATCH); }
}

void _Console::putcDirect(char c)
{
    volatile uint8* status = (volatile uint8*) CONSOLE_STATUS;
    volatile char* txData = (volatile char*) CONSOLE_TX_DATA;

    while ((*status & CONSOLE_TX_STATUS_BIT) == 0) {}

    *txData = c;
}

void _Console::flushDirect()
{
    char c;
    while (outputBuffer.get(c)) { putcDirect(c); }
}

// Radi u S-modu, pa sme na registre kontrolera. Jezgru ipak pristupa preko
// ecall-a kao i korisnicke niti, tako da strukture jezgra dira sa maskiranim
// prekidima, a uposleno cekanje na kontroler ostaje van jezgra, gde sme da
// bude preoteta.
void _Console::outputThreadBody(void*)
{
    volatile uint8* status = (volatile uint8*) CONSOLE_STATUS;
    volatile char* txData = (volatile char*) CONSOLE_TX_DATA;

    for (;;) {
        char c = (char) abiSyscall(SYS_CONSOLE_NEXT_OUTPUT);

        while ((*status & CONSOLE_TX_STATUS_BIT) == 0) {}
        *txData = c;

        abiSyscall(SYS_CONSOLE_OUTPUT_SENT);
    }
}
