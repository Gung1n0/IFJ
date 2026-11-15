#ifndef TOKEN_H
#define TOKEN_H

#include "Enums.h"
typedef struct
{
    TokenType type;
    char *data;
    int row;
    int column;
} Token;

typedef struct
{
    Token *token_data;
    int size;
    int pos;
} token_array;

#endif