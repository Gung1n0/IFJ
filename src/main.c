#include "codegen.h"
#include "common_functions.h"
#include "semantics.h"
#include "symtable.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>

// pseudo lexer.h
Token get_token(FILE *input);
void free_lexer_memory(token_array *arr);

// pseudo parser.h
node *parse_program(token_array *tokens);
void free_node(node *n);

// add a token to the end of array
static void add_token(token_array *tokens, Token token) {
  Token *new_bigger_arr =
      realloc(tokens->token_data, (tokens->size + 1) * sizeof(Token));

  if (new_bigger_arr == NULL) {
    fprintf(stderr, "%s\n", error_to_message(ERR_INTERNAL));
    exit(ERR_INTERNAL);
  }
  tokens->token_data = new_bigger_arr;
  tokens->token_data[tokens->size] = token;
  tokens->size++;
}

// read all tokens from stdin and store them in a token_array
static void read_tokens(token_array *tokens) {
  tokens->token_data = NULL;
  tokens->size = 0;
  tokens->pos = 0;

  Token current_token = get_token(stdin);
  while (current_token.type != TOKEN_EOF) {
    add_token(tokens, current_token);
    current_token = get_token(stdin);
  }
  add_token(tokens, current_token);
}

int main(void) {
  // lexical analysis
  token_array tokens;
  read_tokens(&tokens);

  // syntax analysis
  node *program = parse_program(&tokens);

  // sematic analysis
  sym_table *symbols = sym_init();
  if (symbols == NULL) {
    fprintf(stderr, "%s\n", error_to_message(ERR_INTERNAL));
    exit(ERR_INTERNAL);
  }
  sem_analyze(program, symbols);

  // code generation
  code_generate(program);

  // cleanup
  sym_dispose(symbols);
  free_node(program);

  // release of token buffer memory
  tokens.pos = tokens.size;
  free_lexer_memory(&tokens);
  return 0;
}