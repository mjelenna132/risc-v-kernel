//
// Created by jelena on 8/19/26.
//
#include "../h/Semaphore.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/Scheduler.hpp"

_sem::_sem(unsigned init)
    : value(init),
      blockedHead(nullptr),
      blockedTail(nullptr),
      closed(false)
{
}
// Poziv new _sem(init) automatski poziva ovaj operator sa
// size = sizeof(_sem), kako bi rezervisao memoriju za novi objekat.
void* _sem::operator new(size_t size) noexcept
{
    // Pretvaramo veličinu objekta iz bajtova u blokove.
    size_t blockCount = size / MEM_BLOCK_SIZE;

    if (size % MEM_BLOCK_SIZE != 0) {
        blockCount++;
    }

    return MemoryAllocator::mem_alloc(blockCount);
}

void _sem::operator delete(void* ptr) noexcept
{
    MemoryAllocator::mem_free(ptr);
}

int _sem::open(_sem** handle, unsigned init)
{
    // Potrebna nam je adresa na koju upisujemo ručku.
    if (handle == nullptr) {
        return -1;
    }

    *handle = new _sem(init);

    // Nema dovoljno memorije za objekat semafora.
    if (*handle == nullptr) {
        return -2;
    }

    return 0;
}

void _sem::addBlocked(_thread* thread)
{
    // Nova nit postaje poslednja u redu.
    thread->next = nullptr;

    // Red je prazan.
    if (blockedTail == nullptr) {
        blockedHead = thread;
        blockedTail = thread;
    } else {
        // Stari poslednji pokazuje na novu nit.
        blockedTail->next = thread;
        blockedTail = thread;
    }
}

_thread* _sem::removeBlocked()
{
    // Niko ne čeka.
    if (blockedHead == nullptr) {
        return nullptr;
    }

    // Uzimamo prvu nit.
    _thread* thread = blockedHead;
    blockedHead = blockedHead->next;

    // Ako smo uklonili jedinu nit, red je sada prazan.
    if (blockedHead == nullptr) {
        blockedTail = nullptr;
    }

    thread->next = nullptr;
    return thread;
}

int _sem::wait()
{
    // Ugašen semafor ne može da se koristi.
    if (closed) {
        return -1;
    }

    // Postoji slobodan resurs.
    if (value > 0) {
        value--;
        return 0;
    }

    // Nema resursa, pa blokiramo tekuću nit.
    _thread* current = _thread::running;

    current->blocked = true;
    current->waitResult = 0;

    addBlocked(current);

    // Prelazimo na neku drugu spremnu nit.
    _thread::dispatch();

    // Kada se ova nit probudi, nastavlja odavde.
    return _thread::running->waitResult;
}

int _sem::signal()
{
    // Ugašen semafor ne može da se koristi.
    if (closed) {
        return -1;
    }

    // Proveravamo da li neka nit čeka.
    _thread* thread = removeBlocked();

    if (thread != nullptr) {
        // Probuđena nit sada može ponovo da se izvršava.
        thread->blocked = false;
        thread->waitResult = 0;

        Scheduler::put(thread);
    } else {
        // Niko ne čeka, pa povećavamo broj resursa.
        value++;
    }

    return 0;
}

int _sem::close(_sem* handle)
{
    // Ne postoji semafor koji treba ugasiti.
    if (handle == nullptr) {
        return -1;
    }

    // Semafor više ne sme da se koristi.
    handle->closed = true;

    // Budimo sve niti koje su čekale.
    _thread* thread = handle->removeBlocked();

    while (thread != nullptr) {
        thread->blocked = false;

        // Wait vraća grešku jer je semafor ugašen.
        thread->waitResult = -1;

        Scheduler::put(thread);

        thread = handle->removeBlocked();
    }

    // Oslobađamo memoriju objekta semafora.
    delete handle;

    return 0;
}