#include "../h/ThreadQueue.hpp"
#include "../h/TCB.hpp"

void ThreadQueue::put(TCB* thread)
{
    if (thread == nullptr) { return; }

    thread->next = nullptr;
    if (tail) { tail->next = thread; }
    else      { head = thread; }
    tail = thread;
}

TCB* ThreadQueue::get()
{
    if (!head) { return nullptr; }

    TCB* thread = head;
    head = head->next;
    if (!head) { tail = nullptr; }

    thread->next = nullptr;
    return thread;
}

bool ThreadQueue::remove(TCB* thread)
{
    if (thread == nullptr) { return false; }

    TCB* previous = nullptr;
    TCB** position = &head;
    while (*position != nullptr && *position != thread) {
        previous = *position;
        position = &(*position)->next;
    }
    if (*position == nullptr) { return false; }

    *position = thread->next;
    if (tail == thread) { tail = previous; }

    thread->next = nullptr;
    return true;
}
