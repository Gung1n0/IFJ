/*
 * implementation of a compiler for the language IFJ26
 * codegen_tests.c
 * tests of the code generator without the parser: every test builds an AST by
 * hand (the same nodes Parser.c would create), runs the semantic analysis
 * (sem_analyze) and then the code generator (codegen_generate). The generated
 * IFJcode26 is printed to stdout, run_codegen_tests.sh runs it in ic26int.
 * Not part of the compiler (test only, folder tests/).
 * xbadint00 Tomáš Badinský
 *
 * AI GENERATED (Claude Opus 5.5, Anthropic) - whole file.
 * Prompt (14. 11. 2026): "dobre chcem teda otestovat tieto codegen subory ako
 *   to mam spravit s tym co mam"
 *
 * Usage: codegen_tests <test name>        e.g. codegen_tests factorial
 * Some tests use expressions as call arguments (print(a + b)) - in the base
 * assignment only terms are allowed there (extension FUNEXP), the generator
 * supports both.
 */

#include "ast.h"
#include "codegen.h"
#include "semantics.h"
#include "symtable.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- helpers that build AST nodes (like Parser.c)
 * ---------------- */

/// @brief Creates an empty node of the given type
static node *mk(node_type type) {
  node *n = calloc(1, sizeof(node));
  n->type = type;
  n->row = 1;
  n->column = 1;
  return n;
}

/// @brief Copies a string (the AST owns its strings, free_test_node frees them)
static char *dup(const char *s) {
  char *p = malloc(strlen(s) + 1);
  strcpy(p, s);
  return p;
}

/// @brief Int literal
static node *LI(int v) {
  node *n = mk(node_lit);
  n->as.lit.type = TYPE_INT;
  n->as.lit.i = v;
  return n;
}

/// @brief Double literal
static node *LD(double v) {
  node *n = mk(node_lit);
  n->as.lit.type = TYPE_DOUBLE;
  n->as.lit.d = v;
  return n;
}

/// @brief String literal (escape sequences already processed, as from lexer)
static node *LS(const char *s) {
  node *n = mk(node_lit);
  n->as.lit.type = TYPE_STRING;
  n->as.lit.s = dup(s);
  return n;
}

/// @brief Unit literal ()
static node *LU(void) {
  node *n = mk(node_lit);
  n->as.lit.type = TYPE_UNIT;
  return n;
}

/// @brief Use of a variable
static node *ID(const char *name) {
  node *n = mk(node_id);
  n->as.ident.name = dup(name);
  return n;
}

/// @brief Binary operation left op right
static node *BIN(operator op, node *left, node *right) {
  node *n = mk(node_binar);
  n->as.binar.op = op;
  n->as.binar.left = left;
  n->as.binar.right = right;
  return n;
}

/// @brief Unary minus -e
static node *NEG(node *e) {
  node *n = mk(node_unar);
  n->as.unar.op = OP_NEGATE;
  n->as.unar.operand = e;
  return n;
}

/// @brief Function call name(args...)
static node *CALL(const char *name, int argc, ...) {
  node *n = mk(node_call);
  n->as.call.name = dup(name);
  n->as.call.items = calloc(argc ? argc : 1, sizeof(node *));
  va_list args;
  va_start(args, argc);
  for (int i = 0; i < argc; i++)
    n->as.call.items[i] = va_arg(args, node *);
  va_end(args);
  n->as.call.size = argc;
  return n;
}

/// @brief Expression used as a statement
static node *ES(node *e) {
  node *n = mk(node_expression);
  n->as.expression.expression = e;
  return n;
}

/// @brief val / var name [: type] = init (TYPE_UNKNOWN = type not written)
static node *VAL_(bool changable, const char *name, data_types type,
                  node *init) {
  node *n = mk(node_val_var);
  n->as.val_var.name = dup(name);
  n->as.val_var.changable = changable;
  n->as.val_var.type = type;
  n->as.val_var.init = init;
  return n;
}
#define VAL(name, type, init) VAL_(false, name, type, init)
#define VAR(name, type, init) VAL_(true, name, type, init)

/// @brief Assignment name = init
static node *ASG(const char *name, node *init) {
  node *n = mk(node_asign);
  n->as.asign.name = dup(name);
  n->as.asign.init = init;
  return n;
}

/// @brief return [value] (NULL = return without value)
static node *RET(node *value) {
  node *n = mk(node_return);
  n->as.ret.value = value;
  return n;
}

/// @brief if condition then ... else ...
static node *IF(node *condition, node *then_block, node *else_block) {
  node *n = mk(node_if);
  n->as.if_n.condition = condition;
  n->as.if_n.then = then_block;
  n->as.if_n.else_n = else_block;
  return n;
}

/// @brief while condition do ...
static node *WHILE(node *condition, node *body) {
  node *n = mk(node_while);
  n->as.while_n.condition = condition;
  n->as.while_n.body = body;
  return n;
}

/// @brief Block with count statements
static node *BLK(int count, ...) {
  node *n = mk(node_block);
  n->as.block.items = calloc(count ? count : 1, sizeof(node *));
  va_list args;
  va_start(args, count);
  for (int i = 0; i < count; i++)
    n->as.block.items[i] = va_arg(args, node *);
  va_end(args);
  n->as.block.size = count;
  return n;
}
/* BLOCK(a, b, c) counts its statements automatically */
#define BLOCK(...)                                                             \
  BLK((int)(sizeof((node *[]){__VA_ARGS__}) / sizeof(node *)), __VA_ARGS__)

/// @brief Function definition; after param_count come pairs (name, type)
static node *FN(const char *name, data_types return_type, bool explicit_type,
                bool is_main, node *body, int param_count, ...) {
  node *n = mk(node_function);
  n->as.function.name = dup(name);
  n->as.function.return_type = return_type;
  n->as.function.explicit = explicit_type;
  n->as.function.is_main = is_main;
  n->as.function.body = body;
  n->as.function.param_count = param_count;
  n->as.function.params = calloc(param_count ? param_count : 1, sizeof(param));
  va_list args;
  va_start(args, param_count);
  for (int i = 0; i < param_count; i++) {
    n->as.function.params[i].name = dup(va_arg(args, const char *));
    n->as.function.params[i].types = (data_types)va_arg(args, int);
  }
  va_end(args);
  return n;
}

/// @brief Whole program with count functions
static node *PROG(int count, ...) {
  node *n = mk(node_program);
  n->as.program.items = calloc(count, sizeof(node *));
  va_list args;
  va_start(args, count);
  for (int i = 0; i < count; i++)
    n->as.program.items[i] = va_arg(args, node *);
  va_end(args);
  n->as.program.size = count;
  return n;
}

#define PRINT(e) ES(CALL("print", 1, e))
#define NL PRINT(LS("\n"))
#define MAIN(body) FN("main", TYPE_UNIT, true, true, body, 0)

/// @brief Frees the test AST (same as free_node in Parser.c)
static void free_test_node(node *n) {
  if (n == NULL)
    return;
  switch (n->type) {
  case node_program:
    for (int i = 0; i < n->as.program.size; i++)
      free_test_node(n->as.program.items[i]);
    free(n->as.program.items);
    break;
  case node_function:
    free(n->as.function.name);
    free_test_node(n->as.function.body);
    for (int i = 0; i < n->as.function.param_count; i++)
      free(n->as.function.params[i].name);
    free(n->as.function.params);
    break;
  case node_block:
    for (int i = 0; i < n->as.block.size; i++)
      free_test_node(n->as.block.items[i]);
    free(n->as.block.items);
    break;
  case node_val_var:
    free(n->as.val_var.name);
    free_test_node(n->as.val_var.init);
    break;
  case node_asign:
    free(n->as.asign.name);
    free_test_node(n->as.asign.init);
    break;
  case node_expression:
    free_test_node(n->as.expression.expression);
    break;
  case node_return:
    free_test_node(n->as.ret.value);
    break;
  case node_if:
    free_test_node(n->as.if_n.condition);
    free_test_node(n->as.if_n.then);
    free_test_node(n->as.if_n.else_n);
    break;
  case node_while:
    free_test_node(n->as.while_n.condition);
    free_test_node(n->as.while_n.body);
    break;
  case node_lit:
    if (n->as.lit.type == TYPE_STRING)
      free(n->as.lit.s);
    break;
  case node_id:
    free(n->as.ident.name);
    break;
  case node_unar:
    free_test_node(n->as.unar.operand);
    break;
  case node_binar:
    free_test_node(n->as.binar.left);
    free_test_node(n->as.binar.right);
    break;
  case node_call:
    free(n->as.call.name);
    for (int i = 0; i < n->as.call.size; i++)
      free_test_node(n->as.call.items[i]);
    free(n->as.call.items);
    break;
  }
  free(n);
}

/* ---------------- tests (expected outputs are in run_codegen_tests.sh)
 * ---------------- */

/// @brief Builds the AST of the test with the given name, NULL if unknown
static node *build_test(const char *t) {
  /* factorial: factorial from the specification (chapter 2): readInt, while,
   * function with a parameter */
  if (!strcmp(t, "factorial")) {
    node *fact = FN(
        "factorial", TYPE_INT, true, false,
        BLOCK(
            VAR("result", TYPE_UNKNOWN, LI(1)), VAR("i", TYPE_INT, ID("n")),
            WHILE(BIN(OP_GREATER, ID("i"), LI(1)),
                  BLOCK(ASG("result", BIN(OP_MULTIPLY, ID("result"), ID("i"))),
                        ASG("i", BIN(OP_SUBTRACT, ID("i"), LI(1))))),
            ES(ID("result"))),
        1, "n", TYPE_INT);
    node *m =
        FN("runFoo", TYPE_UNIT, true, true,
           BLOCK(VAL("n", TYPE_UNKNOWN, CALL("readInt", 0)),
                 VAL("result", TYPE_UNKNOWN, CALL("factorial", 1, ID("n"))),
                 PRINT(ID("result")), NL),
           0);
    return PROG(2, fact, m);
  }
  /* arith: arithmetic: Int / Int (IDIVS), Int + Double (INT2FLOATS), String +
   * String (CONCAT), toStr, toInt */
  if (!strcmp(t, "arith")) {
    return PROG(
        1,
        MAIN(BLOCK(
            VAL("a", TYPE_UNKNOWN, LI(7)), VAL("b", TYPE_UNKNOWN, LI(2)),
            PRINT(BIN(OP_DIVIDE, ID("a"), ID("b"))), NL,
            VAL("c", TYPE_UNKNOWN, NEG(LI(7))),
            PRINT(BIN(OP_DIVIDE, ID("c"), ID("b"))), NL,
            VAL("d", TYPE_UNKNOWN, LD(1.5)),
            PRINT(BIN(OP_ADD, ID("a"), ID("d"))), NL,
            VAL("e", TYPE_DOUBLE, LI(3)), PRINT(ID("e")), NL,
            PRINT(CALL("toStr", 1, ID("e"))), NL,
            VAL("s", TYPE_UNKNOWN, BIN(OP_ADD, LS("ab"), LS("cd"))),
            PRINT(ID("s")), NL,
            PRINT(BIN(OP_SUBTRACT, ID("a"), BIN(OP_MULTIPLY, ID("b"), LI(3)))),
            NL, PRINT(NEG(ID("a"))), NL,
            VAL("x", TYPE_UNKNOWN, BIN(OP_DIVIDE, ID("a"), ID("d"))),
            PRINT(CALL("toStr", 1, ID("x"))), NL,
            PRINT(CALL("toInt", 1, ID("x"))), NL,
            PRINT(CALL("toInt", 1, LI(5))), NL)));
  }
  /* strings: built-ins strLength, subStr, ord, chr; Unit value () in print and
   * toStr; escape sequences */
  if (!strcmp(t, "strings")) {
    return PROG(1,
                MAIN(BLOCK(VAL("s", TYPE_UNKNOWN, LS("Hello, World")),
                           PRINT(CALL("strLength", 1, ID("s"))), NL,
                           PRINT(CALL("subStr", 3, ID("s"), LI(0), LI(5))), NL,
                           PRINT(CALL("subStr", 3, ID("s"), LI(7), LI(12))), NL,
                           PRINT(CALL("ord", 2, ID("s"), LI(1))), NL,
                           PRINT(CALL("ord", 2, ID("s"), LI(100))), NL,
                           PRINT(CALL("ord", 2, LS(""), LI(0))), NL,
                           PRINT(CALL("chr", 1, LI(65))), NL,
                           VAL("u", TYPE_UNKNOWN, CALL("print", 1, LS("x"))),
                           PRINT(ID("u")), PRINT(CALL("toStr", 1, ID("u"))), NL,
                           PRINT(LS("a b#c\\d\t|")), NL,
                           PRINT(CALL("toStr", 1, LI(-42))))));
  }
  /* substr_err1: subStr with start > end -> runtime error 40 (output before it
   * stays) */
  if (!strcmp(t, "substr_err1"))
    return PROG(1,
                MAIN(BLOCK(PRINT(LS("before")),
                           PRINT(CALL("subStr", 3, LS("abc"), LI(2), LI(1))))));
  /* substr_err2: subStr with start == strLength -> runtime error 40 */
  if (!strcmp(t, "substr_err2"))
    return PROG(1,
                MAIN(BLOCK(PRINT(CALL("subStr", 3, LS("abc"), LI(3), LI(3))))));
  /* substr_ok_empty: subStr with start == end -> empty string */
  if (!strcmp(t, "substr_ok_empty"))
    return PROG(1, MAIN(BLOCK(PRINT(CALL("subStr", 3, LS("abc"), LI(1), LI(1))),
                              PRINT(LS("|")))));
  /* chr_err: chr(200) -> interpreter error 58 */
  if (!strcmp(t, "chr_err"))
    return PROG(1, MAIN(BLOCK(PRINT(CALL("chr", 1, LI(200))))));
  /* divzero: Int division by zero -> interpreter error 57 */
  if (!strcmp(t, "divzero"))
    return PROG(1, MAIN(BLOCK(VAL("z", TYPE_UNKNOWN, LI(0)),
                              PRINT(BIN(OP_DIVIDE, LI(1), ID("z"))))));
  /* scopes: shadowing in an if branch; val defined inside while (DEFVAR only
   * once per call) */
  if (!strcmp(t, "scopes")) {
    return PROG(
        1, MAIN(BLOCK(VAL("x", TYPE_UNKNOWN, LI(1)),
                      IF(BIN(OP_LESSER, ID("x"), LI(5)),
                         BLOCK(VAL("x", TYPE_UNKNOWN, LS("inner")),
                               PRINT(ID("x")), NL),
                         BLOCK(PRINT(LS("no")))),
                      PRINT(ID("x")), NL, VAR("i", TYPE_UNKNOWN, LI(0)),
                      WHILE(BIN(OP_LESSER, ID("i"), LI(3)),
                            BLOCK(VAL("sq", TYPE_UNKNOWN,
                                      BIN(OP_MULTIPLY, ID("i"), ID("i"))),
                                  PRINT(ID("sq")), PRINT(LS(" ")),
                                  ASG("i", BIN(OP_ADD, ID("i"), LI(1))))))));
  }
  /* functions: user functions: recursion, return, Double return type, Unit
   * function returning a value, inferred return type, early return */
  if (!strcmp(t, "functions")) {
    node *fib =
        FN("fib", TYPE_INT, true, false,
           BLOCK(IF(
               BIN(OP_LESSER, ID("n"), LI(2)), BLOCK(RET(ID("n"))),
               BLOCK(VAL("n1", TYPE_UNKNOWN, BIN(OP_SUBTRACT, ID("n"), LI(1))),
                     VAL("n2", TYPE_UNKNOWN, BIN(OP_SUBTRACT, ID("n"), LI(2))),
                     VAL("a", TYPE_UNKNOWN, CALL("fib", 1, ID("n1"))),
                     VAL("b", TYPE_UNKNOWN, CALL("fib", 1, ID("n2"))),
                     RET(BIN(OP_ADD, ID("a"), ID("b")))))),
           1, "n", TYPE_INT);
    node *half = FN("half", TYPE_DOUBLE, true, false, BLOCK(RET(ID("x"))), 1,
                    "x", TYPE_INT);
    node *first = FN(
        "firstOver", TYPE_INT, true, false,
        BLOCK(VAR("i", TYPE_UNKNOWN, LI(0)),
              WHILE(BIN(OP_LESSER, ID("i"), LI(100)),
                    BLOCK(IF(BIN(OP_GREATER, BIN(OP_MULTIPLY, ID("i"), ID("i")),
                                 ID("limit")),
                             BLOCK(RET(ID("i"))), BLOCK(ES(LU()))),
                          ASG("i", BIN(OP_ADD, ID("i"), LI(1))))),
              RET(NEG(LI(1)))),
        1, "limit", TYPE_INT);
    node *greet =
        FN("greet", TYPE_UNIT, true, false,
           BLOCK(PRINT(BIN(OP_ADD, LS("Hi "), ID("name"))), ES(LI(42))), 1,
           "name", TYPE_STRING);
    node *sq =
        FN("sq", TYPE_UNKNOWN, false, false,
           BLOCK(ES(BIN(OP_MULTIPLY, ID("x"), ID("x")))), 1, "x", TYPE_INT);
    node *noop =
        FN("noop", TYPE_UNKNOWN, false, false, BLOCK(PRINT(LS("noop"))), 0);
    node *show = FN("show", TYPE_UNIT, true, false, BLOCK(PRINT(ID("x"))), 1,
                    "x", TYPE_DOUBLE);
    node *ret_unit = FN("early", TYPE_UNIT, true, false,
                        BLOCK(PRINT(LS("e1")), RET(NULL)), 0);
    node *m = MAIN(BLOCK(
        VAL("ten", TYPE_UNKNOWN, LI(10)),
        VAL("f", TYPE_UNKNOWN, CALL("fib", 1, ID("ten"))), PRINT(ID("f")), NL,
        PRINT(CALL("half", 1, LI(3))), NL, PRINT(CALL("firstOver", 1, LI(50))),
        NL, VAL("r", TYPE_UNKNOWN, CALL("greet", 1, LS("Bob"))), PRINT(ID("r")),
        NL, PRINT(CALL("sq", 1, LI(9))), NL,
        VAL("nn", TYPE_UNKNOWN, CALL("noop", 0)), PRINT(ID("nn")), NL,
        ES(CALL("show", 1, LI(2))), NL, ES(CALL("early", 0)), NL));
    return PROG(9, fib, half, first, greet, sq, noop, show, ret_unit, m);
  }
  /* relational: relational operators ==, !=, <, <=, >= on numbers (Int vs
   * Double) and strings */
  if (!strcmp(t, "relational")) {
    return PROG(
        1, MAIN(BLOCK(
               VAL("a", TYPE_UNKNOWN, LI(3)), VAL("b", TYPE_UNKNOWN, LD(3.0)),
               IF(BIN(OP_EQUAL, ID("a"), ID("b")), BLOCK(PRINT(LS("eq "))),
                  BLOCK(PRINT(LS("ne ")))),
               IF(BIN(OP_LESSER_OR_EQUAL, ID("a"), LI(2)),
                  BLOCK(PRINT(LS("le2 "))), BLOCK(PRINT(LS("gt2 ")))),
               IF(BIN(OP_GREATER_OR_EQUAL, ID("b"), LI(3)),
                  BLOCK(PRINT(LS("ge3 "))), BLOCK(PRINT(LS("lt3 ")))),
               IF(BIN(OP_LESSER, LS("abc"), LS("abd")),
                  BLOCK(PRINT(LS("strlt "))), BLOCK(PRINT(LS("strge ")))),
               IF(BIN(OP_NOT_EQUAL, ID("a"), LI(4)), BLOCK(PRINT(LS("ne4 "))),
                  BLOCK(PRINT(LS("eq4 ")))),
               IF(BIN(OP_GREATER_OR_EQUAL, LS("b"), LS("a")),
                  BLOCK(PRINT(LS("sge"))), BLOCK(PRINT(LS("slt")))))));
  }
  /* coerce: implicit Int -> Double in assignment and initialization */
  if (!strcmp(t, "coerce")) {
    return PROG(1, MAIN(BLOCK(VAR("d", TYPE_UNKNOWN, LD(1.5)), ASG("d", LI(2)),
                              PRINT(ID("d")), NL, VAR("e", TYPE_DOUBLE, LI(1)),
                              ASG("e", LI(7)), PRINT(ID("e")), NL,
                              VAR("k", TYPE_UNKNOWN, LI(1)),
                              ASG("k", BIN(OP_ADD, ID("k"), LI(41))))));
  }
  /* io: readLine, readDouble, readInt (bad input -> 0), readLine at EOF -> ""
   */
  if (!strcmp(t, "io")) {
    return PROG(
        1,
        MAIN(BLOCK(
            VAL("l", TYPE_UNKNOWN, CALL("readLine", 0)), PRINT(ID("l")),
            VAL("x", TYPE_UNKNOWN, CALL("readDouble", 0)), PRINT(ID("x")), NL,
            VAL("bad", TYPE_UNKNOWN, CALL("readInt", 0)), PRINT(ID("bad")), NL,
            VAL("eof", TYPE_UNKNOWN, CALL("readLine", 0)),
            PRINT(BIN(OP_ADD, LS("["), BIN(OP_ADD, ID("eof"), LS("]")))))));
  }
  /* negzero: unary minus on Double, -0.0 */
  if (!strcmp(t, "negzero")) {
    return PROG(
        1, MAIN(BLOCK(VAL("m", TYPE_UNKNOWN, NEG(LD(2.5))), PRINT(ID("m")), NL,
                      VAL("z", TYPE_UNKNOWN, LD(0.0)), PRINT(NEG(ID("z"))))));
  }
  /* newsem: call as a statement and as the last statement, return 5 in a Unit
   * function, empty body, -5 as Double, <= on strings */
  if (!strcmp(t, "newsem")) {
    node *g = FN("g", TYPE_INT, true, false, BLOCK(ES(LI(7))), 0);
    node *h = FN("h", TYPE_DOUBLE, true, false, BLOCK(CALL("g", 0)),
                 0); /* posledný príkaz = node_call bez obalu */
    node *u = FN("u", TYPE_UNIT, true, false, BLOCK(RET(LI(5))),
                 0); /* return 5 vo funkcii typu Unit */
    node *empty =
        FN("empty", TYPE_UNIT, true, false, BLK(0), 0); /* prázdne telo */
    node *m =
        MAIN(BLOCK(PRINT(CALL("h", 0)), NL, PRINT(CALL("u", 0)), NL,
                   PRINT(CALL("empty", 0)), NL, CALL("print", 1, LS("direct")),
                   NL, /* volanie ako príkaz bez obalu */
                   VAL("d", TYPE_DOUBLE, NEG(LI(5))), PRINT(ID("d")), NL,
                   IF(BIN(OP_LESSER_OR_EQUAL, LS("a"), LS("a")),
                      BLOCK(PRINT(LS("le "))), BLOCK(PRINT(LS("gt ")))),
                   IF(BIN(OP_GREATER, LI(3), LD(2.5)), BLOCK(PRINT(LS("gt"))),
                      BLOCK(PRINT(LS("le"))))));
    return PROG(5, g, h, u, empty, m);
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <test name>\n", argv[0]);
    return 2;
  }
  node *program = build_test(argv[1]);
  if (program == NULL) {
    fprintf(stderr, "unknown test: %s\n", argv[1]);
    return 2;
  }

  /* the same order as in main.c: semantics fills data_type, var, func ... */
  sym_table *table = sym_init();
  if (table == NULL) {
    fprintf(stderr, "sym_init failed\n");
    return 99;
  }
  sem_analyze(program, table);
  code_generate(program);

  /* the symbol table must live until codegen_generate returns */
  sym_dispose(table);
  free_test_node(program);
  return 0;
}
