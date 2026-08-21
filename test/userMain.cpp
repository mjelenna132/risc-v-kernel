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

/*void userMain() {
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

}*/

void userMain()
{
    testSemaphoreN();
    testPeriodic();
    return;

    // Postojeci kod...
}