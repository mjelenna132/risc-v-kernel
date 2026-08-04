#ifndef _syscall_dispatch
#define _syscall_dispatch

#include "syscall_abi.hpp"

// Razgranati skok na obradu pojedinacnog sistemskog poziva. Poziva se iz
// prekidne rutine, sa argumentima koje je pozivalac ostavio u registrima
// a0-a4; vraca vrednost koju sistemski poziv vraca kroz registar a0.
int64 dispatchSyscall(uint64 code, uint64 a1, uint64 a2, uint64 a3, uint64 a4);

#endif
