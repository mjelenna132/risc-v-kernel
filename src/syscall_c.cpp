#include "../h/syscall_c.hpp"
#include "../h/syscall_abi.hpp"

void* mem_alloc(size_t size)
{
    // Sistemski poziv ocekuje velicinu izrazenu u blokovima, zaokruzenu navise.
    size_t blocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    return (void*) abiSyscall(SYS_MEM_ALLOC, blocks);
}

int mem_free(void* p)
{
    return (int) abiSyscall(SYS_MEM_FREE, (uint64) p);
}

size_t mem_get_free_space()
{
    return (size_t) abiSyscall(SYS_MEM_GET_FREE_SPACE);
}

size_t mem_get_largest_free_block()
{
    return (size_t) abiSyscall(SYS_MEM_GET_LARGEST_FREE_BLOCK);
}

int thread_create(thread_t* handle, void (*start_routine)(void*), void* arg)
{
    // Sistemski poziv ocekuje vec odvojen prostor za stek niti.
    void* stackBase = mem_alloc(DEFAULT_STACK_SIZE);
    if (stackBase == nullptr) { return -1; }
    void* stackSpace = (void*) ((uint64) stackBase + DEFAULT_STACK_SIZE);

    int status = (int) abiSyscall(SYS_THREAD_CREATE, (uint64) handle,
                                  (uint64) start_routine, (uint64) arg,
                                  (uint64) stackSpace);
    if (status < 0) { mem_free(stackBase); }

    return status;
}

int thread_exit()
{
    return (int) abiSyscall(SYS_THREAD_EXIT);
}

void thread_dispatch()
{
    abiSyscall(SYS_THREAD_DISPATCH);
}

int sem_open(sem_t* handle, unsigned init)
{
    return (int) abiSyscall(SYS_SEM_OPEN, (uint64) handle, init);
}

int sem_close(sem_t handle)
{
    return (int) abiSyscall(SYS_SEM_CLOSE, (uint64) handle);
}

int sem_wait(sem_t id)
{
    return (int) abiSyscall(SYS_SEM_WAIT, (uint64) id);
}

int sem_signal(sem_t id)
{
    return (int) abiSyscall(SYS_SEM_SIGNAL, (uint64) id);
}

int sem_wait_n(sem_t id, unsigned n)
{
    return (int) abiSyscall(SYS_SEM_WAIT_N, (uint64) id, n);
}

int sem_signal_n(sem_t id, unsigned n)
{
    return (int) abiSyscall(SYS_SEM_SIGNAL_N, (uint64) id, n);
}

int sem_trywait(sem_t id)
{
    return sem_trywait_n(id, 1);
}

int sem_trywait_n(sem_t id, unsigned n)
{
    return (int) abiSyscall(SYS_SEM_TRYWAIT, (uint64) id, n);
}

int sem_timedwait(sem_t id, time_t periods)
{
    return sem_timedwait_n(id, 1, periods);
}

int sem_timedwait_n(sem_t id, unsigned n, time_t periods)
{
    return (int) abiSyscall(SYS_SEM_TIMEDWAIT, (uint64) id, n, periods);
}

int time_sleep(time_t period)
{
    return (int) abiSyscall(SYS_TIME_SLEEP, period);
}

char getc()
{
    return (char) abiSyscall(SYS_GETC);
}

void putc(char c)
{
    abiSyscall(SYS_PUTC, (uint64) c);
}
