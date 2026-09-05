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

    // Pravi novu korisničku nit.
    static int create(_thread** handle, Body body,
                      void* arg, uint64* stackTop);

    // Pravi internu sistemsku nit.
    // Poziva se iz sistemskog režima sa isključenim prekidima.
    static int createSystem(_thread** handle, Body body, void* arg);

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
    static bool hasActiveUserThreads()
    {
        return activeUserThreads != 0;
    }

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

    // Blokiranu nit ne vraćamo među spremne niti.
    bool blocked;

    // Rezultat čekanja: negativan ako je semafor ugašen.
    int waitResult;

    // Koliko jedinica resursa nit čeka od semafora.
    unsigned waitUnits;

    // Broj perioda do buđenja ove niti.
    uint64 sleepTime;

    // Sledeća nit u redu.
    _thread* next;

    // Da li telo niti treba da radi u sistemskom režimu.
    bool systemThread;

    // Nit koja se trenutno izvršava.
    static _thread* running;

    // Prva nit u listi uspavanih niti.
    static _thread* sleepingHead;

    // Koliko perioda tajmera tekuća nit već izvršava.
    static uint64 timeSliceCounter;

    // Završena nit čije je oslobađanje odloženo.
    static _thread* threadToDelete;

    _thread(Body body, void* arg, uint64* stackTop);
    ~_thread();

    // Početna funkcija svake nove niti.
    static void threadWrapper();

    friend class Scheduler;
    friend class _sem;

    // Alokacija TCB-a pomoću našeg alokatora.
    void* operator new(size_t size) noexcept;
    void operator delete(void* ptr) noexcept;

    // Ubacuje nit u uređenu listu uspavanih niti.
    static void addToSleepList(_thread* thread);

    // Oslobađa završenu nit sa steka druge niti.
    static void cleanupFinishedThread();
    static unsigned activeUserThreads;
};

// Asemblerska promena registara sp i ra.
extern "C" void contextSwitch(uint64* oldContext,
                              uint64* newContext);

#endif // OS_PROJECT_TCB_HPP