#ifndef _syscall_abi
#define _syscall_abi

#include "../lib/hw.h"

typedef long int64;

// Kodovi sistemskih poziva. Uz one iz postavke stoje i 0x03, 0x04, 0x27 i 0x28,
// koje postavka ne trazi. Kodovi od 0x80 navise koriste samo interne niti
// jezgra, da bi i one u jezgro ulazile kroz prekidnu rutinu.
enum SyscallCode
{
    SYS_MEM_ALLOC = 0x01,
    SYS_MEM_FREE = 0x02,
    SYS_MEM_GET_FREE_SPACE = 0x03,
    SYS_MEM_GET_LARGEST_FREE_BLOCK = 0x04,

    SYS_THREAD_CREATE = 0x11,
    SYS_THREAD_EXIT = 0x12,
    SYS_THREAD_DISPATCH = 0x13,

    SYS_SEM_OPEN = 0x21,
    SYS_SEM_CLOSE = 0x22,
    SYS_SEM_WAIT = 0x23,
    SYS_SEM_SIGNAL = 0x24,
    SYS_SEM_WAIT_N = 0x25,
    SYS_SEM_SIGNAL_N = 0x26,
    SYS_SEM_TRYWAIT = 0x27,
    SYS_SEM_TIMEDWAIT = 0x28,

    SYS_TIME_SLEEP = 0x31,

    SYS_GETC = 0x41,
    SYS_PUTC = 0x42,

    SYS_CONSOLE_NEXT_OUTPUT = 0x81,
    SYS_CONSOLE_OUTPUT_SENT = 0x82,
};

// Jedini ulaz u jezgro. Kod i argumenti idu kroz a0-a4, rezultat kroz a0.
inline int64 abiSyscall(uint64 code, uint64 a1 = 0, uint64 a2 = 0,
                        uint64 a3 = 0, uint64 a4 = 0)
{
    register uint64 r_a0 __asm__("a0") = code;
    register uint64 r_a1 __asm__("a1") = a1;
    register uint64 r_a2 __asm__("a2") = a2;
    register uint64 r_a3 __asm__("a3") = a3;
    register uint64 r_a4 __asm__("a4") = a4;

    __asm__ volatile ("ecall"
        : "+r"(r_a0)
        : "r"(r_a1), "r"(r_a2), "r"(r_a3), "r"(r_a4)
        : "memory");

    return (int64) r_a0;
}

#endif
