//
// Created by jelena on 8/12/26.
//

#include "../h/MemoryAllocator.hpp"
MemoryAllocator::BlockHeader* MemoryAllocator::freeHead = nullptr;
bool MemoryAllocator::initialized = false;
void* MemoryAllocator::mem_alloc(size_t blockCount) {
    // Nije moguće alocirati nula blokova
    if (blockCount == 0) {
        return nullptr;
    }

    // Heap se priprema samo prilikom prve alokacije
    if (!initialized) {
        // Na početku je ceo heap jedan veliki slobodan segment
        freeHead = (BlockHeader*) HEAP_START_ADDR;

        // Veličina celog heap-a u bajtovima
        freeHead->size =
            (const char*) HEAP_END_ADDR -
            (const char*) HEAP_START_ADDR;

        // Na početku ne postoji drugi slobodan segment
        freeHead->next = nullptr;

        initialized = true;
    }
    // Ukupan broj blokova u heap-u.
    size_t heapBlockCount =
        ((const char*)HEAP_END_ADDR -
         (const char*)HEAP_START_ADDR) / MEM_BLOCK_SIZE;

    // Mora ostati i jedan blok za zaglavlje.
    if (blockCount >= heapBlockCount) {
        return nullptr;
    }

    // Potreban prostor za korisnika i jedan blok zaglavlje
    size_t requiredSize = (blockCount + 1) * MEM_BLOCK_SIZE;

    BlockHeader* current = freeHead;
    BlockHeader* previous = nullptr;

    // Tražimo prvi slobodan segment koji je dovoljno veliki
    while (current != nullptr && current->size < requiredSize) {
        previous = current;
        current = current->next;
    }

    // Nijedan slobodan segment nije dovoljno veliki
    if (current == nullptr) {
        return nullptr;
    }
    // Koliko bajtova ostaje posle alokacije.
    size_t remainingSize = current->size - requiredSize;

    if (remainingSize >= MEM_BLOCK_SIZE) {
        // Početak preostalog slobodnog dela.
        BlockHeader* remaining =
            (BlockHeader*)((char*)current + requiredSize);

        remaining->size = remainingSize;
        remaining->next = current->next;

        // Ubacujemo preostali deo u slobodnu listu.
        if (previous == nullptr) {
            freeHead = remaining;
        } else {
            previous->next = remaining;
        }

        current->size = requiredSize;
    } else {
        // Uzimamo ceo segment i izbacujemo ga iz slobodne liste.
        if (previous == nullptr) {
            freeHead = current->next;
        } else {
            previous->next = current->next;
        }
    }

    // Segment više nije slobodan.
    current->next = nullptr;

    // Preskačemo blok sa zaglavljem i vraćamo korisnički prostor.
    return (void*)((char*)current + MEM_BLOCK_SIZE);
}

int MemoryAllocator::mem_free(void* ptr) {
    if (ptr == nullptr) {
        return -1;
    }

    // Zaglavlje se nalazi jedan blok pre korisničkog prostora.
    BlockHeader* block =
        (BlockHeader*)((char*)ptr - MEM_BLOCK_SIZE);

    BlockHeader* current = freeHead;
    BlockHeader* previous = nullptr;

    // Tražimo mesto po adresi na koje vraćamo segment.
    while (current != nullptr &&
           (uint64)current < (uint64)block) {
        previous = current;
        current = current->next;
           }

    // Segment je već slobodan.
    if (current == block) {
        return -2;
    }

    // Vraćamo segment u slobodnu listu.
    block->next = current;

    if (previous == nullptr) {
        freeHead = block;
    } else {
        previous->next = block;
    }

    // Spajamo segment sa sledećim ako su susedni.
    if (current != nullptr &&
        (char*)block + block->size == (char*)current) {
        block->size += current->size;
        block->next = current->next;
        }

    // Spajamo segment sa prethodnim ako su susedni.
    if (previous != nullptr &&
        (char*)previous + previous->size == (char*)block) {
        previous->size += block->size;
        previous->next = block->next;
        }

    return 0;
}