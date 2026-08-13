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
    static int sleep(time_t);

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

#endif // OS_PROJECT_SYSCALL_CPP_HPP