#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Enums.h"
#include "Constants.h"

typedef struct
{
    token_type type;
    char *data;
} token;

typedef struct
{
    token *token_data;
    int size;
    int pos;
    ;
} token_array;

// temporary solution for testing without lexer
token get_token(token_type type, char *data)
{
    token Token;
    Token.type = type;
    Token.data = data;
    return Token;
}
//, posun, kontrola a spotrebovanie očakávaného tokenu.

// initialize token array
token_array construct_token_array(int size)
{
    token_array t;
    t.token_data = malloc(size * sizeof(token));
    if (t.token_data != NULL)
    {
        t.size = size;
    }
    else
    {
        t.size = 0;
    }
    return t;
}

// destroy token array
void destruct_token_array(token_array *t)
{
    free(t->token_data);
    t->token_data = NULL;
}

// initialize token array
void init_token_array(token_array *arr)
{
    arr->size = 0;
    arr->token_data = NULL;
}

// destroy token
void destruct_token(token *t)
{
    free(t->data);
    t->data = NULL;
}

// checks and gets current token from array
token *get_token_from_array(token_array *arr, token_type type, char *data)
{
    token *curr = &arr->token_data[arr->pos];

    bool data_matches = true;
    if (data != NULL)
    {
        if (curr->data == NULL || strcmp(curr->data, data) != 0)
        {
            data_matches = false;
        }
    }

    if (curr->type == type && data_matches)
    {
        skip_token(arr);
        return curr;
    }

    print_error("syntax error", SYNTAX_ERROR);
}

// peek at first token
token *peek_token_array(token_array *arr, int offset)
{
    if (arr->size > 0 && arr->pos + offset < arr->size)
    {
        return &arr->token_data[offset + arr->pos];
    }
    print_error("syntax error", SYNTAX_ERROR);
}

// skip token
void skip_token(token_array *arr)
{
    if (arr != NULL && arr->pos < arr->size - 1)
        arr->pos++;
}
//prints error message and exists with error code
void print_error(const char *message, int error_code)
{
    fprintf(stderr, "%s\n", message);
    exit(error_code);
}

int main(int argc, char const *argv[])
{
    return 0;
}
