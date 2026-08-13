//
// Created by jelena on 8/13/26.
//

#ifndef OS_PROJECT_SCHEDULER_HPP
#define OS_PROJECT_SCHEDULER_HPP

class _thread;

class Scheduler {
public:
    // Dodaje nit na kraj reda spremnih niti.
    static void put(_thread* thread);

    // Uzima prvu spremnu nit.
    static _thread* get();

private:
    // Prva i poslednja nit u redu.
    static _thread* head;
    static _thread* tail;
};
#endif //OS_PROJECT_SCHEDULER_HPP
