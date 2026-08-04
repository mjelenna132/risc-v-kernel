#include "../h/Semaphore.hpp"
#include "../h/TCB.hpp"

int _Semaphore::open(_Semaphore** handle, unsigned init)
{
    if (handle == nullptr) { return INVALID_HANDLE; }

    _Semaphore* semaphore = new _Semaphore(init);
    if (semaphore == nullptr) { return NO_MEMORY; }

    *handle = semaphore;
    return 0;
}

int _Semaphore::close(_Semaphore* handle)
{
    if (handle == nullptr) { return INVALID_HANDLE; }

    delete handle;
    return 0;
}

_Semaphore::~_Semaphore()
{
    for (TCB* thread = blocked.get(); thread != nullptr; thread = blocked.get()) {
        // Ako je cekala sa ogranicenim vremenom, stoji i u redu uspavanih.
        TCB::cancelTimeout(thread);
        thread->setSemaphoreClosed(true);
        TCB::unblock(thread);
    }
}

int _Semaphore::wait(unsigned units)
{
    if (units == 0) { return 0; }

    if (blocked.isEmpty() && value >= units) {
        value -= units;
        return 0;
    }

    TCB* caller = TCB::running;
    caller->setRequest(units);
    caller->setSemaphoreClosed(false);
    blocked.put(caller);
    TCB::block();

    // Signal je vec oduzeo jedinice u ime ove niti. Ovde se stize i ako je
    // semafor u medjuvremenu dealociran.
    return caller->isSemaphoreClosed() ? SEMAPHORE_CLOSED : 0;
}

int _Semaphore::tryWait(unsigned units)
{
    if (units == 0) { return 0; }

    if (blocked.isEmpty() && value >= units) {
        value -= units;
        return 0;
    }

    return WOULD_BLOCK;
}

int _Semaphore::timedWait(unsigned units, time_t periods)
{
    if (units == 0) { return 0; }

    if (blocked.isEmpty() && value >= units) {
        value -= units;
        return 0;
    }

    if (periods == 0) { return TIMED_OUT; }

    TCB* caller = TCB::running;
    caller->setRequest(units);
    caller->setSemaphoreClosed(false);
    blocked.put(caller);

    // Od sada je nit u dva reda; prvi dogadjaj je skida sa onog drugog.
    TCB::blockWithTimeout(this, periods);

    if (caller->isSemaphoreClosed()) { return SEMAPHORE_CLOSED; }
    if (caller->hasTimedOut()) { return TIMED_OUT; }

    return 0;
}

int _Semaphore::signal(unsigned units)
{
    value += units;
    releaseWaiting();
    return 0;
}

void _Semaphore::releaseWaiting()
{
    for (TCB* first = blocked.peek();
         first != nullptr && value >= first->getRequest();
         first = blocked.peek()) {
        value -= first->getRequest();

        TCB* thread = blocked.get();
        TCB::cancelTimeout(thread);     // probudio ju je signal, ne tajmer
        TCB::unblock(thread);
    }
}
