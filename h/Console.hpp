#ifndef _console_hpp
#define _console_hpp

#include "../lib/hw.h"

// Sprega jezgra ka kontroleru konzole. Oba smera su baferisana, da nijedan
// sistemski poziv ne bi uposleno cekao na hardver:
//   izlaz: putc ostavi znak u bafer, interna nit jezgra ga prenese na kontroler
//   ulaz:  prekidna rutina napuni bafer, getc uzme iz njega
class _Console
{
public:
    // Pravi semafore i pokrece internu nit jezgra za ispis.
    static void init();

    // Obrada poziva 0x41 i 0x42. Blokiraju nit ako nema znaka odnosno mesta.
    static char getc();
    static void putc(char c);

    // Obrada internih poziva 0x81 i 0x82 interne niti jezgra. Znak se iz bafera
    // uklanja tek potvrdom, pa prazan bafer znaci da je stvarno sve ispisano.
    static char nextOutputChar();
    static void outputCharSent();

    static void handleInterrupt();

    // Ceka da se izlazni bafer isprazni, pre gasenja jezgra.
    static void flush();

    // Ispis prozivanjem, bez bafera i bez niti. Samo za KERNEL PANIC.
    static void putcDirect(char c);
    static void flushDirect();

private:
    // Ogranicenje da obrada prekida ne traje predugo u naletu znakova.
    static const size_t MAX_CHARS_PER_INTERRUPT = 64;

    static void outputThreadBody(void* arg);
};

#endif
