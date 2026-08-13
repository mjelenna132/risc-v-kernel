#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/Riscv.hpp"
#include "../h/syscall_c.hpp"

_thread* _thread::running = nullptr;

void* _thread::operator new(size_t size) noexcept {
    size_t blockCount =
        (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    return MemoryAllocator::mem_alloc(blockCount);
}

void _thread::operator delete(void* ptr) noexcept {
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
      next(nullptr) {
}

void _thread::initialize() {
    if (running == nullptr) {
        // Main već radi, pa mu ne pravimo novi stek.
        running = new _thread(nullptr, nullptr, nullptr);
    }
}

int _thread::create(_thread** handle, Body body,
                    void* arg, uint64* stackTop) {
    if (handle == nullptr || body == nullptr ||
        stackTop == nullptr) {
        return -1;
    }

    _thread* thread = new _thread(body, arg, stackTop);

    if (thread == nullptr) {
        return -2;
    }

    *handle = thread;
    Scheduler::put(thread);

    return 0;
}

int _thread::exit() {
    if (running == nullptr) {
        return -1;
    }

    // Završenu nit više ne vraćamo u Scheduler.
    running->finished = true;
    dispatch();

    return 0;
}

void _thread::dispatch() {
    _thread* old = running;

    // Nezavršena nit se vraća na kraj reda.
    if (old != nullptr && !old->finished) {
        Scheduler::put(old);
    }

    running = Scheduler::get();

    // Nema druge niti za izvršavanje.
    if (running == nullptr) {
        running = old;
        return;
    }

    // Ako je izabrana ista nit, nema promene.
    if (running == old) {
        return;
    }

    contextSwitch((uint64*)&old->context,
                  (uint64*)&running->context);
}

void _thread::threadWrapper()
{
    // Korisnička funkcija niti mora da radi u korisničkom režimu
    Riscv::popSppSpie();

    // Pokrećemo funkciju ove niti
    running->body(running->arg);

    // Kada se funkcija završi, gasimo nit sistemskim pozivom
    thread_exit();
}
// Created by jelena on 8/13/26.
//
