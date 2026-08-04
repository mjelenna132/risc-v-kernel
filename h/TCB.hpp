#ifndef _tcb_hpp
#define _tcb_hpp

#include "../lib/hw.h"
#include "MemoryAllocator.hpp"
#include "SleepingQueue.hpp"
#include "ThreadQueue.hpp"

class _Semaphore;

extern "C" void startUserThread();
extern "C" void startKernelThread();
extern "C" void threadWrapper();

// Kontrolni blok niti: kontekst, stanje i ulancavanje u redovima jezgra.
// Kontekst cuva samo ra, sp i s0-s11. Ostalo ne treba: promena konteksta se
// uvek desava u pozivu threadContextSwitch, pa je pozivalac vec odlozio svoje
// registre, a registre tela niti cuva prekidna rutina u svom okviru.
class TCB
{
public:
    using Body = void (*)(void*);

    // Telo korisnicke niti radi u U-modu, telo interne niti jezgra u S-modu
    // (mora, jer pristupa registrima kontrolera konzole).
    enum Mode
    {
        USER_MODE,
        SYSTEM_MODE,
    };

    // stackSpace je KRAJ vec odvojenog prostora za stek, kako trazi ABI 0x11.
    static TCB* createThread(Body body, void* arg, void* stackSpace);

    // Interna nit jezgra: sama alocira stek i ne broji se u activeThreads.
    static TCB* createKernelThread(Body body, void* arg);

    // Kontrolni blok za tok kontrole koji je vec u toku (funkcija main).
    static TCB* createMainThread();

    static void exit();
    static void dispatch();

    static void sleep(time_t periods);

    static void block();
    static void unblock(TCB* thread);

    // Blokira nit koja je vec u redu blokiranih datog semafora, ali najvise na
    // zadato vreme. Nit je tada u dva reda; budi je prvi dogadjaj koji nastupi.
    static void blockWithTimeout(_Semaphore* semaphore, time_t periods);

    // Nit je probudjena signalom, pa se otkazuje istek vremena. Zove semafor.
    static void cancelTimeout(TCB* thread);

    // Nit je probudjena istekom vremena. Zove red uspavanih niti.
    static void wakeFromSleep(TCB* thread);

    bool hasTimedOut() const { return timedOut; }

    // Iz obrade prekida od tajmera.
    static void onTimerTick();

    // Broj jos zivih niti aplikacije; kad padne na nulu, program se zavrsava.
    static uint64 getActiveThreads() { return activeThreads; }

    bool isFinished() const { return finished; }
    bool isBlocked() const { return blocked; }
    bool isSystemThread() const { return mode == SYSTEM_MODE; }

    // Sta semafor pamti uz nit dok ona ceka na njemu.
    unsigned getRequest() const { return request; }
    void setRequest(unsigned units) { request = units; }
    bool isSemaphoreClosed() const { return semaphoreClosed; }
    void setSemaphoreClosed(bool value) { semaphoreClosed = value; }

    void* operator new(size_t size) noexcept { return MemoryAllocator::mem_alloc(size); }
    void operator delete(void* p) noexcept { MemoryAllocator::mem_free(p); }

    struct Context
    {
        uint64 ra;
        uint64 sp;
        uint64 s[12];   // s0-s11
    };

    static TCB* running;

private:
    friend void threadWrapper();
    friend class ThreadQueue;
    friend class SleepingQueue;

    TCB(Body body, void* arg, void* stackSpace, Mode mode);
    ~TCB();

    // Oslobadja ugasene niti. Sme tek posle promene konteksta, jer se jezgro do
    // tada jos izvrsava na steku ugasene niti.
    static void reapFinished();

    Body body;
    void* arg;
    void* stackBase;
    Context context;

    TCB* next;              // red spremnih, blokiranih ili ugasenih
    TCB* nextSleeping;      // red uspavanih (poseban, zbog sem_timedwait)

    // Semafor na kom nit ceka sa ogranicenim vremenom, ili nullptr. Treba da bi
    // nit pri isteku vremena mogla da se skine sa njegovog reda blokiranih.
    _Semaphore* waitingSemaphore;

    time_t timeSlice;
    time_t sleepingTime;
    unsigned request;
    Mode mode;
    bool finished;
    bool blocked;
    bool semaphoreClosed;
    bool timedOut;

    static ThreadQueue finishedThreads;
    static SleepingQueue sleepingThreads;
    static uint64 activeThreads;

    // Odnosi se samo na nit koja drzi procesor, pa je dovoljna jedna vrednost.
    static time_t remainingTimeSlice;
};

extern "C" void threadContextSwitch(TCB::Context* oldContext, TCB::Context* runningContext);

#endif
