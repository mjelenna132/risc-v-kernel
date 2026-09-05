//
// Created by jelena on 9/5/26.
//

#ifndef OS_PROJECT_KERNEL_CONSOLE_HPP
#define OS_PROJECT_KERNEL_CONSOLE_HPP

#include "ConsoleBuffer.hpp"

class _sem;

class KernelConsole {
public:
    // Poziva se jednom, nakon inicijalizacije TCB-a,
    // u sistemskom režimu sa isključenim prekidima.
    static int initialize();

    // Pozivaju se iz obrade sistemskih poziva.
    static char get();
    static void put(char character);

    // Poziva se iz obrade spoljašnjeg prekida.
    static void handleInterrupt();

    // Poziva se sa isključenim prekidima.
    static bool outputEmpty();

private:
    static ConsoleBuffer input;
    static ConsoleBuffer output;

    // Broj dostupnih ulaznih znakova.
    static _sem* inputItems;

    // Broj izlaznih znakova koji čekaju slanje.
    static _sem* outputItems;

    // Broj slobodnih mesta u izlaznom baferu.
    static _sem* outputSpaces;

    static bool initialized;

    // Telo sistemske niti za slanje znakova.
    static void outputBody(void*);
};

#endif // OS_PROJECT_KERNEL_CONSOLE_HPP
