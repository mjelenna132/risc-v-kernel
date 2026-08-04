#ifndef _scheduler_hpp
#define _scheduler_hpp

#include "ThreadQueue.hpp"

class TCB;

// Rasporedjivac: red spremnih niti, rasporedjivanje u ciklusu (round-robin).
class Scheduler
{
public:
    static TCB* get();
    static void put(TCB* thread);

private:
    static ThreadQueue readyThreadQueue;
};

#endif
