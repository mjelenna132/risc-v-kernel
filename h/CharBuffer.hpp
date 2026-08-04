#ifndef _charBuffer_hpp
#define _charBuffer_hpp

#include "../lib/hw.h"

// Kruzni bafer znakova, bez ikakve sinhronizacije. Cekanje na znak odnosno na
// slobodno mesto resava korisnik bafera.
class CharBuffer
{
public:
    static const size_t CAPACITY = 256;

    constexpr CharBuffer() : first(0), count(0), data{} {}

    CharBuffer(const CharBuffer&) = delete;

    CharBuffer& operator=(const CharBuffer&) = delete;

    // Sve tri vracaju netacno ako je bafer pun odnosno prazan.
    bool put(char c);
    bool get(char& c);
    bool peek(char& c) const;   // cita, ali ne uklanja

    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count == CAPACITY; }

private:
    size_t first;
    size_t count;
    char data[CAPACITY];
};

#endif
