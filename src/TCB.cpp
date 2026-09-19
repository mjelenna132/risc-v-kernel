#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/Riscv.hpp"
#include "../h/syscall_c.hpp"

_thread* _thread::running = nullptr;
_thread* _thread::sleepingHead = nullptr;
uint64 _thread::timeSliceCounter = 0;
_thread* _thread::threadToDelete = nullptr;
unsigned _thread::activeUserThreads = 0;

void* _thread::operator new(size_t size) noexcept
{
    size_t blockCount =
        (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    return MemoryAllocator::mem_alloc(blockCount);
}

void _thread::operator delete(void* ptr) noexcept
{
    MemoryAllocator::mem_free(ptr);
}

_thread::_thread(Body body, void* arg, uint64* stackTop)
    : body(body),
      arg(arg),
      stack(body != nullptr
                ? stackTop - DEFAULT_STACK_SIZE / sizeof(uint64)
                : nullptr),
      context{
          body != nullptr ? (uint64)&threadWrapper : 0,
          body != nullptr ? (uint64)stackTop : 0
      },
      finished(false),
      blocked(false),
      waitResult(0),
      waitUnits(0),
      sleepTime(0),
      next(nullptr),
      systemThread(false)
{
}

_thread::~_thread()
{
    if (stack != nullptr) {
        MemoryAllocator::mem_free(stack);
    }
}

void _thread::cleanupFinishedThread()
{
    if (threadToDelete != nullptr &&
        threadToDelete != running) {

        _thread* finishedThread = threadToDelete;
        threadToDelete = nullptr;

        delete finishedThread;
    }
}

void _thread::initialize()
{
    if (running == nullptr) {
        // Main već radi, pa mu ne pravimo novi stek.
        running = new _thread(nullptr, nullptr, nullptr);
    }
}

int _thread::create(_thread** handle, Body body,
                    void* arg, uint64* stackTop)
{
    if (handle == nullptr || body == nullptr ||
        stackTop == nullptr) {
        return -1;
    }

    _thread* thread = new _thread(body, arg, stackTop);

    if (thread == nullptr) {
        return -2;
    }

    *handle = thread;
    activeUserThreads++;
    Scheduler::put(thread);

    return 0;
}

int _thread::createSystem(_thread** handle, Body body, void* arg)
{
    // Poziva se iz sistemskog režima sa isključenim prekidima.
    if (handle == nullptr || body == nullptr) {
        return -1;
    }

    *handle = nullptr;

    size_t blocks = DEFAULT_STACK_SIZE / MEM_BLOCK_SIZE;

    if (DEFAULT_STACK_SIZE % MEM_BLOCK_SIZE != 0) {
        blocks++;
    }

    // Već smo u jezgru, pa direktno koristimo alokator.
    uint64* stackBase =
        (uint64*)MemoryAllocator::mem_alloc(blocks);

    if (stackBase == nullptr) {
        return -2;
    }

    uint64* stackTop =
        stackBase + DEFAULT_STACK_SIZE / sizeof(uint64);

    _thread* thread = new _thread(body, arg, stackTop);

    if (thread == nullptr) {
        MemoryAllocator::mem_free(stackBase);
        return -3;
    }

    // Postavljamo režim pre ubacivanja među spremne niti.
    thread->systemThread = true;

    *handle = thread;
    Scheduler::put(thread);

    return 0;
}

int _thread::exit()
{
    if (running == nullptr || running->finished) {
        return -1;
    }

    // Početna main nit nema body i nije uključena u brojač.
    if (!running->systemThread && running->body != nullptr) {
        activeUserThreads--;
    }

    running->finished = true;

    // Oslobađanje obavlja druga nit nakon promene steka.
    threadToDelete = running;

    dispatch();

    return 0;
}

void _thread::dispatch()
{
    _thread* old = running;

    // Spremnu nezavršenu nit vraćamo na kraj reda.
    if (old != nullptr && !old->finished && !old->blocked) {
        Scheduler::put(old);
    }

    running = Scheduler::get();

    // Nova nit dobija ceo vremenski odsečak.
    timeSliceCounter = 0;

    // Trenutni main ostaje spreman dok čeka korisničke niti.
    if (running == nullptr) {
        running = old;
        return;
    }

    if (running == old) {
        return;
    }

    contextSwitch((uint64*)&old->context,
                  (uint64*)&running->context);

    cleanupFinishedThread();
}

void _thread::threadWrapper()
{
    // Već smo na steku nove niti.
    cleanupFinishedThread();

    if (running->systemThread) {
        // Ostajemo u sistemskom režimu i omogućavamo prekide.
        asm volatile("csrsi sstatus, 2" ::: "memory");
    }
    else {
        // Obične niti prelaze u korisnički režim.
        Riscv::popSppSpie();
    }

    running->body(running->arg);

    thread_exit();
}

void _thread::addToSleepList(_thread* thread)
{
    if (thread == nullptr) {
        return;
    }

    uint64 remainingTime = thread->sleepTime;

    _thread* previous = nullptr;
    _thread* current = sleepingHead;

    // Vremena u listi predstavljaju razlike između buđenja.
    while (current != nullptr &&
           remainingTime > current->sleepTime) {

        remainingTime -= current->sleepTime;
        previous = current;
        current = current->next;
    }

    thread->sleepTime = remainingTime;
    thread->next = current;

    if (current != nullptr) {
        current->sleepTime -= remainingTime;
    }

    if (previous == nullptr) {
        sleepingHead = thread;
    }
    else {
        previous->next = thread;
    }
}

int _thread::sleep(uint64 time)
{
    if (time == 0) {
        return 0;
    }

    running->blocked = true;
    running->sleepTime = time;

    addToSleepList(running);

    dispatch();

    return 0;
}

void _thread::timerTick(bool allowPreemption)
{
    if (sleepingHead != nullptr) {
        // Smanjujemo samo vreme prve niti.
        if (sleepingHead->sleepTime > 0) {
            sleepingHead->sleepTime--;
        }

        // Budimo sve niti kojima je isteklo vreme.
        while (sleepingHead != nullptr &&
               sleepingHead->sleepTime == 0) {

            _thread* awakened = sleepingHead;
            sleepingHead = sleepingHead->next;

            awakened->next = nullptr;
            awakened->blocked = false;

            Scheduler::put(awakened);
        }
    }

    // Sistemski kod trenutno ne preotimamo.
    if (!allowPreemption) {
        return;
    }

    timeSliceCounter++;

    if (timeSliceCounter >= DEFAULT_TIME_SLICE) {
        dispatch();
    }
}
