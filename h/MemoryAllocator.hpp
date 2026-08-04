#ifndef _memoryAllocator_hpp
#define _memoryAllocator_hpp

#include "../lib/hw.h"

class MemoryAllocator
{
public:
    static void init();
    static void* mem_alloc(size_t size);
    static int mem_free(void* addr);

    // Sirove velicine iz slobodne liste, sa zaglavljima.
    static size_t getFreeSpace();
    static size_t getLargestFreeBlock();

private:
    // Zaglavlje bloka, veliko MEM_BLOCK_SIZE. Kod slobodnog bloka su u upotrebi
    // sva tri polja, kod zauzetog samo size i magic.
    struct FreeBlock
    {
        size_t size;
        FreeBlock* next;
        uint64 magic;
    };

    // Upisuje se u zaglavlje pri zauzimanju, brise pri oslobadjanju. Sluzi da
    // mem_free prepozna pokazivac koji ne pokazuje na pocetak zauzetog bloka.
    // Vrednost je ASCII zapis reci MEMBLOCK.
    static const uint64 ALLOCATED_MAGIC = 0x4D454D424C4F434BUL;

    static FreeBlock* head;
    static uint64 heapStart;
    static uint64 heapEnd;

    static void tryToJoin(FreeBlock* prev, FreeBlock* curr);
};

#endif
