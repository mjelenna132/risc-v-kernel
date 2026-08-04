#ifndef _semaphore_hpp
#define _semaphore_hpp

#include "../lib/hw.h"
#include "MemoryAllocator.hpp"
#include "ThreadQueue.hpp"

// Opsti (brojacki) semafor. Vrednost je broj raspolozivih jedinica i nikad nije
// negativna; niti koje cekaju stoje u FIFO redu i nose broj trazenih jedinica.
// Nit koja naidje dok red nije prazan i sama staje u red, cak i kad bi mogla
// odmah da prodje, da ne bi preticala i izgladnjivala one koje traze vise.
class _Semaphore
{
public:
    static int open(_Semaphore** handle, unsigned init);
    static int close(_Semaphore* handle);

    int wait(unsigned units);
    int signal(unsigned units);

    // Ne blokira se; vraca WOULD_BLOCK i ne menja stanje ako ne moze odmah.
    int tryWait(unsigned units);

    // Ceka najvise zadati broj perioda tajmera, pa vraca TIMED_OUT.
    int timedWait(unsigned units, time_t periods);

    // Skida nit sa reda blokiranih kad joj istekne vreme; zove TCB.
    void removeWaiting(TCB* thread) { blocked.remove(thread); }

    void* operator new(size_t size) noexcept { return MemoryAllocator::mem_alloc(size); }
    void operator delete(void* p) noexcept { MemoryAllocator::mem_free(p); }

    // Nije greska, pa nije negativno.
    static const int WOULD_BLOCK = 1;

private:
    enum ErrorCode
    {
        INVALID_HANDLE = -1,
        NO_MEMORY = -2,
        SEMAPHORE_CLOSED = -3,
        TIMED_OUT = -4,
    };

    explicit _Semaphore(unsigned init) : value(init), blocked() {}

    // Deblokira sve zatecene niti; njihov wait vraca gresku.
    ~_Semaphore();

    // Deblokira sa cela reda dok god vrednost pokriva zahtev prve niti.
    void releaseWaiting();

    uint64 value;
    ThreadQueue blocked;
};

#endif
