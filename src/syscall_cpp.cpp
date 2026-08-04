#include "../h/syscall_cpp.hpp"

Thread::Thread(void (*body)(void*), void* arg)
        : myHandle(nullptr), body(body), arg(arg) {}

Thread::Thread()
        : myHandle(nullptr), body(nullptr), arg(nullptr) {}

Thread::~Thread() {}

void Thread::wrapper(void* arg)
{
    Thread* self = (Thread*) arg;
    if (self->body != nullptr) {
        self->body(self->arg);
    } else {
        self->run();
    }
}

int Thread::start()
{
    return thread_create(&myHandle, &Thread::wrapper, this);
}

void Thread::dispatch()
{
    thread_dispatch();
}

int Thread::sleep(time_t period)
{
    return time_sleep(period);
}

Semaphore::Semaphore(unsigned init)
        : myHandle(nullptr)
{
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore()
{
    sem_close(myHandle);
}

int Semaphore::wait()
{
    return sem_wait(myHandle);
}

int Semaphore::signal()
{
    return sem_signal(myHandle);
}

int Semaphore::trywait()
{
    return sem_trywait(myHandle);
}

int Semaphore::timedwait(time_t periods)
{
    return sem_timedwait(myHandle, periods);
}

PeriodicThread::PeriodicThread(time_t period)
        : Thread(), period(period) {}

void PeriodicThread::run()
{
    // Klasa ne sme da dobije nove podatke clanove, pa se zaustavljanje oznacava
    // samom periodom: perioda nula ionako ne opisuje smislenu periodicnu nit.
    while (period != 0) {
        periodicActivation();

        // Provera opet, ako je terminate pozvan iz same aktivacije.
        if (period != 0) { sleep(period); }
    }
}

void PeriodicThread::terminate()
{
    period = 0;
}

char Console::getc()
{
    return ::getc();
}

void Console::putc(char c)
{
    ::putc(c);
}
