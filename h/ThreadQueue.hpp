#ifndef _threadQueue_hpp
#define _threadQueue_hpp

class TCB;

// FIFO red niti. Intruzivan je: ulancava preko TCB::next, pa ne alocira nista.
// Nit je u najvise jednom ovakvom redu (spremne, blokirane na jednom semaforu,
// ili ugasene). Red uspavanih koristi drugi pokazivac, videti SleepingQueue.
class ThreadQueue
{
public:
    constexpr ThreadQueue() : head(nullptr), tail(nullptr) {}

    ThreadQueue(const ThreadQueue&) = delete;

    ThreadQueue& operator=(const ThreadQueue&) = delete;

    void put(TCB* thread);
    TCB* get();

    // Izbacuje nit sa proizvoljnog mesta; netacno ako nije bila u redu.
    bool remove(TCB* thread);

    TCB* peek() const { return head; }
    bool isEmpty() const { return head == nullptr; }

private:
    TCB* head;
    TCB* tail;
};

#endif
