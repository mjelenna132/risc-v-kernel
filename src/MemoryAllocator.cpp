#include "../h/MemoryAllocator.hpp"

MemoryAllocator::FreeBlock* MemoryAllocator::head = nullptr;
uint64 MemoryAllocator::heapStart = 0;
uint64 MemoryAllocator::heapEnd = 0;

void MemoryAllocator::init()
{
    heapStart = ((uint64) HEAP_START_ADDR + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE * MEM_BLOCK_SIZE;
    heapEnd   =  (uint64) HEAP_END_ADDR / MEM_BLOCK_SIZE * MEM_BLOCK_SIZE;

    if (heapEnd <= heapStart) { head = nullptr; return; }

    head = (FreeBlock*) heapStart;
    head->size = heapEnd - heapStart;
    head->next = nullptr;
    head->magic = 0;
}

void* MemoryAllocator::mem_alloc(size_t size)
{
    if (size == 0 || !head) { return nullptr; }

    size_t need = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE * MEM_BLOCK_SIZE;
    need += MEM_BLOCK_SIZE;

    FreeBlock* prev = nullptr;
    FreeBlock* curr = head;

    while (curr) {
        if (curr->size >= need) {
            if (curr->size - need >= 2 * MEM_BLOCK_SIZE) {
                FreeBlock* leftover = (FreeBlock*) ((uint64) curr + need);
                leftover->size = curr->size - need;
                leftover->next = curr->next;
                leftover->magic = 0;
                curr->size = need;
                if (prev) { prev->next = leftover; }
                else      { head = leftover; }
            } else {
                if (prev) { prev->next = curr->next; }
                else      { head = curr->next; }
            }

            curr->magic = ALLOCATED_MAGIC;
            return (void*) ((uint64) curr + MEM_BLOCK_SIZE);
        }
        prev = curr;
        curr = curr->next;
    }
    return nullptr;
}

// Povratne vrednosti: 0 uspeh, -1 nema pokazivaca, -2 adresa ne moze da bude
// pocetak bloka, -3 blok nije zauzet. Sve provere se rade pre bilo kakve izmene,
// da nevalidan poziv ne ostavi slobodnu listu u polovicnom stanju.
int MemoryAllocator::mem_free(void* addr)
{
    if (!addr) { return -1; }

    // Blokovi pocinju na visekratniku MEM_BLOCK_SIZE i zaglavlje je iste
    // velicine, pa je i vracena adresa uvek tako poravnata. Neporavnat
    // pokazivac zato sigurno pokazuje u sredinu nekog bloka.
    if ((uint64) addr % MEM_BLOCK_SIZE != 0) { return -2; }

    FreeBlock* blk = (FreeBlock*) ((uint64) addr - MEM_BLOCK_SIZE);

    if ((uint64) blk < heapStart || (uint64) blk >= heapEnd) { return -2; }

    // Poravnat pokazivac jos uvek moze da pokazuje u sredinu zauzetog bloka ili
    // na vec oslobodjen blok; oba slucaja hvata trag u zaglavlju.
    if (blk->magic != ALLOCATED_MAGIC) { return -3; }

    if (blk->size == 0 || (uint64) blk + blk->size > heapEnd) { return -2; }

    FreeBlock* prev = nullptr;
    FreeBlock* curr = head;
    while (curr != nullptr && curr < blk) {
        prev = curr;
        curr = curr->next;
    }

    // Zastita za slucaj da je trag u zaglavlju slucajno pogodjen zatecenim
    // podacima: blok koji upada u sredinu slobodnog regiona je vec oslobodjen.
    if (prev != nullptr && (uint64) prev + prev->size > (uint64) blk) { return -3; }

    blk->magic = 0;
    blk->next = curr;
    if (prev) { prev->next = blk; }
    else      { head = blk; }

    if (curr) { tryToJoin(blk, curr); }
    if (prev) { tryToJoin(prev, blk); }

    return 0;
}

size_t MemoryAllocator::getFreeSpace()
{
    size_t total = 0;
    for (FreeBlock* curr = head; curr != nullptr; curr = curr->next) {
        total += curr->size;
    }
    return total;
}

size_t MemoryAllocator::getLargestFreeBlock()
{
    size_t largest = 0;
    for (FreeBlock* curr = head; curr != nullptr; curr = curr->next) {
        if (curr->size > largest) { largest = curr->size; }
    }
    return largest;
}

void MemoryAllocator::tryToJoin(FreeBlock* prev, FreeBlock* curr)
{
    if ((uint64) prev + prev->size == (uint64) curr) {
        prev->size += curr->size;
        prev->next = curr->next;
    }
}
