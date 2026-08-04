#ifndef _sleepingQueue_hpp
#define _sleepingQueue_hpp

#include "../lib/hw.h"

class TCB;

// Red uspavanih niti, uredjen po trenutku budjenja. Cuvaju se RELATIVNA vremena
// (koliko posle niti ispred sebe), pa tick() azurira samo prvu nit u redu.
//
// Ulancava preko TCB::nextSleeping, a ne preko TCB::next, da bi nit koja ceka
// na semaforu sa ogranicenim vremenom mogla da bude u oba reda odjednom.
class SleepingQueue
{
public:
    constexpr SleepingQueue() : head(nullptr) {}

    SleepingQueue(const SleepingQueue&) = delete;

    SleepingQueue& operator=(const SleepingQueue&) = delete;

    void put(TCB* thread, time_t periods);

    // Izbacuje nit pre isteka vremena, kad je probudi neki drugi dogadjaj.
    bool remove(TCB* thread);

    // Jedna perioda tajmera; budi sve niti kojima je vreme isteklo.
    void tick();

private:
    TCB* head;
};

#endif
