//
// Created by jelena on 8/13/26.
//
#include "../h/syscall_cpp.hpp"


void* operator new(size_t size) {
    return mem_alloc(size);
}

void operator delete(void* ptr) noexcept {
    if (ptr != nullptr) {
        mem_free(ptr);
    }
}

Thread::Thread(void (*body)(void*), void* arg)
    : myHandle(nullptr), body(body), arg(arg) {
}

Thread::Thread()
    : myHandle(nullptr), body(nullptr), arg(nullptr) {
}

Thread::~Thread() {
    myHandle = nullptr;
}

int Thread::start() {
    // Isti objekat ne pokrećemo dva puta.
    if (myHandle != nullptr) {
        return -1;
    }

    if (body != nullptr) {
        // Konstruktor je dobio običnu funkciju.
        return thread_create(&myHandle, body, arg);
    }

    // Izvedena klasa je redefinisala run().
    return thread_create(&myHandle, runWrapper, this);
}

void Thread::runWrapper(void* thread) {
    Thread* object = (Thread*)thread;
    object->run();
}

void Thread::dispatch() {
    thread_dispatch();
}

int Thread::sleep(time_t time) {

    return time_sleep(time);
}

Semaphore::Semaphore(unsigned init)
    : myHandle(nullptr)
{
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore()
{
    if (myHandle != nullptr) {
        sem_close(myHandle);
        myHandle = nullptr;
    }
}

int Semaphore::wait()
{
    return sem_wait(myHandle);
}

int Semaphore::signal()
{
    return sem_signal(myHandle);
}

char Console::getc()
{
    return ::getc();
}

void Console::putc(char character)
{
    ::putc(character);
}


PeriodicThread::PeriodicThread(time_t period)
    : Thread(),
      period(period)
{
}

void PeriodicThread::run()
{
    while (period != 0) {
        periodicActivation();

        // periodicActivation() je možda pozvao terminate().
        if (period != 0) {
            Thread::sleep(period);
        }
    }
}

void PeriodicThread::terminate()
{
    // Nula označava da više nema novih aktivacija.
    period = 0;
}