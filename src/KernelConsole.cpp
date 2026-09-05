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

    // ConsoleBuffer ima 256 mesta.
    if (_sem::open(&outputSpaces, 256) < 0) {
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
    volatile uint8* status =
        (volatile uint8*)CONSOLE_STATUS;

    volatile uint8* transmit =
        (volatile uint8*)CONSOLE_TX_DATA;

    while (true) {
        // Sistemska nit ovde ima omogućene prekide.
        // Koristimo C API da se pri blokiranju sačuva
        // ceo kontekst kroz prekidnu rutinu.
        if (sem_wait(outputItems) < 0) {
            return;
        }

        // Ako hardver nije spreman, predajemo procesor.
        while ((*status & CONSOLE_TX_STATUS_BIT) == 0) {
            thread_dispatch();
        }

        // Uzimanje znaka i oslobađanje mesta radimo
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

        // Tajmer ne preotima sistemski kod, pa izlazna
        // nit dobrovoljno daje priliku drugim nitima.
        thread_dispatch();
    }
}

bool KernelConsole::outputEmpty()
{
    return output.isEmpty();
}