#!/usr/bin/env bash
#
# implementation of a compiler for the language IFJ26
# run_e2e.sh
# end-to-end tests of the whole compiler: every program in tests/e2e/ is
# compiled by our compiler (stdin -> stdout) and, if it should compile,
# interpreted by ic26int; exit codes and stdout are compared with the
# expectations written in the test. Every failure is explained in detail
# (meaning of the exit codes, probable module, error position in the source,
# diff of the output) and written to tests/e2e_out/report.txt.
# Not part of the compiler (test only, folder tests/).
# xbadint00 Tomáš Badinský
#
# AI GENERATED (Claude Opus 5.5, Anthropic) - whole file.
# Prompts (16. 11. 2026): "okej, ideme dalej, tie testy co si posielal predtym,
#   to su testy na cely prekladac alebo iba na tu moju cast? ak iba na tu moju
#   cast tak budem od teba potrebovat testy pre projekt ako celok, na ktorych
#   sa zisti ci splna zadanie a specifikaciu" and "... vytvor funkcny main.c
#   ktory bude splnat poziadavky, tak isto sprav aj obsiahle testy
#   s podrobnymi vypismi chyb pre projekt ako celok"
#
# Test format (tests/e2e/<category>/<name>.ifj26):
#   // TEST: <description>
#   // SPEC: <section of the specification>
#   // EXPECT: compile=<exit code of compiler> run=<exit code of ic26int>
#   // EXT: <EXTENSION>   optional; the expectation holds only WITHOUT this
#                         extension, the test is skipped if it is implemented
#   // HINT: <text>       optional; frequent cause, printed when the test fails
#   <name>.in   optional stdin for the interpreted program
#   <name>.out  expected stdout of the interpreted program (compared exactly)
#
# Usage:
#   tests/run_e2e.sh [options] [filter]
#     -c FILE   compiler (default: ./ifj26 in the project root, built by make)
#     -i FILE   interpreter (default: ic26int in PATH or ~/ic26int/ic26int)
#     -x LIST   implemented extensions, e.g. -x FUNEXP,BOOLTHEN
#               (default: lines of the file "rozsireni" in the project root)
#     -m        also run the compiler in valgrind: invalid reads/writes and
#               use of uninitialised values are failures, memory leaks of
#               successful compilations are reported as warnings
#     -s FILE   check the expectations with Scala instead of our compiler:
#               runs "scala run FILE prog.scala" (FILE = ifj26.scala) for every
#               test with compile=0 run=0 and compares stdout with <name>.out
#     -q        do not print the lines of passed tests
#     -v        print whole diffs and stderr (otherwise shortened)
#     filter    run only tests whose path contains this text, e.g. sem6
#
# Exit code: 0 if all tests passed, 1 otherwise.

set -u

DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
TESTS="$DIR/e2e"
OUTDIR="$DIR/e2e_out"
REPORT="$OUTDIR/report.txt"

COMPILER="$ROOT/ifj26"
INTERP=""
EXTS=""
SCALA_LIB=""
VERBOSE=0
QUIET=0
VALGRIND=0
TIMEOUT=10

while getopts "c:i:x:s:mqvh" opt; do
  case "$opt" in
  c) COMPILER="$OPTARG" ;;
  i) INTERP="$OPTARG" ;;
  x) EXTS="$OPTARG" ;;
  s) SCALA_LIB="$OPTARG" ;;
  m) VALGRIND=1 ;;
  q) QUIET=1 ;;
  v) VERBOSE=1 ;;
  *)
    sed -n '/^# Usage:/,/^# Exit code/p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
    ;;
  esac
done
shift $((OPTIND - 1))
FILTER="${1:-}"

if [ "${BASH_VERSINFO[0]}" -lt 4 ]; then
  echo "error: bash 4 or newer is needed" >&2
  exit 2
fi

# ---------------------------------------------------------------- setup

if [ -z "$EXTS" ] && [ -f "$ROOT/rozsireni" ]; then
  EXTS="$(tr -d '\r' <"$ROOT/rozsireni" | tr '\n' ',')"
fi

if [ -n "$SCALA_LIB" ]; then
  if ! command -v scala >/dev/null 2>&1; then
    echo "error: scala not found in PATH" >&2
    exit 2
  fi
  if [ ! -f "$SCALA_LIB" ]; then
    echo "error: $SCALA_LIB (ifj26.scala) not found" >&2
    exit 2
  fi
  SCALA_LIB="$(cd "$(dirname "$SCALA_LIB")" && pwd)/$(basename "$SCALA_LIB")"
  TIMEOUT=120
else
  if [ ! -x "$COMPILER" ]; then
    echo "error: compiler $COMPILER not found, run make first (or use -c)" >&2
    exit 2
  fi
  if [ -z "$INTERP" ]; then
    if command -v ic26int >/dev/null 2>&1; then
      INTERP="$(command -v ic26int)"
    elif [ -x "$HOME/ic26int/ic26int" ]; then
      INTERP="$HOME/ic26int/ic26int"
    elif [ -x "$ROOT/ic26int" ]; then
      INTERP="$ROOT/ic26int"
    fi
  fi
  if [ -z "$INTERP" ] || [ ! -x "$INTERP" ]; then
    echo "error: interpreter ic26int not found (use -i)" >&2
    exit 2
  fi
  if [ "$VALGRIND" -eq 1 ]; then
    if ! command -v valgrind >/dev/null 2>&1; then
      echo "error: valgrind not found (sudo apt install valgrind)" >&2
      exit 2
    fi
    TIMEOUT=60
  fi
fi

if ! command -v timeout >/dev/null 2>&1; then
  echo "error: command timeout (coreutils) is missing" >&2
  exit 2
fi

rm -rf "$OUTDIR"
mkdir -p "$OUTDIR"
: >"$REPORT"

# ---------------------------------------------------------------- meaning of exit codes

# text description of an exit code of our compiler
compiler_code_text() {
  case "$1" in
  0) echo "0 = preklad v poriadku" ;;
  1) echo "1 = lexikálna chyba" ;;
  2) echo "2 = syntaktická chyba" ;;
  3) echo "3 = nedefinovaná funkcia/premenná alebo chýba @main" ;;
  4) echo "4 = redefinícia funkcie/premennej" ;;
  5) echo "5 = zlý počet/typ argumentov alebo parametre @main" ;;
  6) echo "6 = typová chyba (výraz, priradenie, návratová hodnota, podmienka)" ;;
  7) echo "7 = zápis do val alebo parametra" ;;
  10) echo "10 = iná sémantická chyba" ;;
  99) echo "99 = interná chyba prekladača" ;;
  124) echo "124 = TIMEOUT (prekladač sa zacyklil)" ;;
  *)
    if [ "$1" -gt 128 ]; then
      echo "$1 = PÁD prekladača na signál $(($1 - 128)) $(signal_name $(($1 - 128)))"
    else
      echo "$1 = neznámy kód (špecifikácia ho nepozná)"
    fi
    ;;
  esac
}

# text description of an exit code of ic26int
interp_code_text() {
  case "$1" in
  0) echo "0 = beh v poriadku" ;;
  40) echo "40 = behová sémantická chyba (napr. subStr mimo reťazca)" ;;
  50) echo "50 = zlé parametre interpretu" ;;
  51) echo "51 = lexikálna/syntaktická chyba vo vygenerovanom IFJcode26" ;;
  52) echo "52 = sémantická chyba v IFJcode26 (redefinícia premennej/návestia, neexistujúce návestie)" ;;
  53) echo "53 = zlé typy operandov inštrukcie" ;;
  54) echo "54 = prístup k neexistujúcej premennej (chýba DEFVAR)" ;;
  55) echo "55 = rámec neexistuje (CREATEFRAME/PUSHFRAME/POPFRAME)" ;;
  56) echo "56 = chýbajúca hodnota (neinicializovaná premenná, prázdny dátový zásobník, RETURN bez CALL)" ;;
  57) echo "57 = zlá hodnota operandu (delenie nulou, EXIT mimo 0–49)" ;;
  58) echo "58 = chybná práca s reťazcom (index mimo, INT2CHAR mimo 0–127)" ;;
  60) echo "60 = interná chyba interpretu" ;;
  124) echo "124 = TIMEOUT (nekonečný cyklus vo vygenerovanom kóde?)" ;;
  *)
    if [ "$1" -gt 128 ]; then
      echo "$1 = pád interpretu na signál $(($1 - 128))"
    else
      echo "$1 = iný kód"
    fi
    ;;
  esac
}

signal_name() {
  case "$1" in
  6) echo "(SIGABRT – abort, napr. double free)" ;;
  8) echo "(SIGFPE – delenie nulou v C)" ;;
  11) echo "(SIGSEGV – zlý prístup do pamäte, napr. NULL)" ;;
  *) echo "" ;;
  esac
}

# module of our compiler responsible for an error code
module_of() {
  case "$1" in
  1) echo "lexer" ;;
  2) echo "parser" ;;
  3 | 4 | 5 | 6 | 7 | 10) echo "sémantika" ;;
  *) echo "?" ;;
  esac
}

# order of the phases (lexer < parser < semantics)
phase_index() {
  case "$1" in
  1) echo 1 ;;
  2) echo 2 ;;
  3 | 4 | 5 | 6 | 7 | 10) echo 3 ;;
  *) echo 0 ;;
  esac
}

# ---------------------------------------------------------------- counters

PASSED=0
FAILED=0
SKIPPED=0
MEMWARN=0
FAILED_LIST=""
declare -A CAT_PASS CAT_FAIL CAT_SKIP BLAME

count_cat() {
  local cat="$1" what="$2"
  case "$what" in
  pass) CAT_PASS[$cat]=$((${CAT_PASS[$cat]:-0} + 1)) ;;
  fail) CAT_FAIL[$cat]=$((${CAT_FAIL[$cat]:-0} + 1)) ;;
  skip) CAT_SKIP[$cat]=$((${CAT_SKIP[$cat]:-0} + 1)) ;;
  esac
}

# ---------------------------------------------------------------- output helpers

# writes a line to the console and to the report
both() {
  printf '%s\n' "$*"
  printf '%s\n' "$*" >>"$REPORT"
}

# writes a file (indented) to the report and shortened to the console
both_file() {
  local file="$1" lines="$2"
  if [ ! -s "$file" ]; then
    return
  fi
  sed 's/^/        | /' "$file" >>"$REPORT"
  if [ "$VERBOSE" -eq 1 ]; then
    sed 's/^/        | /' "$file"
  else
    head -n "$lines" "$file" | sed 's/^/        | /'
    local total
    total="$(wc -l <"$file")"
    if [ "$total" -gt "$lines" ]; then
      echo "        | ... ($((total - lines)) ďalších riadkov v report.txt, alebo spusti s -v)"
    fi
  fi
}

# source line of the error reported by the compiler ("at row R column C" of
# sem_error/print_error/gen_error, or "riadku R, stlpec C" of the lexer)
show_error_position() {
  local src="$1" cerr="$2" row col line
  row="$(grep -oE 'row [0-9]+' "$cerr" | head -n1 | grep -oE '[0-9]+')"
  col="$(grep -oE 'column [0-9]+' "$cerr" | head -n1 | grep -oE '[0-9]+')"
  if [ -z "$row" ]; then
    row="$(grep -oE 'riadku [0-9]+' "$cerr" | head -n1 | grep -oE '[0-9]+')"
    col="$(grep -oE 'stlpec [0-9]+' "$cerr" | head -n1 | grep -oE '[0-9]+')"
  fi
  if [ -z "$row" ] || [ "$row" -le 0 ]; then
    return
  fi
  line="$(sed -n "${row}p" "$src")"
  if [ -z "$line" ]; then
    return
  fi
  both "      miesto chyby podľa prekladača (riadok $row${col:+, stĺpec $col}):"
  both "        $(printf '%4d' "$row") | $line"
  if [ -n "$col" ] && [ "$col" -ge 1 ] && [ "$col" -le 300 ]; then
    both "             | $(printf '%*s' $((col - 1)) '')^"
  fi
}

# test program without the header, for the report only
program_to_report() {
  local src="$1"
  echo "      program:" >>"$REPORT"
  awk '!/^\/\/ (TEST|SPEC|EXPECT|EXT|HINT|AI GENERATED)/ { printf "        %4d | %s\n", NR, $0 }' "$src" >>"$REPORT"
}

# ---------------------------------------------------------------- failure report

# fail <name> <category> <src> <short reason> <hint> <blamed module>
fail_head() {
  local name="$1" cat="$2" src="$3" reason="$4"
  FAILED=$((FAILED + 1))
  FAILED_LIST="$FAILED_LIST $name"
  count_cat "$cat" fail
  local desc spec
  desc="$(grep -m1 '^// TEST:' "$src" | sed 's#^// TEST: ##')"
  spec="$(grep -m1 '^// SPEC:' "$src" | sed 's#^// SPEC: ##')"
  both ""
  both "FAIL  $name"
  both "      test:        $desc (špecifikácia $spec)"
  both "      súbor:       tests/e2e/$name.ifj26"
  both "      problém:     $reason"
  local hint
  hint="$(grep -m1 '^// HINT:' "$src" | sed 's#^// HINT: ##')"
  if [ -n "$hint" ]; then
    both "      častá príčina: $hint"
  fi
}

blame() {
  local module="$1" hint="$2"
  BLAME[$module]=$((${BLAME[$module]:-0} + 1))
  both "      kde hľadať:  $module – $hint"
}

# ---------------------------------------------------------------- one test

run_test() {
  local src="$1"
  local rel="${src#"$TESTS"/}"
  local name="${rel%.ifj26}"
  local cat="${name%%/*}"
  local base="${src%.ifj26}"
  local out_base="$OUTDIR/$name"
  mkdir -p "$(dirname "$out_base")"

  local expect_line ext_line exp_compile exp_run stdin_file
  expect_line="$(grep -m1 '^// EXPECT:' "$src")"
  ext_line="$(grep -m1 '^// EXT:' "$src")"
  exp_compile="$(printf '%s\n' "$expect_line" | sed -n 's/.*compile=\([0-9][0-9]*\).*/\1/p')"
  exp_run="$(printf '%s\n' "$expect_line" | sed -n 's/.*run=\([0-9][0-9]*\).*/\1/p')"
  exp_compile="${exp_compile:-0}"
  exp_run="${exp_run:-0}"
  stdin_file="/dev/null"
  if [ -f "$base.in" ]; then
    stdin_file="$base.in"
  fi

  if [ -n "$ext_line" ]; then
    local ext
    for ext in $(printf '%s\n' "$ext_line" | sed 's/^\/\/ EXT://; s/(.*//' | tr ',' ' '); do
      case ",$EXTS," in
      *",$ext,"*)
        SKIPPED=$((SKIPPED + 1))
        count_cat "$cat" skip
        [ "$QUIET" -eq 1 ] || printf 'SKIP  %-50s (rozšírenie %s je implementované)\n' "$name" "$ext"
        return
        ;;
      esac
    done
  fi

  # ---- Scala mode: only checks the expected stdout of valid programs
  if [ -n "$SCALA_LIB" ]; then
    if [ "$exp_compile" -ne 0 ] || [ "$exp_run" -ne 0 ] || [ ! -f "$base.out" ]; then
      SKIPPED=$((SKIPPED + 1))
      count_cat "$cat" skip
      return
    fi
    local work="$OUTDIR/scala_work"
    mkdir -p "$work"
    cp "$src" "$work/prog.scala"
    (cd "$work" && timeout "$TIMEOUT" scala run "$SCALA_LIB" prog.scala <"$stdin_file" >"$out_base.out" 2>"$out_base.err")
    local scala_rc=$?
    if [ "$scala_rc" -ne 0 ]; then
      fail_head "$name" "$cat" "$src" "scala skončila s kódom $scala_rc"
      both_file "$out_base.err" 8
    elif ! cmp -s "$base.out" "$out_base.out"; then
      fail_head "$name" "$cat" "$src" "výstup Scaly sa líši od $name.out (< očakávané, > Scala)"
      diff <(cat -A "$base.out") <(cat -A "$out_base.out") >"$out_base.diff"
      both_file "$out_base.diff" 10
      both "      → over, či je chyba v očakávaní testu, alebo ide o rozdiel Scaly a IFJ26 (špecifikácia 4.1)"
    else
      PASSED=$((PASSED + 1))
      count_cat "$cat" pass
      [ "$QUIET" -eq 1 ] || printf 'ok    %s\n' "$name"
    fi
    return
  fi

  # ---- compile
  local rc
  if [ "$VALGRIND" -eq 1 ]; then
    timeout "$TIMEOUT" valgrind --log-file="$out_base.vg" --leak-check=full \
      --errors-for-leak-kinds=none --track-origins=yes \
      "$COMPILER" <"$src" >"$out_base.code" 2>"$out_base.cerr"
    rc=$?
  else
    timeout "$TIMEOUT" "$COMPILER" <"$src" >"$out_base.code" 2>"$out_base.cerr"
    rc=$?
  fi

  if [ "$VALGRIND" -eq 1 ] && [ -f "$out_base.vg" ]; then
    local vg_errors
    vg_errors="$(grep -oE 'ERROR SUMMARY: [0-9]+' "$out_base.vg" | grep -oE '[0-9]+$' | tail -n1)"
    if [ -n "$vg_errors" ] && [ "$vg_errors" -gt 0 ]; then
      fail_head "$name" "$cat" "$src" "valgrind: $vg_errors chýb práce s pamäťou v prekladači"
      grep -E '^==[0-9]+== (Invalid|Conditional|Use of uninit|Process terminating| Address|   (at|by) 0x)' "$out_base.vg" |
        sed 's/^==[0-9]*== //' >"$out_base.vgshort"
      both_file "$out_base.vgshort" 12
      blame "pamäť" "funkcie v zásobníku volaní vyššie (at/by) ukazujú súbor a riadok; celý log: tests/e2e_out/$name.vg"
      return
    fi
    if [ "$rc" -eq 0 ] && grep -qE 'definitely lost: [1-9]' "$out_base.vg"; then
      MEMWARN=$((MEMWARN + 1))
      printf 'LEAK  %s: %s\n' "$name" "$(grep -oE 'definitely lost: [0-9,]+ bytes in [0-9,]+ blocks' "$out_base.vg" | head -n1)" >>"$REPORT"
    fi
  fi

  if [ "$rc" -ne "$exp_compile" ]; then
    fail_head "$name" "$cat" "$src" "prekladač vrátil $(compiler_code_text "$rc"), očakávané $(compiler_code_text "$exp_compile")"
    if [ -s "$out_base.cerr" ]; then
      both "      stderr prekladača:"
      both_file "$out_base.cerr" 4
    else
      both "      stderr prekladača: (prázdny)"
    fi
    show_error_position "$src" "$out_base.cerr"

    local exp_phase got_phase
    exp_phase="$(phase_index "$exp_compile")"
    got_phase="$(phase_index "$rc")"
    if [ "$rc" -eq 124 ]; then
      blame "?" "prekladač sa zacyklil; poslednú fázu zistíš z výpisov na stderr alebo v debuggeri"
    elif [ "$rc" -gt 128 ]; then
      blame "?" "prekladač spadol; spusti test s -m (valgrind) alebo v gdb: gdb --args $COMPILER < tests/e2e/$name.ifj26"
    elif [ "$rc" -eq 99 ]; then
      blame "?" "interná chyba – podľa hlášky na stderr (alokácia, codegen bez údajov zo sémantiky...)"
    elif [ "$exp_compile" -eq 0 ]; then
      blame "$(module_of "$rc")" "správny program bol odmietnutý vo fáze $(module_of "$rc")"
    elif [ "$rc" -eq 0 ]; then
      blame "$(module_of "$exp_compile")" "chybu mala odhaliť fáza $(module_of "$exp_compile"), ale program prešiel celým prekladom"
    elif [ "$got_phase" -lt "$exp_phase" ]; then
      blame "$(module_of "$rc")" "skoršia fáza ($(module_of "$rc")) odmietla program, ktorý má prejsť až do fázy $(module_of "$exp_compile")"
    elif [ "$got_phase" -gt "$exp_phase" ]; then
      blame "$(module_of "$exp_compile")" "chybu mala odhaliť už fáza $(module_of "$exp_compile"), program prešiel až do fázy $(module_of "$rc")"
    else
      blame "$(module_of "$rc")" "správne odhalená chyba, ale iný kód – porovnaj so špecifikáciou, kap. 6"
    fi
    program_to_report "$src"
    return
  fi
  if [ "$exp_compile" -ne 0 ]; then
    PASSED=$((PASSED + 1))
    count_cat "$cat" pass
    [ "$QUIET" -eq 1 ] || printf 'ok    %s\n' "$name"
    return
  fi

  # ---- interpret
  timeout "$TIMEOUT" "$INTERP" "$out_base.code" <"$stdin_file" >"$out_base.out" 2>"$out_base.ierr"
  rc=$?
  if [ "$rc" -ne "$exp_run" ]; then
    fail_head "$name" "$cat" "$src" "ic26int vrátil $(interp_code_text "$rc"), očakávané $(interp_code_text "$exp_run")"
    if [ -s "$out_base.ierr" ]; then
      both "      stderr interpretu:"
      both_file "$out_base.ierr" 5
    fi
    case "$rc" in
    51 | 52 | 53 | 54 | 55 | 56)
      blame "codegen" "vygenerovaný kód je chybný ($(interp_code_text "$rc")); kód je v tests/e2e_out/$name.code"
      ;;
    124)
      blame "codegen" "nekonečný cyklus – skontroluj skoky while/if (tests/e2e_out/$name.code)"
      ;;
    0)
      blame "codegen" "program mal skončiť chybou $exp_run, ale prebehol – chýba kontrola vo vstavanej funkcii / inštrukcii"
      ;;
    *)
      if [ "$exp_run" -eq 0 ]; then
        blame "codegen" "správny program skončil chybou za behu (tests/e2e_out/$name.code)"
      else
        blame "codegen" "iná chyba za behu, než očakáva test (tests/e2e_out/$name.code)"
      fi
      ;;
    esac
    program_to_report "$src"
    return
  fi
  if [ -f "$base.out" ] && ! cmp -s "$base.out" "$out_base.out"; then
    fail_head "$name" "$cat" "$src" "stdout sa líši od očakávaného (< očakávané, > skutočné; ^I = tab, ^M = \\r, \$ = koniec riadku)"
    diff <(cat -A "$base.out") <(cat -A "$out_base.out") >"$out_base.diff"
    both_file "$out_base.diff" 10
    blame "codegen" "zlý výsledok výpočtu alebo výpisu; ak sa líši text literálu, môže to byť aj lexer (escape sekvencie, hodnota čísla)"
    program_to_report "$src"
    return
  fi
  PASSED=$((PASSED + 1))
  count_cat "$cat" pass
  [ "$QUIET" -eq 1 ] || printf 'ok    %s\n' "$name"
}

# ---------------------------------------------------------------- main

{
  echo "IFJ26 end-to-end testy – $(date '+%Y-%m-%d %H:%M')"
  if [ -n "$SCALA_LIB" ]; then
    echo "režim: kontrola očakávaní Scalou ($SCALA_LIB)"
  else
    echo "prekladač:   $COMPILER"
    echo "interpret:   $INTERP"
  fi
  echo "rozšírenia:  ${EXTS:-(žiadne)}"
} | tee -a "$REPORT"
if [ "$VALGRIND" -eq 1 ]; then
  both "valgrind:    zapnutý"
fi
echo

while IFS= read -r src; do
  case "$src" in
  *"$FILTER"*) run_test "$src" ;;
  esac
done < <(find "$TESTS" -name '*.ifj26' | sort)

# ---------------------------------------------------------------- summary

both ""
both "================================================================ súhrn"
both "$(printf '%-10s %7s %7s %7s' kategória ok fail skip)"
for cat in $(find "$TESTS" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort); do
  p="${CAT_PASS[$cat]:-0}"
  f="${CAT_FAIL[$cat]:-0}"
  s="${CAT_SKIP[$cat]:-0}"
  if [ $((p + f + s)) -gt 0 ]; then
    both "$(printf '%-10s %7d %7d %7d' "$cat" "$p" "$f" "$s")"
  fi
done
both "$(printf '%-10s %7d %7d %7d' SPOLU "$PASSED" "$FAILED" "$SKIPPED")"

if [ "$FAILED" -ne 0 ]; then
  both ""
  both "pravdepodobný modul pri neúspešných testoch:"
  for module in "${!BLAME[@]}"; do
    both "$(printf '  %-10s %3d' "$module" "${BLAME[$module]}")"
  done
fi
if [ "$MEMWARN" -ne 0 ]; then
  both ""
  both "valgrind: $MEMWARN úspešných prekladov má neuvoľnenú pamäť (definitely lost), zoznam v report.txt"
fi
both ""
both "passed=$PASSED failed=$FAILED skipped=$SKIPPED"
echo "podrobný report: $REPORT"
if [ "$FAILED" -ne 0 ]; then
  exit 1
fi
exit 0
