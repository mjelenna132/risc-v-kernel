#include "../h/Console.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/Riscv.hpp"
#include "../h/TCB.hpp"
#include "../h/syscall_abi.hpp"

void userMain();

// Korisnicki program je i sam nit, pa radi u U-modu kao i sve ostale.
static void userMainBody(void*)
{
    userMain();
}

int main()
{
    MemoryAllocator::init();
    Riscv::initTrap();

    if (TCB::createMainThread() == nullptr) {
        Riscv::panic("neuspesna inicijalizacija jezgra");
    }

    _Console::init();

    void* stack = MemoryAllocator::mem_alloc(DEFAULT_STACK_SIZE);
    if (stack == nullptr
        || TCB::createThread(userMainBody, nullptr,
                             (void*) ((uint64) stack + DEFAULT_STACK_SIZE))
           == nullptr) {
        Riscv::panic("neuspesno pokretanje korisnickog programa");
    }

    // Tek sada, kad jezgro ume da obradi prekid.
    Riscv::enableInterrupts();

    // Glavna nit je ujedno i idle nit: vrteci se ovde drzi red spremnih niti
    // nepraznim, pa Scheduler::get() nikad ne vrati nullptr. Procesor ustupa
    // sistemskim pozivom, a ne direktno, da bi i ona u jezgro ulazila sa
    // maskiranim prekidima kao i svaka druga nit.
    while (TCB::getActiveThreads() > 0) { abiSyscall(SYS_THREAD_DISPATCH); }

    _Console::flush();

    // Povratak iz main-a bi zavrsio u beskonacnoj petlji startup koda.
    Riscv::halt();
}
