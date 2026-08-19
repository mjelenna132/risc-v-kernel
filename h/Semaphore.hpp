//
// Created by jelena on 8/19/26.
//

#ifndef OS_PROJECT_SEMAPHORE_HPP
#define OS_PROJECT_SEMAPHORE_HPP

#include "../lib/hw.h"
#include "../h/TCB.hpp"

class _thread;

class _sem {
public:
    // Pravi novi semafor.
    static int open(_sem** handle, unsigned init);

    // Gasi semafor.
    static int close(_sem* handle);

    // Uzima jednu jedinicu resursa.
    int wait();

    // Vraća jednu jedinicu resursa.
    int signal();

private:
    explicit _sem(unsigned init);

    // Broj trenutno slobodnih jedinica.
    unsigned value;

    // Niti koje čekaju na ovom semaforu.
    _thread* blockedHead;
    _thread* blockedTail;

    // Da li je semafor ugašen.
    bool closed;

    // Alokacija pomoću našeg alokatora.
    void* operator new(size_t size) noexcept;
    void operator delete(void* ptr) noexcept;

    // Dodaje nit na kraj reda čekanja.
    void addBlocked(_thread* thread);

    // Uklanja i vraća prvu nit iz reda čekanja.
    _thread* removeBlocked();

};

#endif // OS_PROJECT_SEMAPHORE_HPP
