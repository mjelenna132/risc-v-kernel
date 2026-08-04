# Jezgro operativnog sistema za RISC-V

Malo, ali funkcionalno jezgro operativnog sistema sa nitima i deljenjem vremena,
napisano od nule u C++ i asembleru za arhitekturu RISC-V (rv64ima). Izvršava se
bez ijednog operativnog sistema ispod sebe, na golom hardveru koji emulira QEMU.

Projekat je rađen za predmet Operativni sistemi 1 na Elektrotehničkom fakultetu
u Beogradu, školska 2025/2026. godina. Cela postavka zadatka nalazi se u
[docs/postavka.md](docs/postavka.md).

## Šta jezgro radi

- **Upravljanje memorijom.** Alokator sa slobodnom listom (first fit), blokovima
  poravnatim na `MEM_BLOCK_SIZE` i spajanjem susednih slobodnih blokova pri
  oslobađanju.
- **Niti.** Kreiranje, gašenje, ustupanje procesora i raspoređivanje u ciklusu
  (round robin). Telo korisničke niti izvršava se u neprivilegovanom (U) režimu
  rada procesora, pa greška u korisničkom programu ne može da obori jezgro.
- **Semafori.** Opšti brojački semafori sa FIFO redom čekanja, uključujući
  operacije nad više jedinica resursa odjednom (`sem_wait_n`, `sem_signal_n`).
- **Deljenje vremena.** Preotimanje na prekid od tajmera, sa vremenskim odsečkom
  po niti, dakle asinhrona promena konteksta.
- **Uspavljivanje niti.** `time_sleep` nad redom uspavanih niti uređenim po
  trenutku buđenja.
- **Konzola.** Baferisan ulaz i izlaz, tako da nijedan sistemski poziv ne
  zadržava procesor uposlenim čekanjem na hardver.
- **Tri sloja interfejsa.** Objektni C++ API, proceduralni C API i binarni ABI
  zasnovan na instrukciji `ecall`.

## Arhitektura

Jezgro je „bibliotečno“: jezgro i korisnička aplikacija dele isti adresni
prostor i statički se povezuju u jedinstven program koji je unapred učitan u
memoriju, kao kod ugrađenih sistema.

```
+---------------------------------------------+
|  korisnički program (test/)                  |
+---------------------------------------------+
|  C++ API   Thread, Semaphore, PeriodicThread |  korisnički (U) režim
+---------------------------------------------+
|  C API     thread_create, sem_wait, getc...  |
+---------------------------------------------+
|  ABI       ecall                             |
+=============================================+
|  jezgro    src/, h/                          |  sistemski (S) režim
+---------------------------------------------+
|  hw.lib    pristup hardveru                  |
+---------------------------------------------+
```

Ceo kôd jezgra izvršava se sa maskiranim prekidima. To se dobija besplatno, jer
procesor pri skoku u prekidnu rutinu sam upisuje `sstatus.SIE = 0`, pa nigde u
jezgru nije potrebno ručno maskiranje. Posledica te odluke je da jezgru pristupa
i glavna nit jezgra, kao i interne niti jezgra, isključivo sistemskim pozivom, a
ne direktnim pozivom funkcije: tako postoji tačno jedno mesto ulaska u jezgro.

### Sistemski pozivi

| Kôd | Poziv | Kôd | Poziv |
|---|---|---|---|
| `0x01` | `mem_alloc` | `0x24` | `sem_signal` |
| `0x02` | `mem_free` | `0x25` | `sem_wait_n` |
| `0x11` | `thread_create` | `0x26` | `sem_signal_n` |
| `0x12` | `thread_exit` | `0x31` | `time_sleep` |
| `0x13` | `thread_dispatch` | `0x41` | `getc` |
| `0x21` | `sem_open` | `0x42` | `putc` |
| `0x22` | `sem_close` | `0x81` | interni: sledeći znak za ispis |
| `0x23` | `sem_wait` | `0x82` | interni: potvrda ispisa |

Kôd poziva i argumenti prenose se registrima `a0` do `a4`, a povratna vrednost
stiže kroz `a0`. Pozivi od `0x80` naviše služe internoj niti jezgra i odbijaju
se ako ih pozove korisnička nit.

## Struktura projekta

```
h/            zaglavlja jezgra i interfejsnih slojeva
src/          implementacija jezgra
  contextSwitch.S   promena konteksta (ra, sp, s0-s11)
  trapEntry.S       ulazak u prekidnu rutinu i povratak iz nje
  TCB.cpp           kontrolni blok niti, raspoređivanje, uspavljivanje
  Console.cpp       baferisan ulaz i izlaz na konzolu
  MemoryAllocator.cpp
  Semaphore.cpp
  Riscv.cpp         rad sa CSR registrima, obrada trapova
test/         test primeri i korisnički program (userMain)
lib/          date biblioteke i zaglavlja (hw.lib)
docs/         postavka zadatka i razvojne beleške
kernel.ld     skript za povezivanje
```

Od datih biblioteka uvezuje se samo `hw.lib`. Biblioteke `mem.lib` (alokator) i
`console.lib` (funkcije `getc` i `putc`) nisu potrebne, jer su i alokator i rad
sa konzolom implementirani u okviru projekta.

## Prevođenje i pokretanje

Potreban je RISC-V unakrsni prevodilac i QEMU. Na sistemima zasnovanim na
Debian-u i Ubuntu-u:

```bash
sudo apt install gcc-riscv64-linux-gnu g++-riscv64-linux-gnu \
                 binutils-riscv64-linux-gnu qemu-system-misc gdb-multiarch
```

Zatim:

```bash
make        # pravi kernel i kernel.asm
make qemu   # pokreće jezgro u emulatoru
make clean  # briše sve što je nastalo prevođenjem
```

Iz QEMU-a se izlazi kombinacijom `Ctrl-A`, pa `X`.

## Testovi

Po pokretanju, korisnički program traži broj testa:

| Test | Šta proverava |
|---|---|
| 1 | niti i sinhrona promena konteksta, C API |
| 2 | niti i sinhrona promena konteksta, C++ API |
| 3 | proizvođač i potrošač sa semaforima, C API |
| 4 | proizvođač i potrošač sa semaforima, C++ API |
| 5 | `time_sleep`, C API |
| 6 | asinhrona promena konteksta i `PeriodicThread`, C++ API |
| 7 | da li se korisnički kôd zaista izvršava u korisničkom režimu |

Test 7 uspeva tako što **pukne**: korisnička nit pokuša da pročita privilegovan
CSR registar, jezgro prijavi nedozvoljenu instrukciju i zaustavi emulator. Ako
se ispiše poruka o uspešnom završetku, to znači da korisnički kôd radi u
sistemskom režimu, dakle da nešto nije u redu.

Testovi 3 i 4 očekuju unos sa tastature, a zaustavljaju se tasterom `ESC`.

## Debagovanje

```bash
make qemu-gdb      # pokreće emulator zaustavljen, i ispisuje port
gdb-multiarch kernel
(gdb) target remote localhost:PORT
```

Fajl `kernel.asm`, koji `make` usput generiše, sadrži disasembliran kôd
prepleten sa izvornim, pa se preko njega adrese iz poruka o greškama prevode u
konkretne linije.

Zamke na koje se naišlo tokom izrade, zajedno sa dijagnozama i rešenjima,
zapisane su u [docs/razvojne-beleske.md](docs/razvojne-beleske.md).

## Neke projektne odluke

- **Redovi niti su intruzivni.** Pokazivač na sledbenika stoji u samom
  kontrolnom bloku niti, pa ulazak u red i izlazak iz njega ne traže alokaciju
  memorije. To je moguće zato što se nit u svakom trenutku nalazi u najviše
  jednom redu.
- **Red uspavanih niti čuva relativna vremena.** Uz svaku nit stoji vreme koje
  protekne od buđenja niti ispred nje, pa se na svaku periodu tajmera ažurira
  samo prva nit u redu, bez obzira na to koliko niti spava.
- **Kontekst niti čuva `ra`, `sp` i `s0-s11`.** Registre koje čuva pozivalac nije
  potrebno čuvati, jer se promena konteksta uvek dešava unutar poziva
  potprograma; registre tela niti čuva prekidna rutina, jer se preotimanje
  dešava isključivo u njoj.
- **Ugašena nit se ne oslobađa odmah.** Jezgro se u tom trenutku još izvršava na
  njenom steku, pa nit ide u red ugašenih, a memorija se oslobađa tek posle
  promene konteksta.
- **Izlazni bafer konzole prazni interna nit jezgra.** Znak se iz bafera uklanja
  tek pošto je stvarno prenet na kontroler, pa prazan bafer pouzdano znači da je
  ispisano sve što je traženo.

## Poreklo koda i licenca

Okruženje za prevođenje (`Makefile`, `kernel.ld`, `.gdbinit.tmpl-riscv`) i
biblioteke u `lib/` deo su materijala predmeta i izvedeni su iz edukativnog
operativnog sistema [xv6](https://github.com/mit-pdos/xv6-riscv), koji je pod
MIT licencom (videti [LICENSE](LICENSE)). Test primeri u `test/` dati su uz
postavku zadatka. Sav kôd u `h/` i `src/`, dakle celo jezgro i sva tri
interfejsna sloja, napisan je u okviru ovog projekta.
