/*
 * implementation of a compiler for the language IFJ26
 * main.c
 * compiler entry, runs all sections lexer -> parser -> semantic analysis ->
 * code generation
 *
 * xbadint00 Tomáš Badinský
 *
 * reads from input and prints IFJcode26 to output with code generator, errors
 * are printed to stderr, returns 0 if successful, otherwise section that found
 * the error ends the program by itself with error code
 */

#include "codegen.h"
#include "common_functions.h"
#include "semantics.h"
#include "symtable.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>

// lexer functions
Token get_token(FILE *input);
void free_lexer_memory(token_array *arr);

// parser functions
node *parse_program(token_array *tokens);
void free_node(node *n);

/// @brief adds token to the end of the array, if allocation fails - prints error and exits
/// @param tokens array of tokens 
/// @param token token to be added
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

/// @brief lexical analysis of the input, reads all tokens until TOKEN_EOF stored at the end of token array so parser knows where program ends
/// @param tokens initialized and filled array 
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


/// @brief compiler entry, runs all sections lexer -> parser -> semantic analysis -> code generation, frees allocated memory 
/// @return 0 if translation was successful
int main(void) {
  // lexical analysis
  token_array tokens;
  read_tokens(&tokens);

  // syntax analysis, bulds AST
  node *program = parse_program(&tokens);

  // sematic analysis, checks AST and fills symtable
  sym_table *symbols = sym_init();
  if (symbols == NULL) {
    fprintf(stderr, "%s\n", error_to_message(ERR_INTERNAL));
    exit(ERR_INTERNAL);
  }
  sem_analyze(program, symbols);

  // code generation to output
  code_generate(program);

  // cleanup
  sym_dispose(symbols);
  free_node(program);

  // release buffer of the lexer, pos is set to the end to free all tokens
  tokens.pos = tokens.size;
  free_lexer_memory(&tokens);
  return 0;
}