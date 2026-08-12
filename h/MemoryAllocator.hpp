//
// Created by jelena on 8/12/26.
//

#ifndef OS_PROJECT_MEMORYALLOCATOR_HPP
#define OS_PROJECT_MEMORYALLOCATOR_HPP

#include "../lib/hw.h"

class MemoryAllocator {
public:
    // Broj blokova memorije koji treba alocirati
    static void* mem_alloc(size_t blockCount);
    static int mem_free(void* ptr);
private:
    struct BlockHeader
    {
        // Ukupna veličina ovog segmenta memorije izražena u bajtovima
        size_t size;
        BlockHeader* next;
    };
    //freeHead - slobodan segment - slobodan segment - nullptr
    static BlockHeader* freeHead;
    static bool initialized;
};
#endif //OS_PROJECT_MEMORYALLOCATOR_HPP
