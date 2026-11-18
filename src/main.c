#include "codegen.h"
#include "semantics.h"
#include "symtable.h"
#include "token.h"


int main(void) {
  // lexical analysis
  token_array tokens;
  read_tokens(&tokens);

  // syntax analysis
  node *program = parse_program(&tokens);
  free_tokens(&tokens);

  // sematic analysis
  sym_table *symbols = sym_init();
  sem_analyze(program, symbols);

  // code generation
  code_generate(program);

  // cleanup
  sym_dispose(symbols);
  free_node(program);
  return 0;
}