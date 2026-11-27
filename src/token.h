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