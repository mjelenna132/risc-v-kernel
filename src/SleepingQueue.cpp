#include "../h/SleepingQueue.hpp"
#include "../h/TCB.hpp"

void SleepingQueue::put(TCB* thread, time_t periods)
{
    if (thread == nullptr) { return; }

    // Trazi mesto za nit, oduzimajuci usput vremena niti koje se bude pre nje.
    TCB** position = &head;
    while (*position != nullptr && (*position)->sleepingTime <= periods) {
        periods -= (*position)->sleepingTime;
        position = &(*position)->nextSleeping;
    }

    thread->sleepingTime = periods;
    thread->nextSleeping = *position;
    *position = thread;

    // Sledeca se sada budi za toliko posle nove.
    if (thread->nextSleeping != nullptr) {
        thread->nextSleeping->sleepingTime -= periods;
    }
}

bool SleepingQueue::remove(TCB* thread)
{
    if (thread == nullptr) { return false; }

    TCB** position = &head;
    while (*position != nullptr && *position != thread) {
        position = &(*position)->nextSleeping;
    }
    if (*position == nullptr) { return false; }

    *position = thread->nextSleeping;

    // Vremena su relativna, pa sledeca preuzima i vreme uklonjene niti.
    if (*position != nullptr) { (*position)->sleepingTime += thread->sleepingTime; }

    thread->nextSleeping = nullptr;
    thread->sleepingTime = 0;
    return true;
}

void SleepingQueue::tick()
{
    if (head == nullptr) { return; }

    head->sleepingTime--;

    // Niti zakazane za isti trenutak nose vreme nula i stoje odmah iza prve.
    while (head != nullptr && head->sleepingTime == 0) {
        TCB* awakened = head;
        head = head->nextSleeping;

        awakened->nextSleeping = nullptr;
        TCB::wakeFromSleep(awakened);
    }
}
