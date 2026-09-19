//
// Created by jelena on 9/5/26.
//


#ifndef OS_PROJECT_CONSOLE_BUFFER_HPP
#define OS_PROJECT_CONSOLE_BUFFER_HPP

class ConsoleBuffer {
public:
    // Poziva se prilikom inicijalizacije konzole,
    // pre pokretanja niti i obrade prekida.
    void initialize()
    {
        head = 0;
        tail = 0;
        count = 0;
    }

    // Dodaje znak. Ako nema mesta, vraća false.
    bool put(char character)
    {
        if (count == CAPACITY) {
            return false;
        }

        data[tail] = character;
        tail = (tail + 1) % CAPACITY;
        count++;

        return true;
    }

    // Uzima najstariji znak. Ako je prazno, vraća false.
    bool get(char& character)
    {
        if (count == 0) {
            return false;
        }

        character = data[head];
        head = (head + 1) % CAPACITY;
        count--;

        return true;
    }

    bool isEmpty() const
    {
        return count == 0;
    }

    bool isFull() const
    {
        return count == CAPACITY;
    }

    static unsigned capacity()
    {
        return CAPACITY;
    }

private:
    static const unsigned CAPACITY = 256;

    char data[CAPACITY];

    // Mesto sledećeg čitanja.
    unsigned head;

    // Mesto sledećeg upisa.
    unsigned tail;

    // Broj trenutno sačuvanih znakova.
    unsigned count;
};

#endif // OS_PROJECT_CONSOLE_BUFFER_HPP
