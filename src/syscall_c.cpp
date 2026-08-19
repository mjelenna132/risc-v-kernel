//
// Created by jelena on 8/12/26.
//

#include "../h/syscall_c.hpp"

void* mem_alloc(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    // Pretvaranje bajtova u blokove, uz zaokruživanje naviše.
    size_t blockCount = size / MEM_BLOCK_SIZE;

    if (size % MEM_BLOCK_SIZE != 0) {
        blockCount++;
    }

    // a0 = kod poziva, a1 = argument.
    register uint64 a0 asm("a0") = 0x01;
    register uint64 a1 asm("a1") = blockCount;

    // Prelazak u sistemski režim.
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1)
                 : "memory");

    // Povratna vrednost se nalazi u a0.
    return (void*)a0;
}

int mem_free(void* ptr) {
    if (ptr == nullptr) {
        return -1;
    }

    // a0 = kod poziva, a1 = pokazivač.
    register uint64 a0 asm("a0") = 0x02;
    register uint64 a1 asm("a1") = (uint64)ptr;

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1)
                 : "memory");

    return (int)a0;
}
int thread_create(thread_t* handle,
                  void (*start_routine)(void*),
                  void* arg) {
    if (handle == nullptr || start_routine == nullptr) {
        return -1;
    }

    // Odvajamo stek za novu nit.
    void* stack = mem_alloc(DEFAULT_STACK_SIZE);

    if (stack == nullptr) {
        return -2;
    }

    // Stek raste od viših ka nižim adresama.
    uint64* stackTop =
        (uint64*)stack + DEFAULT_STACK_SIZE / sizeof(uint64);

    register uint64 a0 asm("a0") = 0x11;
    register uint64 a1 asm("a1") = (uint64)handle;
    register uint64 a2 asm("a2") = (uint64)start_routine;
    register uint64 a3 asm("a3") = (uint64)arg;
    register uint64 a4 asm("a4") = (uint64)stackTop;

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
                 : "memory");

    // Ako kreiranje nije uspelo, vraćamo stek.
    if ((int)a0 < 0) {
        mem_free(stack);
    }

    return (int)a0;
}

int thread_exit() {
    register uint64 a0 asm("a0") = 0x12;

    asm volatile("ecall"
                 : "+r"(a0)
                 :
                 : "memory");

    return (int)a0;
}

void thread_dispatch() {
    register uint64 a0 asm("a0") = 0x13;
    //Napravi 64-bitnu promenljivu code, stavi je u registar a0 i upiši u nju vrednost 0x13
    asm volatile("ecall"
                 : "+r"(a0)
                 :
                 : "memory");
}
char getc()
{
    register uint64 code asm("a0") = 0x41;
    //code c++ promenljiva i a0 je isto

    // Tražimo jedan znak od jezgra
    __asm__ volatile(
        "ecall"
        : "+r"(code)
        :
        : "memory"
    );

    return (char)code;
}

void putc(char character)
{
    register uint64 code asm("a0") = 0x42;
    register uint64 argument asm("a1") =
        (uint64)(unsigned char)character;

    // Šaljemo znak jezgru
    __asm__ volatile(
        "ecall"
        : "+r"(code)
        : "r"(argument)
        : "memory"
    );
}

int sem_open(sem_t* handle, unsigned init)
{
    if (handle == nullptr) {
        return -1;
    }

    // a0 = kod poziva, a1 = adresa ručke, a2 = početna vrednost.
    register uint64 a0 asm("a0") = 0x21;
    register uint64 a1 asm("a1") = (uint64)handle;
    register uint64 a2 asm("a2") = (uint64)init;

    asm volatile(
        "ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2)
        : "memory"
    );

    return (int)a0;
}

int sem_close(sem_t handle)
{
    if (handle == nullptr) {
        return -1;
    }

    register uint64 a0 asm("a0") = 0x22;
    register uint64 a1 asm("a1") = (uint64)handle;

    asm volatile(
        "ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory"
    );

    return (int)a0;
}

int sem_wait(sem_t id)
{
    if (id == nullptr) {
        return -1;
    }

    register uint64 a0 asm("a0") = 0x23;
    register uint64 a1 asm("a1") = (uint64)id;

    asm volatile(
        "ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory"
    );

    return (int)a0;
}

int sem_signal(sem_t id)
{
    if (id == nullptr) {
        return -1;
    }

    register uint64 a0 asm("a0") = 0x24;
    register uint64 a1 asm("a1") = (uint64)id;

    asm volatile(
        "ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory"
    );

    return (int)a0;
}

