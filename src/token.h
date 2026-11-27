/*
 * implementation of a compiler for the language IFJ26
 * token.h
 * token and token array data structure definitions
 *
 * xsturaf00 Filip Šturák
 *
 * basic thinking behind token array: stores tokens produced by the lexer and
 * supports lookahead access without mutating the current stream position
 *
 */

#ifndef TOKEN_H
#define TOKEN_H

#include "enums.h"
typedef struct {
  TokenType type;
  char *data;
  int row;
  int column;
} Token;

typedef struct {
  Token *token_data;
  int size;
  int pos;
} token_array;

#endif