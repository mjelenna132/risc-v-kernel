#include "printing.hpp"
#include "../h/syscall_cpp.hpp"

#define LEVEL_1_IMPLEMENTED 1
#define LEVEL_2_IMPLEMENTED 1
#define LEVEL_3_IMPLEMENTED 1
#define LEVEL_4_IMPLEMENTED 1

#if LEVEL_2_IMPLEMENTED == 1
// TEST 1 (zadatak 2, niti C API i sinhrona promena konteksta)
#include "../test/Threads_C_API_test.hpp"
// TEST 2 (zadatak 2., niti CPP API i sinhrona promena konteksta)
#include "../test/Threads_CPP_API_test.hpp"
// TEST 7 (zadatak 2., testiranje da li se korisnicki kod izvrsava u korisnickom rezimu)
#include "../test/System_Mode_test.hpp"
#endif

#if LEVEL_3_IMPLEMENTED == 1
// TEST 3 (zadatak 3., kompletan C API sa semaforima, sinhrona promena konteksta)
#include "../test/ConsumerProducer_C_API_test.hpp"
// TEST 4 (zadatak 3., kompletan CPP API sa semaforima, sinhrona promena konteksta)
#include "../test/ConsumerProducer_CPP_Sync_API_test.hpp"
#endif

#if LEVEL_4_IMPLEMENTED == 1
// TEST 5 (zadatak 4., thread_sleep test C API)
#include "../test/ThreadSleep_C_API_test.hpp"
// TEST 6 (zadatak 4. CPP API i asinhrona promena konteksta)
#include "../test/ConsumerProducer_CPP_API_test.hpp"
#include "System_Mode_test.hpp"

#endif

#if 0
static sem_t testSemaphore;
static volatile bool newSemaphoreTestFinished = false;

static void waitForThree(void*)
{
    printString("Nit: cekam 3 jedinice\n");

    int result = sem_wait_n(testSemaphore, 3);

    if (result == 0) {
        printString("Nit: dobila sam 3 jedinice\n");
    }
    else {
        printString("GRESKA: sem_wait_n nije uspeo\n");
    }

    newSemaphoreTestFinished = true;
}

static void testSemaphoreN()
{
    printString("Pocetak testa sem_wait_n/sem_signal_n\n");

    sem_open(&testSemaphore, 0);

    thread_t thread;
    thread_create(&thread, waitForThree, nullptr);

    // Dajemo niti priliku da pozove sem_wait_n i blokira se.
    thread_dispatch();

    printString("Glavna nit: dodajem 2 jedinice\n");
    sem_signal_n(testSemaphore, 2);

    // Nit još ne sme da se probudi jer je tražila 3.
    thread_dispatch();

    if (newSemaphoreTestFinished) {
        printString("GRESKA: nit se prerano probudila\n");
    }
    else {
        printString("Dobro: nit i dalje ceka\n");
    }

    printString("Glavna nit: dodajem jos 1 jedinicu\n");
    sem_signal_n(testSemaphore, 1);

    // Čekamo da probuđena nit završi.
    while (!newSemaphoreTestFinished) {
        thread_dispatch();
    }

    sem_close(testSemaphore);

    printString("KRAJ TESTA SEMAFORA\n");
}
#endif
static volatile bool periodicTestFinished = false;

class TestPeriodicThread : public PeriodicThread {
public:
    TestPeriodicThread()
        : PeriodicThread(3),
          activationCount(0)
    {
    }

protected:
    void periodicActivation() override
    {
        activationCount++;

        printString("Periodicna aktivacija broj ");
        printInt(activationCount);
        printString("\n");

        if (activationCount == 3) {
            terminate();
            periodicTestFinished = true;
        }
    }

private:
    int activationCount;
};

static void testPeriodic()
{
    printString("POCETAK PERIODICNOG TESTA\n");

    periodicTestFinished = false;

    TestPeriodicThread* thread = new TestPeriodicThread();
    thread->start();

    while (!periodicTestFinished) {
        Thread::dispatch();
    }

    printString("KRAJ PERIODICNOG TESTA\n");
}

static volatile bool stopSpinner = false;
static volatile bool spinnerFinished = false;
static volatile uint64 spinCount = 0;

static void spinnerBody(void*)
{
    // Nema sistemskih poziva, blokiranja ni dispatch-a.
    while (!stopSpinner) {
        spinCount++;
    }

    spinnerFinished = true;
}

static void testTimerPreemption()
{
    stopSpinner = false;
    spinnerFinished = false;
    spinCount = 0;

    printString("POCETAK TESTA PREOTIMANJA\n");

    thread_t spinner = nullptr;

    if (thread_create(&spinner, spinnerBody, nullptr) < 0) {
        printString("GRESKA: kreiranje niti nije uspelo\n");
        return;
    }

    // Kada spinner počne da vrti petlju, tajmer mora
    // da mu oduzme procesor da bismo nastavili ovde.
    while (spinCount < 100000) {
        thread_dispatch();
    }

    stopSpinner = true;

    while (!spinnerFinished) {
        thread_dispatch();
    }

    printString("PREOTIMANJE: PROSLO\n");
}

static void delayedWorker(void*)
{
    Thread::sleep(20);
    printString("DETE: probudilo se i zavrsava\n");
}

/*
void userMain() {

    printString("Unesite broj testa? [1-7]\n");
    int test = getc() - '0';
    getc(); // Enter posle broja

    if ((test >= 1 && test <= 2) || test == 7) {
        if (LEVEL_2_IMPLEMENTED == 0) {
            printString("Nije navedeno da je zadatak 2 implementiran\n");
            return;
        }
    }

    if (test >= 3 && test <= 4) {
        if (LEVEL_3_IMPLEMENTED == 0) {
            printString("Nije navedeno da je zadatak 3 implementiran\n");
            return;
        }
    }

    if (test >= 5 && test <= 6) {
        if (LEVEL_4_IMPLEMENTED == 0) {
            printString("Nije navedeno da je zadatak 4 implementiran\n");
            return;
        }
    }

    switch (test) {
        case 1:
#if LEVEL_2_IMPLEMENTED == 1
            Threads_C_API_test();
            printString("TEST 1 (zadatak 2, niti C API i sinhrona promena konteksta)\n");
#endif
            break;
        case 2:
#if LEVEL_2_IMPLEMENTED == 1
            Threads_CPP_API_test();
            printString("TEST 2 (zadatak 2., niti CPP API i sinhrona promena konteksta)\n");
#endif
            break;
        case 3:
#if LEVEL_3_IMPLEMENTED == 1
            producerConsumer_C_API();
            printString("TEST 3 (zadatak 3., kompletan C API sa semaforima, sinhrona promena konteksta)\n");
#endif
            break;
        case 4:
#if LEVEL_3_IMPLEMENTED == 1
            producerConsumer_CPP_Sync_API();
            printString("TEST 4 (zadatak 3., kompletan CPP API sa semaforima, sinhrona promena konteksta)\n");
#endif
            break;
        case 5:
#if LEVEL_4_IMPLEMENTED == 1
            testSleeping();
            printString("TEST 5 (zadatak 4., thread_sleep test C API)\n");
#endif
            break;
        case 6:
#if LEVEL_4_IMPLEMENTED == 1
            testConsumerProducer();
            printString("TEST 6 (zadatak 4. CPP API i asinhrona promena konteksta)\n");
#endif
            break;
        case 7:
#if LEVEL_2_IMPLEMENTED == 1
            System_Mode_test();
            printString("Test se nije uspesno zavrsio\n");
            printString("TEST 7 (zadatak 2., testiranje da li se korisnicki kod izvrsava u korisnickom rezimu)\n");
#endif
            break;
        default:
            printString("Niste uneli odgovarajuci broj za test\n");
    }

}
*/
static sem_t closingSemaphore;

struct CloseWaiter {
    volatile bool finished;
    int result;
};

static CloseWaiter closeWaiters[2];

static void closeWaiterBody(void* argument)
{
    CloseWaiter* waiter = (CloseWaiter*)argument;

    waiter->result = sem_wait(closingSemaphore);
    waiter->finished = true;
}

static void testSemaphoreClose()
{
    printString("POCETAK TESTA SEM_CLOSE\n");

    if (sem_open(&closingSemaphore, 0) < 0) {
        printString("GRESKA: sem_open\n");
        return;
    }

    int created = 0;

    for (int i = 0; i < 2; i++) {
        closeWaiters[i].finished = false;
        closeWaiters[i].result = 0;

        thread_t handle = nullptr;

        if (thread_create(
                &handle, closeWaiterBody, &closeWaiters[i]) < 0) {
            printString("GRESKA: thread_create\n");
            break;
                }

        created++;
    }

    // Dajemo nitima vreme da stignu do sem_wait i blokiraju se.
    Thread::sleep(2);

    bool passed = (created == 2);

    for (int i = 0; i < created; i++) {
        if (closeWaiters[i].finished) {
            printString("GRESKA: nit nije cekala zatvaranje\n");
            passed = false;
        }
    }

    if (sem_close(closingSemaphore) < 0) {
        printString("GRESKA: sem_close\n");
        return;
    }

    for (int i = 0; i < created; i++) {
        while (!closeWaiters[i].finished) {
            thread_dispatch();
        }

        if (closeWaiters[i].result >= 0) {
            passed = false;
        }
    }

    printString(passed
        ? "SEM_CLOSE: PROSLO\n"
        : "SEM_CLOSE: GRESKA\n");
}
static sem_t multiSemaphore;

struct MultiWaiter {
    unsigned units;
    int result;
    volatile bool finished;
};

static MultiWaiter multiWaiters[2];

static void multiWaiterBody(void* argument)
{
    MultiWaiter* waiter = (MultiWaiter*)argument;

    waiter->result = sem_wait_n(multiSemaphore, waiter->units);
    waiter->finished = true;
}

static void testSemaphoreMultiple()
{
    printString("POCETAK TESTA SEM_N VISE NITI\n");

    if (sem_open(&multiSemaphore, 0) < 0) {
        printString("GRESKA: sem_open\n");
        return;
    }

    bool passed = true;

    // Nula ne sme da blokira niti da promeni vrednost.
    if (sem_wait_n(multiSemaphore, 0) != 0) {
        passed = false;
    }

    if (sem_signal_n(multiSemaphore, 0) != 0) {
        passed = false;
    }

    int created = 0;

    for (int i = 0; i < 2; i++) {
        multiWaiters[i].units = i + 2; // Prva trazi 2, druga 3.
        multiWaiters[i].result = -999;
        multiWaiters[i].finished = false;

        thread_t handle = nullptr;

        if (thread_create(
                &handle, multiWaiterBody, &multiWaiters[i]) < 0) {
            printString("GRESKA: thread_create\n");
            passed = false;
            break;
        }

        created++;
    }

    Thread::sleep(2);

    for (int i = 0; i < created; i++) {
        if (multiWaiters[i].finished) {
            printString("GRESKA: prerano budjenje\n");
            passed = false;
        }
    }

    // Jedan poziv treba da obezbedi resurse za obe niti.
    if (sem_signal_n(multiSemaphore, 5) != 0) {
        printString("GRESKA: sem_signal_n\n");
        sem_close(multiSemaphore);
        return;
    }

    for (int i = 0; i < created; i++) {
        while (!multiWaiters[i].finished) {
            thread_dispatch();
        }

        if (multiWaiters[i].result != 0) {
            passed = false;
        }
    }

    if (sem_close(multiSemaphore) != 0) {
        passed = false;
    }

    printString(passed
        ? "SEM_N VISE NITI: PROSLO\n"
        : "SEM_N VISE NITI: GRESKA\n");
}

static volatile bool shortThreadFinished = false;
static bool memoryFreeError = false;

// Privremeno povezujemo alocirane blokove u listu.
struct MemoryProbe {
    MemoryProbe* next;
};

static unsigned countAvailableBlocks()
{
    MemoryProbe* head = nullptr;
    unsigned count = 0;

    while (true) {
        MemoryProbe* block = (MemoryProbe*)mem_alloc(4096);

        if (block == nullptr) {
            break;
        }

        block->next = head;
        head = block;
        count++;
    }

    // Vracamo svu memoriju koju je provera zauzela.
    while (head != nullptr) {
        MemoryProbe* next = head->next;

        if (mem_free(head) != 0) {
            memoryFreeError = true;
        }

        head = next;
    }

    return count;
}

static void shortThreadBody(void*)
{
    shortThreadFinished = true;
    // Povratak iz funkcije zavrsava nit.
}

static void testThreadMemory()
{
    printString("POCETAK TESTA MEMORIJE NITI\n");

    // Prethodne test-niti dobijaju vreme da zavrse.
    Thread::sleep(2);

    memoryFreeError = false;
    unsigned before = countAvailableBlocks();
    unsigned created = 0;

    for (unsigned i = 0; i < 256; i++) {
        shortThreadFinished = false;
        thread_t handle = nullptr;

        if (thread_create(
                &handle, shortThreadBody, nullptr) < 0) {
            printString("GRESKA: kreiranje kratke niti\n");
            break;
        }

        created++;

        while (!shortThreadFinished) {
            thread_dispatch();
        }

        // Dajemo niti priliku da dovrsi izlazak.
        thread_dispatch();
    }

    // Oznaka finished se postavlja pre stvarnog izlaska iz niti.
    Thread::sleep(2);

    unsigned after = countAvailableBlocks();

    printString("Dostupni blokovi pre: ");
    printInt(before);
    printString("\nDostupni blokovi posle: ");
    printInt(after);
    printString("\n");

    if (created == 256 &&
        before > 0 &&
        before == after &&
        !memoryFreeError) {
        printString("MEMORIJA NITI: PROSLO\n");
    }
    else {
        printString("MEMORIJA NITI: GRESKA\n");
    }
}
    void userMain()
    {

        testTimerPreemption();
        testSemaphoreClose();
        testSemaphoreMultiple();
        testThreadMemory();
        testPeriodic();
        thread_t child = nullptr;

        if (thread_create(&child, delayedWorker, nullptr) < 0) {
            printString("GRESKA: kreiranje deteta\n");
            return;
        }

        printString("USERMAIN: zavrsavam\n");
        return;

        // Postojeci kod za izbor testova ostaje ispod.
    }

    // Postojeći kod za izbor testova ostaje ispod.
