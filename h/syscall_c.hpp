#ifndef _syscall_c
#define _syscall_c

#include "../lib/hw.h"

void* mem_alloc(size_t size);
int mem_free(void*);

// Sirove velicine iz slobodne liste. Uz svaki zauzet blok ide zaglavlje od
// MEM_BLOCK_SIZE, pa je najveci moguci zahtev za toliko manji.
size_t mem_get_free_space();
size_t mem_get_largest_free_block();

class _thread;
typedef _thread* thread_t;

int thread_create(thread_t* handle, void (*start_routine)(void*), void* arg);
int thread_exit();
void thread_dispatch();

class _sem;
typedef _sem* sem_t;

int sem_open(sem_t* handle, unsigned init);
int sem_close(sem_t handle);
int sem_wait(sem_t id);
int sem_signal(sem_t id);
int sem_wait_n(sem_t id, unsigned n);
int sem_signal_n(sem_t id, unsigned n);

// Ne blokira se: 0 uspeh, SEM_WOULD_BLOCK ako bi trebalo cekati, < 0 greska.
const int SEM_WOULD_BLOCK = 1;
int sem_trywait(sem_t id);
int sem_trywait_n(sem_t id, unsigned n);

// Ceka najvise zadato vreme: 0 uspeh, SEM_TIMEOUT istek, < 0 druga greska.
const int SEM_TIMEOUT = -4;
int sem_timedwait(sem_t id, time_t periods);
int sem_timedwait_n(sem_t id, unsigned n, time_t periods);

int time_sleep(time_t);

const int EOF = -1;
char getc();
void putc(char);

#endif
