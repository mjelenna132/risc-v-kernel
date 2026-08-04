#include "../h/Scheduler.hpp"

ThreadQueue Scheduler::readyThreadQueue;

TCB* Scheduler::get()
{
    return readyThreadQueue.get();
}

void Scheduler::put(TCB* thread)
{
    readyThreadQueue.put(thread);
}
