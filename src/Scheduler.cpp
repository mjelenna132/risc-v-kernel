//
// Created by jelena on 8/13/26.
//
#include "../h/Scheduler.hpp"
#include "../h/TCB.hpp"

_thread* Scheduler::head = nullptr;
_thread* Scheduler::tail = nullptr;

void Scheduler::put(_thread* thread) {
    if (thread == nullptr) {
        return;
    }

    thread->next = nullptr;

    // Red je prazan.
    if (tail == nullptr) {
        head = tail = thread;
    } else {
        // Dodavanje na kraj reda.
        tail->next = thread;
        tail = thread;
    }
}

_thread* Scheduler::get() {
    // Nema spremnih niti.
    if (head == nullptr) {
        return nullptr;
    }

    // Uzimamo prvu nit.
    _thread* thread = head;
    head = head->next;

    if (head == nullptr) {
        tail = nullptr;
    }

    thread->next = nullptr;
    return thread;
}