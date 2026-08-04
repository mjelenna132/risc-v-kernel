# Razvojne beleške

Hronološki zapis problema na koje se naišlo tokom izrade, kako su
dijagnostikovani i čime su rešeni. Ovo nije opis jezgra (za to je README),
nego podsetnik na zamke specifične za RISC-V i za projektne odluke ovog
rešenja.

---

# Niti u korisničkom (U) režimu: testovi 1, 2 i 7

Kratka referenca za sledeći put kad niti zakucaju čim pređu u U-mod.

---

## Simptom

- Testovi ispišu `ThreadA..D created`, pa **ništa** - nema `A: i=0`.
- Nema ni `KERNEL PANIC` - deluje kao tih zastoj (hang).
- U stvari: beskonačna bujica trapova; telo niti se **nikad ne izvrši**.

## Kako sam dijagnostikovao (alati)

```bash
# 1) Log izuzetaka/prekida iz QEMU-a
(sleep 3; printf '1\n'; sleep 4) | timeout 12 qemu-system-riscv64 \
  -machine virt -bios none -kernel kernel -m 128M -smp 1 -nographic \
  -d int -D /tmp/qemu_int.log
sort /tmp/qemu_int.log | uniq -c        # koji scause se ponavlja

# 2) GDB (paket: gdb-multiarch) - break na handleSupervisorTrap, čitaj CSR-ove
#    pokreni qemu sa:  -gdb tcp::26057 -S   (pauzirano), pa u gdb-u:
#      target remote localhost:26057
#      break handleSupervisorTrap
#      commands
#      silent
#      printf "scause=%ld sepc=0x%lx\n", $scause, $sepc
#      continue
#      end
#      continue
```

Mapiranje `scause` (MSB=bit63 postavljen => PREKID, inače izuzetak):
- `0x8000000000000001` = supervisor **software** interrupt (kod nas: prosleđeni M-tajmer preko `sip.SSIP`)
- `0x8000000000000009` = supervisor **external** interrupt (PLIC / konzola UART)
- `2` = illegal instruction, `8` = ecall iz U-moda, `9` = ecall iz S-moda
- `sepc` pokazuje gde je trap uhvaćen (npr. na 1. instrukciji tela niti => prekid odmah okida).

Adresu simbola nađi u `kernel.asm` (npr. `grep threadWrapper kernel.asm`).

---

## GLAVNA ZAMKA (uzrok #1)

> **U U-modu se `sstatus.SIE` IGNORIŠE.** S-prekidi su UVEK omogućeni za niži
> režim. Jedina preostala kapija je registar **`sie[i]`** (per-prekid enable),
> koji važi u SVIM režimima.

`hw.lib` boot ostavi upaljene `sie.SSIE` (tajmer) i `sie.SEIE` (konzola).
Dok sve radi u S-modu sa `sstatus.SIE=0` → bezopasno (maskirano).
Čim nit uđe u U-mod → prekid okine na 1. instrukciji tela, a handler ga
ignoriše bez potvrde → **beskonačna petlja**, telo se ne izvrši.

**FIX:** ugasi ceo `sie` na startu (sinhroni dizajn, konzola ide prozivanjem):

```cpp
// h/Riscv.hpp
static void w_sie(uint64 sie);
inline void Riscv::w_sie(uint64 sie) {
    __asm__ volatile ("csrw sie, %[sie]" : : [sie] "r"(sie));
}

// src/Riscv.cpp - Riscv::initTrap()
w_stvec((uint64) trapEntry);
w_sie(0);      // <-- gasi SVE S-prekide u svim režimima
```

> Napomena: `mc_sip(SIP_SSIE)` (potvrda pending bita) u interrupt-grani handlera
> pomaže SAMO za software prekid i SAMO ako se grana izvrši; sa `sie=0` grana je
> mrtav kod. `w_sie(0)` rešava i software i external prekid odjednom.

---

## Zamka #2 - izlazak niti u U-modu

`threadWrapper` je zvao `TCB::exit()` **direktno**. Pošto wrapper radi u U-modu,
`dispatch()`/`Scheduler`/`threadContextSwitch` bi radili u U-modu; povratak u nit
suspendovanu u S-modu (npr. `main`) ušao bi **pogrešno u U-mod**, pa bi njen
`sret` bio ilegalan (`scause=2`).

**FIX:** izlazi preko sistemskog poziva, da jezgro odradi izlazak u S-modu:

```cpp
// src/TCB.cpp - threadWrapper()  (dodaj #include "../h/syscall_c.hpp")
running->body(running->arg);
thread_exit();     // ecall 0x12  - NE TCB::exit() direktno
for (;;) {}        // safety net
```

---

## Prelazak u U-mod (kontekst, da se ne zaboravi)

- `context.ra` nove niti = `startUserThread` (trapEntry.S), NE `threadWrapper`.
- `startUserThread` fabrikuje lažni trap-okvir (272B) na steku niti,
  **EKSPLICITNO čisti `sstatus.SPP`** i „vraća se“ kroz `trapReturn` (`sret`) →
  telo radi u U-modu.
- Referentni `popSppSpie` (`csrw sepc,ra; sret`) se oslanja na *ambijentalni*
  `SPP=0` - to NE važi ovde: prvi `dispatch` dolazi iz funkcije `main`, koja radi
  u S-modu i uopšte ne prolazi kroz trap, pa `SPP` može biti bilo šta. Zato je
  eksplicitno čišćenje `SPP`-a ispravan pristup u ovom SINHRONOM dizajnu.

---

## Očekivani ispis (posle popravki)

- **TEST 1/2:** preplitanje `A:0, B:0, A:1, B:1…`; `C: t1=7` (registar očuvan kroz
  context switch); `C: fibonaci=144`, `D: fibonaci=987`; `A/D finished!`.
- **TEST 7 (dokaz U-moda):** posle `B: i=10` → `csrr t6, sepc` (privilegovan CSR u
  U-modu) → `scause=2 sepc=0x…` pa `KERNEL PANIC: neocekivan izuzetak u
  korisnickom programu`, nakon čega jezgro zaustavlja emulator. `"Test se nije
  uspešno završio"` se **nikad** ne ispiše (nema regularnog završetka) = ispravno.

## Zamke pri pokretanju u QEMU

- Prosledi broj testa sa zakašnjenjem, inače se pojede pre prompta:
  `{ sleep 3; printf '7\n'; sleep 26; } | timeout 31 qemu-system-riscv64 ... `
- Zvanični testovi imaju OGROMNE busy-loop-ove (`10000*30000`) → vrlo spori pod
  emulacijom (A/B stignu ~i=9-13 za ~10s). Nije bug. Za TEST 7 daj ~25-30s da B
  stigne do `i=10`.
- Čišćenje zaostalih procesa: `pkill -9 -f qemu-system-riscv64`.

## Fajlovi koje sam menjao

- `h/Riscv.hpp` - dodat `w_sie` helper.
- `src/Riscv.cpp` - `w_sie(0)` u `initTrap()`; `mc_sip(SIP_SSIE)` u interrupt-grani.
- `src/TCB.cpp` - `threadWrapper` izlazi preko `thread_exit()` + `#include syscall_c.hpp`.

---

# Naknadne izmene: zadatak 3 i životni ciklus niti

## `userMain` je sada nit, a ne običan poziv

`main` više ne zove `userMain()` direktno (to bi značilo da ceo korisnički
program radi u S-modu). Umesto toga pravi nit nad `userMainBody` i zatim se
vrti u petlji `while (TCB::getActiveThreads() > 0) TCB::dispatch();`.

Time se rešavaju tri stvari odjednom:
- ceo softver iznad jezgra radi u **U-modu** (zahtev iz postavke, TEST 7);
- glavna nit jezgra je ujedno i **idle** nit, pa red spremnih niti nikad nije
  prazan → `Scheduler::get()` ne može da vrati `nullptr`;
- program se završava tek kad se **sve** niti aplikacije ugase (brojač
  `TCB::activeThreads`), a ne čim `userMain` vrati kontrolu.

`main` poziva `TCB::dispatch()` direktno (bez `ecall`), jer je i sam kod jezgra
u S-modu - nema potrebe za trapom, niti se ikad vraća kroz `sret`.

## Oslobađanje ugašene niti

Stek i TCB ugašene niti **ne smeju** da se oslobode u trenutku gašenja, jer se
jezgro tada još izvršava upravo na tom steku. Ugašena nit ide u red
`TCB::finishedThreads`, a `reapFinished()` se poziva tek **posle** promene
konteksta, dakle iz konteksta niti koja je preuzela procesor.

## `getc` mora da ustupi procesor

`_Console::getc()` u petlji čekanja zove `TCB::dispatch()`. Bez toga bi
sinhrono jezgro sa maskiranim prekidima stajalo celo dok se ne pritisne taster,
pa TEST 3/4 (proizvođač na tastaturi) ne bi mogao da radi.

---

# TEST 3 / 4: `scause=5` u `_Console::getc`

## Simptom

TEST 3 i 4 rade neko vreme (bujica znakova proizvođača), pa puknu sa:

```
scause=5 sepc=0x80001934
KERNEL PANIC: neocekivan izuzetak u korisnickom programu
```

`sepc` pokazuje na `lbu a0, 0(s2)` - poslednju instrukciju `_Console::getc()`,
onu koja čita `CONSOLE_RX_DATA`.

## Dijagnoza

```
scause=5 (load access fault) stval=0x8
sacuvano s2 = 0x8       a CONSOLE_RX_DATA = 0x10000000
```

`0x8` je vrednost koju `handleSupervisorTrap` drži u `s2` (`cause` za `ecall`).
Dakle: `s2` jedne niti procurio je u drugu nit.

## Uzrok

`threadContextSwitch` je čuvao **samo `ra` i `sp`**. Obrazloženje „ostale
registre čuva pozvani potprogram po konvenciji prevodioca“ ne važi za promenu
konteksta: prevodilac sme da drži živu vrednost u registru `s*` i **preko**
poziva `threadContextSwitch`, a taj poziv se vraća u *drugu* nit. Nova nit tako
nastavlja sa `s*` registrima stare niti.

Zašto puca baš `getc`, a ne TEST 1/2:
- telo niti radi u U-modu, njegovi `s*` registri idu u trap-okvir (272 B) i
  vraćaju se kroz `trapReturn` - ta strana je bila ispravna;
- ali `_Console::getc()` je **kod jezgra** koji drži `rxData` u `s2` i koristi ga
  *posle* `TCB::dispatch()` u petlji čekanja. U TEST-u 1/2 nijedna funkcija
  jezgra ne koristi `s*` posle promene konteksta, pa se šteta ne vidi.

**FIX** (`src/contextSwitch.S`, `h/TCB.hpp`): kontekst je proširen na `ra`, `sp`
i `s0-s11` (112 B), i `threadContextSwitch` ih sve čuva/obnavlja.

## Zamka pri prevođenju: `.d` datoteke se nisu uključivale

Posle izmene `h/TCB.hpp` `make` **nije** ponovo preveo nijednu `.cpp` datoteku,
pa je nova (112 B) verzija `threadContextSwitch` pisala preko starog (16 B)
`Context`-a i gazila polja `next`, `finished`… u TCB-u. Posledica: `TCB::running`
i red spremnih niti puni smeća, pa skok na adresu 0 (`sepc=0x0`).

Krivac je bio poslednji red Makefile-a:

```make
-include $(wildcard ${DIR_BUILD}/*.d)     # NE hvata build/src/*.d ni build/test/*.d
```

Zamenjeno rekurzivnim traženjem:

```make
-include $(shell find ${DIR_BUILD} -name "*.d" 2>/dev/null)
```

> Do te popravke, posle svake izmene zaglavlja obavezan je `make clean`.

## Očekivani ispis (TEST 3 i 4)

Uz npr. 2 proizvođača i bafer veličine 5: bujica cifara `1` (i `2`… za više
proizvođača), znakovi otkucani na tastaturi pojave se među njima, `ESC` (0x1b)
zaustavlja test, pa sledi:

```
!
Buffer deleted!
!
TEST 3 (zadatak 3., kompletan C API sa semaforima, sinhrona promena konteksta)
Kernel finished
```

---

# Zadatak 4: asinhrona promena konteksta, `time_sleep`, konzola

## Glavna projektna odluka: gde se garantuje međusobno isključenje

Ceo kôd jezgra izvršava se sa **maskiranim prekidima**. To se dobija besplatno
za sistemske pozive, izuzetke i prekide, jer procesor pri skoku u prekidnu
rutinu sam upisuje `sstatus.SIE = 0`.

Posledica: **niko ne sme da dira strukture jezgra van prekidne rutine.** Zato:

- `main` (glavna/idle nit) ustupa procesor sa `abiSyscall(SYS_THREAD_DISPATCH)`,
  a ne pozivom `TCB::dispatch()`, iako i sam radi u S-modu;
- interna nit jezgra za ispis pristupa jezgru **isključivo preko `ecall`-a**
  (`scause = 9`), a u svom telu radi samo ono što se ne tiče struktura jezgra -
  prozivanje bita spremnosti i upis u registar kontrolera.

Time postoji tačno jedno mesto ulaska u jezgro i za korisničke i za interne
niti, pa nigde nije potrebno ručno maskiranje prekida.

## ZAMKA: smeće u registru `tp` (`plic_claim` puca sa `scause=5`)

`startUserThread` je fabrikovao okvir prekidne rutine, ali je popunjavao **samo**
`sp`. Ostalih 31 slot ostajao je sa onim što se zateklo na neinicijalizovanom
steku, pa je `trapReturn` te vrednosti učitavao u registre nove niti.

Dok su prekidi bili maskirani to se nije videlo. Čim je uključen `sie.SEIE`,
prvi spoljašnji prekid u kontekstu takve niti odveo je u `plic_claim`, koji
računa adresu iz `tp` (redni broj jezgra):

```
scause=5 (load access fault)   sepc = plic_claim + 0x2c   # lw a0, 4(a0)
```

**FIX** (`src/trapEntry.S`): okvir se prvo ceo obriše, pa se eksplicitno upišu
`sp` i `tp`. `tp` se prenosi iz tekućeg konteksta jer važi za sve niti.

## Prelazak u režim rada: jedan zajednički put

`startUserThread` i `startKernelThread` razlikuju se samo po vrednosti bita
`SPP` koju upisuju u fabrikovani okvir; ostatak je zajednički
(`startThreadCommon`). Oba postavljaju i `SPIE = 1`, pa telo niti kreće sa
**dozvoljenim prekidima** i može da bude preoteto.

## Deljenje vremena

- `TCB::onTimerTick()` se zove iz grane `scause = 0x8000000000000001`, pošto se
  potvrdi prekid sa `mc_sip(SIP_SSIP)`.
- Preostali odsečak je **jedna statička promenljiva** (`TCB::remainingTimeSlice`),
  jer se odnosi samo na tekuću nit; `TCB::dispatch()` je postavlja na
  `timeSlice` niti koja preuzima procesor.

## Uspavljivanje: `SleepingQueue`

Red je uređen po trenutku buđenja, a uz svaku nit se čuva **relativno** vreme u
odnosu na prethodnu nit u redu. Na svaku periodu tajmera dekrementira se samo
prva nit, pa je `tick()` konstantne složenosti bez obzira na broj uspavanih
niti. Niti sa vremenom `0` na početku reda bude se zajedno (isti trenutak).

Red je intruzivan i koristi isti pokazivač `next` kao i red spremnih niti - nit
koja spava nije istovremeno ni u jednom drugom redu.

## Konzola

| smer | proizvođač | potrošač |
|---|---|---|
| ulaz | prekidna rutina (`handleInterrupt`) | sistemski poziv `getc` |
| izlaz | sistemski poziv `putc` | interna nit jezgra |

- Ulazni bafer: hardver se ne može blokirati, pa se znakovi koji stignu dok je
  bafer pun **odbacuju**. U jednoj obradi prekida čita se najviše
  `MAX_CHARS_PER_INTERRUPT` znakova, da obrada ne traje predugo.
- Izlazni bafer: nit koja poziva `putc` **blokira se** ako nema slobodnog mesta.
- Interna nit koristi dva interna sistemska poziva (`0x81`, `0x82`), koji su
  odbijeni ako ih pozove korisnička nit. Znak se iz bafera uklanja tek pozivom
  `0x82`, dakle **pošto je stvarno prenet na kontroler** - zato prazan izlazni
  bafer pouzdano znači „sve je ispisano", što koristi `_Console::flush()` pre
  gašenja jezgra i `flushDirect()` pre poruke o nepopravljivoj grešci.

### Provereno: nema bujice TX prekida

UART (`uartinit` u `hw.lib`) upisuje `IER = 3`, dakle dozvoljava i prekid „TX
spreman". Postojala je bojazan da bi taj prekid, kad nema šta da se šalje, ostao
trajno postavljen i zaglavio sistem. Mereno brojačem u prekidnoj rutini:
2000 spoljašnjih prekida se ne dostigne ni posle nekoliko sekundi, dakle bujice
nema i izlazni smer sme da se rešava prozivanjem iz interne niti.

## `PeriodicThread` bez novih podataka članova

Postavka zabranjuje proširivanje klasa C++ API-ja nestatičkim podacima
članovima, pa se zaustavljanje ne može označiti novim atributom. Koristi se sama
perioda: `terminate()` je postavlja na `0`, a `run()` se vrti dok je različita
od nule. Redefinisanje `run()` ne menja ni skup ni redosled virtuelnih funkcija,
jer je `run()` već virtuelna u klasi `Thread`.

## Makefile

Iz `LIBS` su izbačeni `mem.lib` i `console.lib`: i zadatak 1 (alokator) i
zadatak 4 (`getc`/`putc`) urađeni su u okviru projekta, pa se uvezuje samo
`hw.lib`.
