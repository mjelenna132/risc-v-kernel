//
// Created by jelena on 8/13/26.
//

#ifndef OS_PROJECT_TCB_HPP
#define OS_PROJECT_TCB_HPP

#include "../lib/hw.h"

class Scheduler;

class _thread {
public:
    using Body = void (*)(void*);

    // Pravi novu nit.
    static int create(_thread** handle, Body body,
                      void* arg, uint64* stackTop);

    // Završava tekuću nit.
    static int exit();

    // Predaje procesor sledećoj niti.
    static void dispatch();

    // Priprema TCB početne main niti.
    static void initialize();

    // Uspavljuje tekuću nit na zadati broj perioda tajmera.
    static int sleep(uint64 time);

    // Poziva se pri svakom prekidu tajmera.
    static void timerTick(bool allowPreemption);

private:
    struct Context {
        // Mesto na kome nit nastavlja izvršavanje.
        uint64 ra;

        // Trenutni vrh steka niti.
        uint64 sp;
    };

    // Funkcija koju nit izvršava.
    Body body;

    // Argument funkcije.
    void* arg;

    // Memorija odvojena za stek.
    uint64* stack;

    // Sačuvani registri niti.
    Context context;

    // Da li se nit završila.
    bool finished;

    // Da li nit čeka na semaforu.
    // Nit ne sme da se vrati u red spremnih ako je blokirana
    bool blocked;

    //waitResult nam treba da probuđena nit zna zašto je probuđena.
    // <0 ako je semafor ugasen znaci odblokirane su sve niti
    int waitResult;

    // Koliko jedinica resursa nit čeka od semafora.
    unsigned waitUnits;

    // Broj perioda do buđenja ove niti.
    uint64 sleepTime;

    // Sledeća nit u redu Scheduler-a.
    _thread* next;

    // Nit koja se trenutno izvršava.
    static _thread* running;

    // Prva nit u listi uspavanih niti.
    static _thread* sleepingHead;

    // Koliko perioda tajmera tekuća nit već izvršava.
    static uint64 timeSliceCounter;


    _thread(Body body, void* arg, uint64* stackTop);

    // Početna funkcija svake nove niti.
    static void threadWrapper();

    friend class Scheduler;
    friend class _sem;

    // Alokacija TCB-a pomoću našeg MemoryAllocator-a.
    void* operator new(size_t size) noexcept;
    void operator delete(void* ptr) noexcept;
    //noexcept ne baca izuzetak
    //

    // Ubacuje nit u uređenu listu uspavanih niti.
    static void addToSleepList(_thread* thread);

};

// Asemblerska promena registara sp i ra.
extern "C" void contextSwitch(uint64* oldContext,
                              uint64* newContext);

#endif // OS_PROJECT_TCB_HPP
