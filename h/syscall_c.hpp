//
// Created by jelena on 8/12/26.
//

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
#endif //OS_PROJECT_SYSCALL_C_HPP
