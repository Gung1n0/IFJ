#!/bin/bash
#
# implementation of a compiler for the language IFJ26
# run_codegen_tests.sh
# builds tests/codegen_tests.c together with the code generator and the
# semantic analysis, runs every test in the official interpreter ic26int and
# compares the output and the exit code with the expected ones.
# Not part of the compiler (test only, folder tests/).
# xbadint00 Tomáš Badinský
#
# AI GENERATED (Claude Opus 5.5, Anthropic) - whole file.
# Prompt (14. 11. 2026): "dobre chcem teda otestovat tieto codegen subory ako
#   to mam spravit s tym co mam"
#
# Usage (from anywhere):
#   tests/run_codegen_tests.sh [path/to/ic26int]
# Without the argument the interpreter is taken from the variable IC26INT,
# then from tests/ic26int, then from PATH.
#
# Generated code and outputs stay in tests/out/ (NAME.code, NAME.stdout,
# NAME.stderr), so a failing test can be run by hand:
#   ic26int tests/out/NAME.code < input

TESTS_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$TESTS_DIR/.." && pwd)
OUT="$TESTS_DIR/out"
mkdir -p "$OUT"

# ---------------- interpreter ----------------
IC26INT="${1:-${IC26INT:-}}"
if [ -z "$IC26INT" ]; then
  if [ -x "$TESTS_DIR/ic26int" ]; then
    IC26INT="$TESTS_DIR/ic26int"
  elif command -v ic26int > /dev/null 2>&1; then
    IC26INT=$(command -v ic26int)
  fi
fi
if [ -z "$IC26INT" ] || [ ! -x "$IC26INT" ]; then
  echo "ic26int not found: pass its path as the first argument, e.g."
  echo "  tests/run_codegen_tests.sh ~/ic26int/ic26int"
  exit 2
fi
echo "interpreter: $IC26INT"

# ---------------- sources ----------------
# only the modules the test needs (no main.c, Parser.c, lexer.c); every file
# must exist exactly once in the repository (tests/ and .git/ are skipped)
SOURCES=()
for name in codegen.c semantics.c symtable.c hashtable.c typerules.c; do
  mapfile -t found < <(find "$ROOT" \( -path "$TESTS_DIR" -o -name .git \) -prune -o -type f -name "$name" -print)
  if [ "${#found[@]}" -ne 1 ]; then
    echo "expected exactly one $name in $ROOT, found ${#found[@]}:"
    printf '  %s\n' "${found[@]}"
    exit 2
  fi
  SOURCES+=("${found[0]}")
done

# -I for every folder with a header (same as the Makefile)
INCLUDES=()
while IFS= read -r dir; do
  INCLUDES+=("-I$dir")
done < <(find "$ROOT" \( -path "$TESTS_DIR" -o -name .git \) -prune -o -type f -name '*.h' -printf '%h\n' | sort -u)

echo "building tests ..."
if ! gcc -std=c11 -Wall -Wextra -g "${INCLUDES[@]}" -o "$OUT/codegen_tests" \
  "$TESTS_DIR/codegen_tests.c" "${SOURCES[@]}"; then
  echo "build failed"
  exit 2
fi

# ---------------- tests ----------------
pass=0
fail=0

# check NAME STDIN EXPECTED_STDOUT EXPECTED_EXIT_CODE
# STDIN and EXPECTED_STDOUT may contain \n and \t (printf %b)
check() {
  local name="$1" input="$2" expected="$3" expected_rc="$4"

  if ! "$OUT/codegen_tests" "$name" > "$OUT/$name.code" 2> "$OUT/$name.gen_stderr"; then
    echo "FAIL $name (compiler part failed, see $OUT/$name.gen_stderr)"
    fail=$((fail + 1))
    return
  fi

  printf '%b' "$input" | "$IC26INT" "$OUT/$name.code" > "$OUT/$name.stdout" 2> "$OUT/$name.stderr"
  local rc=$?
  printf '%b' "$expected" > "$OUT/$name.expected"

  if [ "$rc" -eq "$expected_rc" ] && cmp -s "$OUT/$name.stdout" "$OUT/$name.expected"; then
    echo "ok   $name"
    pass=$((pass + 1))
  else
    echo "FAIL $name (exit code $rc, expected $expected_rc)"
    diff "$OUT/$name.expected" "$OUT/$name.stdout" | sed 's/^/     /'
    fail=$((fail + 1))
  fi
}

check factorial '5\n' '120\n' 0
check factorial '' '1\n' 0
check arith '' '3\n-3\n0x1.1p+3\n0x1.8p+1\n3.00\nabcd\n1\n-7\n4.67\n4\n5\n' 0
check strings '' '12\nHello\nWorld\n101\n-1\n-1\nA\nx()()\na b#c\\d\t|\n-42' 0
check substr_err1 '' 'before' 40
check substr_err2 '' '' 40
check substr_ok_empty '' '|' 0
check chr_err '' '' 58
check divzero '' '' 57
check scopes '' 'inner\n1\n0 1 4 ' 0
check functions '' '55\n0x1.8p+1\n8\nHi Bob()\n81\nnoop()\n0x1p+1\ne1\n' 0
check relational '' 'eq gt2 ge3 strlt ne4 sge' 0
check coerce '' '0x1p+1\n0x1.cp+2\n' 0
check io 'hello\n3.5\nabc\n' 'hello\n0x1.cp+1\n0\n[]' 0
check negzero '' '-0x1.4p+1\n-0x0p+0' 0
check newsem '' '0x1.cp+2\n()\n()\ndirect\n-0x1.4p+2\nle gt' 0

echo "passed=$pass failed=$fail"
[ "$fail" -eq 0 ]
