#include "../h/TCB.hpp"
#include "../h/Riscv.hpp"
#include "../h/Scheduler.hpp"
#include "../h/Semaphore.hpp"
#include "../h/syscall_c.hpp"

TCB* TCB::running = nullptr;
ThreadQueue TCB::finishedThreads;
SleepingQueue TCB::sleepingThreads;
uint64 TCB::activeThreads = 0;
time_t TCB::remainingTimeSlice = 0;

TCB::TCB(Body body, void* arg, void* stackSpace, Mode mode) :
        body(body),
        arg(arg),
        // stackSpace je kraj steka; pocetak, koji se oslobadja, je za
        // DEFAULT_STACK_SIZE nize.
        stackBase(stackSpace != nullptr
                  ? (void*) ((uint64) stackSpace - DEFAULT_STACK_SIZE)
                  : nullptr),
        // Nova nit krece od potprograma iz trapEntry.S koji joj fabrikuje okvir
        // i "vrati" je u telo, u odgovarajucem rezimu.
        context({body != nullptr
                 ? (mode == USER_MODE ? (uint64) &startUserThread
                                      : (uint64) &startKernelThread)
                 : 0,
                 (uint64) stackSpace
                }),
        next(nullptr),
        nextSleeping(nullptr),
        waitingSemaphore(nullptr),
        timeSlice(DEFAULT_TIME_SLICE),
        sleepingTime(0),
        request(0),
        mode(mode),
        finished(false),
        blocked(false),
        semaphoreClosed(false),
        timedOut(false)
{
}

TCB::~TCB()
{
    if (stackBase != nullptr) { MemoryAllocator::mem_free(stackBase); }
}

TCB* TCB::createThread(Body body, void* arg, void* stackSpace)
{
    if (body == nullptr || stackSpace == nullptr) { return nullptr; }

    TCB* thread = new TCB(body, arg, stackSpace, USER_MODE);
    if (thread == nullptr) { return nullptr; }

    activeThreads++;
    Scheduler::put(thread);
    return thread;
}

TCB* TCB::createKernelThread(Body body, void* arg)
{
    if (body == nullptr) { return nullptr; }

    void* stack = MemoryAllocator::mem_alloc(DEFAULT_STACK_SIZE);
    if (stack == nullptr) { return nullptr; }

    void* stackSpace = (void*) ((uint64) stack + DEFAULT_STACK_SIZE);
    TCB* thread = new TCB(body, arg, stackSpace, SYSTEM_MODE);
    if (thread == nullptr) {
        MemoryAllocator::mem_free(stack);
        return nullptr;
    }

    Scheduler::put(thread);
    return thread;
}

TCB* TCB::createMainThread()
{
    running = new TCB(nullptr, nullptr, nullptr, SYSTEM_MODE);
    remainingTimeSlice = (running != nullptr) ? running->timeSlice : 0;
    return running;
}

extern "C" void threadWrapper()
{
    TCB::running->body(TCB::running->arg);

    // Preko ecall-a, a ne direktno: telo radi u U-modu, a promena konteksta
    // mora da se desi u S-modu.
    thread_exit();

    for (;;) {}
}

void TCB::exit()
{
    running->finished = true;
    if (running->mode == USER_MODE) { activeThreads--; }
    dispatch();
}

void TCB::dispatch()
{
    TCB* old = running;
    if (!old->finished && !old->blocked) { Scheduler::put(old); }

    running = Scheduler::get();
    if (running == nullptr) {
        // Ne moze da se desi dok glavna nit jezgra kruzi kroz red spremnih niti.
        Riscv::panic("nema spremnih niti");
    }

    // Nit koja preuzima procesor dobija ceo svoj vremenski odsecak.
    remainingTimeSlice = running->timeSlice;

    if (old->finished) { finishedThreads.put(old); }
    if (old != running) { threadContextSwitch(&old->context, &running->context); }

    reapFinished();
}

void TCB::sleep(time_t periods)
{
    if (periods == 0) { return; }

    sleepingThreads.put(running, periods);
    block();
}

void TCB::block()
{
    running->blocked = true;
    dispatch();
}

void TCB::unblock(TCB* thread)
{
    if (thread == nullptr) { return; }

    thread->blocked = false;
    Scheduler::put(thread);
}

void TCB::blockWithTimeout(_Semaphore* semaphore, time_t periods)
{
    running->waitingSemaphore = semaphore;
    running->timedOut = false;

    sleepingThreads.put(running, periods);
    block();
}

void TCB::cancelTimeout(TCB* thread)
{
    if (thread == nullptr || thread->waitingSemaphore == nullptr) { return; }

    thread->waitingSemaphore = nullptr;
    sleepingThreads.remove(thread);
}

void TCB::wakeFromSleep(TCB* thread)
{
    if (thread == nullptr) { return; }

    // Nit jos stoji u redu blokiranih svog semafora, pa se prvo skida odatle.
    if (thread->waitingSemaphore != nullptr) {
        _Semaphore* semaphore = thread->waitingSemaphore;
        thread->waitingSemaphore = nullptr;
        thread->timedOut = true;
        semaphore->removeWaiting(thread);
    }

    unblock(thread);
}

void TCB::onTimerTick()
{
    sleepingThreads.tick();

    if (remainingTimeSlice > 0) { remainingTimeSlice--; }
    if (remainingTimeSlice == 0) { dispatch(); }
}

void TCB::reapFinished()
{
    for (TCB* thread = finishedThreads.get(); thread != nullptr;
         thread = finishedThreads.get()) {
        delete thread;
    }
}
