//
// Created by jelena on 9/5/26.
//

#include "../h/KernelConsole.hpp"
#include "../h/Semaphore.hpp"
#include "../h/TCB.hpp"
#include "../h/syscall_c.hpp"
#include "../lib/hw.h"

ConsoleBuffer KernelConsole::input;
ConsoleBuffer KernelConsole::output;

_sem* KernelConsole::inputItems = nullptr;
_sem* KernelConsole::outputItems = nullptr;
_sem* KernelConsole::outputSpaces = nullptr;

bool KernelConsole::initialized = false;

int KernelConsole::initialize()
{
    if (initialized) {
        return 0;
    }

    input.initialize();
    output.initialize();

    if (_sem::open(&inputItems, 0) < 0) {
        return -1;
    }

    if (_sem::open(&outputItems, 0) < 0) {
        _sem::close(inputItems);
        inputItems = nullptr;
        return -2;
    }

    if (_sem::open(&outputSpaces, ConsoleBuffer::capacity()) < 0) {
        _sem::close(outputItems);
        _sem::close(inputItems);

        outputItems = nullptr;
        inputItems = nullptr;

        return -3;
    }

    _thread* outputThread = nullptr;

    int result = _thread::createSystem(
        &outputThread,
        outputBody,
        nullptr
    );

    if (result < 0) {
        _sem::close(outputSpaces);
        _sem::close(outputItems);
        _sem::close(inputItems);

        outputSpaces = nullptr;
        outputItems = nullptr;
        inputItems = nullptr;

        return -4;
    }

    initialized = true;
    return 0;
}

char KernelConsole::get()
{
    // Ulazimo iz sistemskog poziva, sa maskiranim prekidima.
    if (!initialized) {
        return (char)EOF;
    }

    // Ako nema znakova, blokiramo pozivajuću nit.
    if (inputItems->wait() < 0) {
        return (char)EOF;
    }

    char character;

    if (!input.get(character)) {
        return (char)EOF;
    }

    return character;
}

void KernelConsole::put(char character)
{
    // Ulazimo iz sistemskog poziva, sa maskiranim prekidima.
    if (!initialized) {
        return;
    }

    // Ako nema mesta, blokiramo pozivajuću nit.
    if (outputSpaces->wait() < 0) {
        return;
    }

    if (!output.put(character)) {
        // Vraćamo rezervisano mesto ako upis nije uspeo.
        outputSpaces->signal();
        return;
    }

    // Obaveštavamo izlaznu nit da postoji novi znak.
    outputItems->signal();
}

void KernelConsole::handleInterrupt()
{
    int irq = plic_claim();

    if (irq == (int)CONSOLE_IRQ && initialized) {
        volatile uint8* status =
            (volatile uint8*)CONSOLE_STATUS;

        volatile uint8* receive =
            (volatile uint8*)CONSOLE_RX_DATA;

        // Ograničavamo trajanje jednog prolaska kroz prekid.
        unsigned received = 0;

        while ((*status & CONSOLE_RX_STATUS_BIT) != 0 &&
               received < 256) {

            char character = (char)*receive;
            received++;

            // Prekidna rutina ne sme da se blokira.
            // Ako je bafer pun, znak odbacujemo.
            if (input.put(character)) {
                inputItems->signal();
            }
        }
    }

    if (irq != 0) {
        plic_complete(irq);
    }
}

void KernelConsole::outputBody(void*)
{
    // Najveći broj znakova poslatih u jednom nizu bez ustupanja
    // procesora. Sprečava da izlazna nit monopoliše procesor kada
    // hardver prihvata znakove brže nego što ih druge niti
    // proizvode, a da pri tom ne mora da ustupa procesor posle
    // svakog pojedinačnog znaka.
    static const unsigned BURST_LIMIT = 32;

    volatile uint8* status =
        (volatile uint8*)CONSOLE_STATUS;

    volatile uint8* transmit =
        (volatile uint8*)CONSOLE_TX_DATA;

    while (true) {
        // Sistemska nit ovde ima omogućene prekide.
        // Koristimo C API da se pri blokiranju sačuva
        // ceo kontekst kroz prekidnu rutinu. Ovde nit
        // blokira samo ako trenutno nema šta da se šalje.
        if (sem_wait(outputItems) < 0) {
            return;
        }

        // Iznad je već rezervisan bar jedan znak za slanje.
        bool haveReservedChar = true;
        unsigned burst = 0;

        while (haveReservedChar) {
            // Ako hardver nije spreman, predajemo procesor dok
            // ne postane (prozivanje, bez uposlenog čekanja).
            while ((*status & CONSOLE_TX_STATUS_BIT) == 0) {
                thread_dispatch();
            }

            // Uzimanje znaka i predaja kontroleru rade se
            // bez prekida, zbog deljenog bafera i semafora.
            uint64 previousStatus;

            asm volatile(
                "csrrc %0, sstatus, %1"
                : "=r"(previousStatus)
                : "r"(2UL)
                : "memory"
            );

            char character;

            if (output.get(character)) {
                *transmit = (uint8)character;
                outputSpaces->signal();
            }

            // Vraćamo prethodno stanje dozvole prekida.
            if ((previousStatus & 2UL) != 0) {
                asm volatile("csrsi sstatus, 2" ::: "memory");
            }

            burst++;

            // Pravičnost: posle ograničenog niza poslatih znakova
            // dobrovoljno ustupamo procesor drugim spremnim
            // nitima, umesto da to radimo posle svakog znaka.
            if (burst >= BURST_LIMIT) {
                thread_dispatch();
                burst = 0;
            }

            // Bez blokiranja pokušavamo da rezervišemo sledeći
            // znak. Ako ga nema, vraćamo se na blokirajuće
            // čekanje na početku spoljašnje petlje.
            haveReservedChar = outputItems->tryWait();
        }
    }
}

bool KernelConsole::outputEmpty()
{
    return output.isEmpty();
}

bool KernelConsole::outputFlushed()
{
    if (!outputEmpty()) {
        return false;
    }

    // Bafer je prazan, ali poslednji znak je tek upisan u
    // kontroler u istoj kritičnoj sekciji u kojoj je izvađen iz
    // bafera. Dodatno proveravamo da je kontroler ponovo spreman
    // za prijem, čime potvrđujemo da je taj znak zaista preuzet
    // pre nego što jezgro zaustavi emulator.
    volatile uint8* status = (volatile uint8*)CONSOLE_STATUS;
    return (*status & CONSOLE_TX_STATUS_BIT) != 0;
}