//
// Created by jelena on 8/12/26.
//
// SPISAK FUNKCIJA KOJE KORISNIK MOZE DA POZOVE
#ifndef OS_PROJECT_SYSCALL_C_HPP
#define OS_PROJECT_SYSCALL_C_HPP
#include "../lib/hw.h"

// Alocira najmanje size bajtova.
void* mem_alloc(size_t size);

// Oslobađa prethodno alociranu memoriju.
int mem_free(void* ptr);

class _thread;
typedef _thread* thread_t;


// Pravi novu nit.
int thread_create(
    thread_t* handle,
    void (*start_routine)(void*),
    void* arg
);

// Završava tekuću nit.
int thread_exit();

// Predaje procesor drugoj niti.
void thread_dispatch();
const int EOF = -1;

char getc();

void putc(char character);

class _sem;
using sem_t = _sem*;

int sem_open(sem_t* handle, unsigned init);
int sem_close(sem_t handle);
int sem_wait(sem_t id);
int sem_signal(sem_t id);

// Zauzima n jedinica semafora.
int sem_wait_n(sem_t id, unsigned n);

// Oslobađa n jedinica semafora.
int sem_signal_n(sem_t id, unsigned n);



// Uspavljuje tekuću nit na zadati broj perioda tajmera.
int time_sleep(time_t time);
// Jedinica vremena predstavlja jednu periodu tajmera.
using time_t = unsigned long;
#endif //OS_PROJECT_SYSCALL_C_HPP
