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
#endif //OS_PROJECT_SYSCALL_C_HPP
