#ifndef TOKEN_H
#define TOKEN_H

#include "Enums.h"
typedef struct
{
    token_type type;
    char *data;
    int row;
    int column;
} token;

typedef struct
{
    token *token_data;
    int size;
    int pos;
} token_array;

#endif