#include "../h/CharBuffer.hpp"

bool CharBuffer::put(char c)
{
    if (isFull()) { return false; }

    data[(first + count) % CAPACITY] = c;
    count++;
    return true;
}

bool CharBuffer::get(char& c)
{
    if (!peek(c)) { return false; }

    first = (first + 1) % CAPACITY;
    count--;
    return true;
}

bool CharBuffer::peek(char& c) const
{
    if (isEmpty()) { return false; }

    c = data[first];
    return true;
}
