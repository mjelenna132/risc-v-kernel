#ifndef OS_PROJECT_SYSCALL_CPP_HPP
#define OS_PROJECT_SYSCALL_CPP_HPP

#include "syscall_c.hpp"

void* operator new(size_t size);
void operator delete(void* ptr) noexcept;

class Thread {
public:
    Thread(void (*body)(void*), void* arg);
    virtual ~Thread();

    int start();

    static void dispatch();
    static int sleep(time_t time);

protected:
    Thread();

    virtual void run() {}

private:
    thread_t myHandle;
    void (*body)(void*);
    void* arg;

    // Poziva run() kod izvedene klase.
    static void runWrapper(void* thread);
};

class PeriodicThread : public Thread {
public:
    void terminate();

protected:
    PeriodicThread(time_t period);
    virtual void periodicActivation() {}

private:
    time_t period;

    static void periodicWrapper(void* argument);
};

class Semaphore {
public:
    Semaphore(unsigned init = 1);
    virtual ~Semaphore();

    int wait();
    int signal();

private:
    sem_t myHandle;
};

class Console {
public:
    static char getc();
    static void putc(char character);
};
#endif // OS_PROJECT_SYSCALL_CPP_HPP