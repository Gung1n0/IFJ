# End-to-end testy prekladača IFJ26

AI GENERATED (Claude Opus 5.5, Anthropic): testy v `tests/e2e/`, skript
`tests/run_e2e.sh` a tento súbor.


## Spustenie (WSL / Linux / Merlin)

```sh
make                                   # vytvorí ./ifj26
tests/run_e2e.sh                       # všetky testy
tests/run_e2e.sh -q                    # vypíše len neúspešné testy a súhrn
tests/run_e2e.sh -i ~/ic26int/ic26int  # cesta k interpretu, ak nie je v PATH
tests/run_e2e.sh sem6                  # len testy, ktorých cesta obsahuje "sem6"
tests/run_e2e.sh -v ok/read_functions  # celé diffy a stderr
tests/run_e2e.sh -m -q                 # navyše valgrind na prekladači
```

Interpret sa hľadá v `PATH`, potom `~/ic26int/ic26int`, potom `./ic26int`.
Potrebný je bash 4+, `timeout` (coreutils) a pre `-m` aj `valgrind`.

## Čo sa vypíše pri chybe

Pre každý neúspešný test:

```
FAIL  ok/operator_not_equal
      test:        operator != on Int, Double and String (špecifikácia 3)
      súbor:       tests/e2e/ok/operator_not_equal.ifj26
      problém:     prekladač vrátil 6 = typová chyba (...), očakávané 0 = preklad v poriadku
      častá príčina: parser must create OP_NOT_EQUAL for != ...
      stderr prekladača:
        | types error at row 11 column 13
      miesto chyby podľa prekladača (riadok 11, stĺpec 13):
          11 |   if a != b then
             |             ^
      kde hľadať:  sémantika – správny program bol odmietnutý vo fáze sémantika
```

- **problém**: oba návratové kódy aj s významom podľa špecifikácie (kap. 6 pre
  prekladač, kap. 7.1 pre ic26int). Pád na signál (SIGSEGV…) a zacyklenie
  (timeout) sa hlásia zvlášť.
- **miesto chyby**: riadok zdrojového kódu so šípkou. Číta sa z hlášky
  `at row R column C` (sem_error, gen_error, print_error) alebo z hlášky
  lexera `na riadku R, stlpec C`.
- **kde hľadať**: odhad modulu podľa kódov. Napríklad očakávaná 6, ale vrátená 2
  znamená, že parser odmietol program, ktorý mal prejsť do sémantiky.
  Chyby 51–56 za behu znamenajú chybný vygenerovaný kód (codegen).
- **častá príčina**: pri niektorých testoch je v hlavičke `// HINT:` so
  známou príčinou.
- pri zlom výstupe sa ukáže diff s viditeľnými znakmi (`^I` tab, `^M` `\r`,
  `$` koniec riadku).
- s `-m` aj chyby pamäte z valgrindu (Invalid read/write, neinicializované
  hodnoty) s miestom v kóde. Úniky pamäte úspešných prekladov sú v reporte ako
  `LEAK`.

Na konci je súhrn po kategóriách a počet neúspechov podľa pravdepodobného
modulu. Všetko, aj celý program každého neúspešného testu, je v
`tests/e2e_out/report.txt`. Ten sa dá poslať kolegovi. Výstupy posledného behu
(`.code`, `.out`, `.cerr`, `.ierr`, `.diff`, `.vg`) sú v `tests/e2e_out/`.
Tento priečinok nedávajte do gitu ani do archívu.

## Najprv over očakávania Scalou

Očakávané výstupy (`*.out`) sú odvodené zo špecifikácie a výpisy desatinných
čísel (`%a`, `%.2f`) sú overené cez `printf` v C. Pred prvým použitím ich over
referenčnou implementáciou (Scala 3.8.4 + `ifj26.scala` z e-learningu):

```sh
tests/run_e2e.sh -s /cesta/k/ifj26.scala
```

Pre každý test, ktorý sa má preložiť a skončiť s kódom 0, sa spustí
`scala run ifj26.scala prog.scala` s rovnakým stdin. Výstup sa porovná so
súborom `.out`. Keď Scala vypíše niečo iné, test je v tomto režime FAIL a treba
zistiť, či je chyba v očakávaní, alebo v rozdiele Scaly oproti IFJ26 (sekcia 4.1).

## Formát testu

`tests/e2e/<kategória>/<meno>.ifj26`. Hlavička je komentár, takže je to stále
platný program v IFJ26 aj v Scale:

```
// TEST: popis
// SPEC: sekcia špecifikácie
// EXPECT: compile=<kód prekladača> run=<kód ic26int>
// EXT: BOOLTHEN    (nepovinné – platí len bez tohto rozšírenia)
// HINT: ...        (nepovinné – častá príčina pri neúspechu)
```

- `<meno>.in`: stdin interpretovaného programu (nepovinné, inak `/dev/null`),
- `<meno>.out`: presný očakávaný stdout (porovnáva sa bajt po bajte).

Ak `compile` nie je 0, kontroluje sa len návratový kód prekladača. Text chybovej
hlášky na stderr zadanie nepredpisuje, preto sa nekontroluje.

## Kategórie (192 testov)

| priečinok | počet | čo sa očakáva | špecifikácia |
|---|---|---|---|
| `ok/` | 56 | preklad 0, beh 0, presný výstup | 1–4 |
| `runtime/` | 14 | preklad 0, beh 40 / 57 / 58 | 3, 4, 6, 7.4 |
| `lex/` | 14 | preklad 1 | 1.1, 1.2 |
| `syntax/` | 45 | preklad 2 | 1.2, 2, 2.3–2.5, 3 |
| `sem3/` | 11 | preklad 3 (nedefinované, chýba `@main`) | 2, 2.2, 2.3 |
| `sem4/` | 6 | preklad 4 (redefinícia) | 2, 2.2, 2.3 |
| `sem5/` | 13 | preklad 5 (argumenty, parametre `@main`) | 2, 2.3, 4 |
| `sem6/` | 29 | preklad 6 (typy, návratové pravidlá, rekurzia) | 1.3, 2–3 |
| `sem7/` | 4 | preklad 7 (zápis do `val` alebo parametra) | 2.2, 2.3 |

V `ok/` sú aj záťažové testy: 12 vnorených blokov, reťazec s 1000 znakmi,
identifikátor s 200 znakmi, 500 príkazov, 60 funkcií a rekurzia do hĺbky 1000.

## Rozšírenia

Niektoré testy platia len pre základné zadanie, napríklad `if` bez `else` je chyba 2
len bez rozšírenia BOOLTHEN. Takéto testy majú riadok `// EXT: <ROZŠÍRENIE>` a
vynechajú sa (SKIP), ak je rozšírenie implementované. Zoznam sa berie zo súboru
`rozsireni` v koreni projektu (rovnaký súbor ide do archívu) alebo z prepínača
`-x FUNEXP,BOOLTHEN`.

## Čo sa zámerne netestuje

Ide o prípady, kde špecifikácia nedáva jednoznačný kód alebo výslovne hovorí,
že sa testovať nebudú:
- reťazenie relačných operátorov (`a < b < c`): dá sa chápať ako chyba 2 aj 6,
- `@ main` s medzerou (1 alebo 2),
- pretečenie Int a kontrola rozsahu literálov (špecifikácia 1.1 a 1.3),
- iné číselné sústavy a prípony `L`, `f` (môže to byť chyba 1 aj 2),
- kľúčové slová rozšírení ako identifikátory (podľa 1.1 sa testovať nebude),
- samotné rozšírenia.
