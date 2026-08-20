#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/Riscv.hpp"
#include "../h/syscall_c.hpp"

_thread* _thread::running = nullptr;
_thread* _thread::sleepingHead = nullptr;
uint64 _thread::timeSliceCounter = 0;

void* _thread::operator new(size_t size) noexcept {
    size_t blockCount =
        (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    return MemoryAllocator::mem_alloc(blockCount);
}

void _thread::operator delete(void* ptr) noexcept {
    MemoryAllocator::mem_free(ptr);
}

_thread::_thread(Body body, void* arg, uint64* stackTop)
    : body(body),
      arg(arg),
      stack(body != nullptr
                ? stackTop - DEFAULT_STACK_SIZE / sizeof(uint64)
                : nullptr),
      context{
          body != nullptr ? (uint64)&threadWrapper : 0,
          body != nullptr ? (uint64)stackTop : 0
      },
      finished(false),
      blocked(false),
      waitResult(0),
      sleepTime(0),
      next(nullptr) {
}

void _thread::initialize() {
    if (running == nullptr) {
        // Main već radi, pa mu ne pravimo novi stek.
        running = new _thread(nullptr, nullptr, nullptr);
    }
}

int _thread::create(_thread** handle, Body body,
                    void* arg, uint64* stackTop) {
    if (handle == nullptr || body == nullptr ||
        stackTop == nullptr) {
        return -1;
    }

    _thread* thread = new _thread(body, arg, stackTop);

    if (thread == nullptr) {
        return -2;
    }

    *handle = thread;
    Scheduler::put(thread);

    return 0;
}

int _thread::exit() {
    if (running == nullptr) {
        return -1;
    }

    // Završenu nit više ne vraćamo u Scheduler.
    running->finished = true;
    dispatch();

    return 0;
}

void _thread::dispatch() {
    _thread* old = running;

    // Nezavršena nit se vraća na kraj reda.
    if (old != nullptr && !old->finished && !old->blocked) {
        Scheduler::put(old);
    }

    running = Scheduler::get();

    // Nova nit dobija ceo vremenski odsečak.
    timeSliceCounter = 0;

    // Nema druge niti za izvršavanje.
    if (running == nullptr) {
        running = old;
        return;
    }

    // Ako je izabrana ista nit, nema promene.
    if (running == old) {
        return;
    }

    contextSwitch((uint64*)&old->context,
                  (uint64*)&running->context);
}

void _thread::threadWrapper()
{
    // Korisnička funkcija niti mora da radi u korisničkom režimu
    Riscv::popSppSpie();

    // Pokrećemo funkciju ove niti
    running->body(running->arg);

    // Kada se funkcija završi, gasimo nit sistemskim pozivom
    thread_exit();
}
// A(5) -> D(2) -> B(2) -> C(6)
 //D se budi 2 periode posle A, odnosno ukupno za 7 perioda.
 // B se 4 periode posle A, budi za 9 perioda.
void _thread::addToSleepList(_thread* thread)
{
    if (thread == nullptr) {
        return;
    }

    uint64 remainingTime = thread->sleepTime;

    _thread* previous = nullptr;
    _thread* current = sleepingHead;

    // Tražimo mesto na kome treba ubaciti nit.
    while (current != nullptr &&
           remainingTime > current->sleepTime) {

        remainingTime -= current->sleepTime;
        previous = current;
        current = current->next;
           }

    // Čuvamo relativno vreme u odnosu na prethodnu nit.
    thread->sleepTime = remainingTime;
    thread->next = current;

    // Vreme sledeće niti umanjujemo zbog umetnute niti.
    if (current != nullptr) {
        current->sleepTime -= remainingTime;
    }

    if (previous == nullptr) {
        // Nova nit se budi prva.
        sleepingHead = thread;
    }
    else {
        previous->next = thread;
    }
}

int _thread::sleep(uint64 time)
{
    // Spavanje od 0 perioda nema efekta.
    if (time == 0) {
        return 0;
    }

    // Tekuća nit više nije spremna za izvršavanje.
    running->blocked = true;
    running->sleepTime = time;

    // Dodajemo je u listu uspavanih niti.
    addToSleepList(running);

    // Procesor predajemo sledećoj spremnoj niti.
    dispatch();


    // Ovde se vraćamo tek kada se nit probudi.
    return 0;
}

void _thread::timerTick(bool allowPreemption)
{
    // Ažuriranje uspavanih niti.
    if (sleepingHead != nullptr) {

        // Smanjujemo samo vreme prve niti u relativnoj listi.
        if (sleepingHead->sleepTime > 0) {
            sleepingHead->sleepTime--;
        }

        // Budimo sve niti čije je vreme stiglo do nule.
        while (sleepingHead != nullptr &&
               sleepingHead->sleepTime == 0) {

            _thread* awakened = sleepingHead;
            sleepingHead = sleepingHead->next;

            awakened->next = nullptr;
            awakened->blocked = false;

            // Probuđena nit ponovo postaje spremna.
            Scheduler::put(awakened);
               }
    }
    // Kernel ne preotimamo dok obrađuje sistemski poziv.
    if (!allowPreemption) {
        return;
    }
    // Brojimo koliko dugo tekuća nit koristi procesor.
    timeSliceCounter++;

    // Kada istekne njen vremenski odsečak, menjamo nit.
    if (timeSliceCounter >= DEFAULT_TIME_SLICE) {
        dispatch();
    }
}
// Created by jelena on 8/13/26.
//
