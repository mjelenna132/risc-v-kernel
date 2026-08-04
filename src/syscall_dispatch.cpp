#include "../h/syscall_dispatch.hpp"
#include "../h/Console.hpp"
#include "../h/Semaphore.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/TCB.hpp"

static const int64 INVALID_ARGUMENT = -1;

static int64 semaphoreWait(uint64 handle, unsigned units)
{
    _Semaphore* semaphore = (_Semaphore*) handle;
    if (semaphore == nullptr) { return INVALID_ARGUMENT; }

    return semaphore->wait(units);
}

static int64 semaphoreSignal(uint64 handle, unsigned units)
{
    _Semaphore* semaphore = (_Semaphore*) handle;
    if (semaphore == nullptr) { return INVALID_ARGUMENT; }

    return semaphore->signal(units);
}

static int64 semaphoreTryWait(uint64 handle, unsigned units)
{
    _Semaphore* semaphore = (_Semaphore*) handle;
    if (semaphore == nullptr) { return INVALID_ARGUMENT; }

    return semaphore->tryWait(units);
}

static int64 semaphoreTimedWait(uint64 handle, unsigned units, time_t periods)
{
    _Semaphore* semaphore = (_Semaphore*) handle;
    if (semaphore == nullptr) { return INVALID_ARGUMENT; }

    return semaphore->timedWait(units, periods);
}

int64 dispatchSyscall(uint64 code, uint64 a1, uint64 a2, uint64 a3, uint64 a4)
{
    switch (code) {
        case SYS_MEM_ALLOC:
            return (int64) MemoryAllocator::mem_alloc((size_t) a1 * MEM_BLOCK_SIZE);

        case SYS_MEM_FREE:
            return MemoryAllocator::mem_free((void*) a1);

        case SYS_MEM_GET_FREE_SPACE:
            return (int64) MemoryAllocator::getFreeSpace();

        case SYS_MEM_GET_LARGEST_FREE_BLOCK:
            return (int64) MemoryAllocator::getLargestFreeBlock();

        case SYS_THREAD_CREATE: {
            if (a1 == 0) { return INVALID_ARGUMENT; }

            TCB* thread = TCB::createThread((TCB::Body) a2, (void*) a3, (void*) a4);
            if (thread == nullptr) { return INVALID_ARGUMENT; }

            *(TCB**) a1 = thread;
            return 0;
        }

        case SYS_THREAD_EXIT:
            TCB::exit();
            return 0;

        case SYS_THREAD_DISPATCH:
            TCB::dispatch();
            return 0;

        case SYS_SEM_OPEN: {
            if (a1 == 0) { return INVALID_ARGUMENT; }

            _Semaphore* semaphore;
            int64 status = _Semaphore::open(&semaphore, (unsigned) a2);
            if (status == 0) { *(_Semaphore**) a1 = semaphore; }
            return status;
        }

        case SYS_SEM_CLOSE:
            return _Semaphore::close((_Semaphore*) a1);

        case SYS_SEM_WAIT:
            return semaphoreWait(a1, 1);

        case SYS_SEM_SIGNAL:
            return semaphoreSignal(a1, 1);

        case SYS_SEM_WAIT_N:
            return semaphoreWait(a1, (unsigned) a2);

        case SYS_SEM_SIGNAL_N:
            return semaphoreSignal(a1, (unsigned) a2);

        case SYS_SEM_TRYWAIT:
            return semaphoreTryWait(a1, (unsigned) a2);

        case SYS_SEM_TIMEDWAIT:
            return semaphoreTimedWait(a1, (unsigned) a2, (time_t) a3);

        case SYS_TIME_SLEEP:
            TCB::sleep((time_t) a1);
            return 0;

        case SYS_GETC:
            return (int64) (unsigned char) _Console::getc();

        case SYS_PUTC:
            _Console::putc((char) a1);
            return 0;

        // Samo za interne niti jezgra; korisnickoj niti se vraca greska.
        case SYS_CONSOLE_NEXT_OUTPUT:
            if (!TCB::running->isSystemThread()) { return INVALID_ARGUMENT; }
            return (int64) (unsigned char) _Console::nextOutputChar();

        case SYS_CONSOLE_OUTPUT_SENT:
            if (!TCB::running->isSystemThread()) { return INVALID_ARGUMENT; }
            _Console::outputCharSent();
            return 0;

        default:
            return INVALID_ARGUMENT;
    }
}
