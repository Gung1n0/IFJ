#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Enums.h"

typedef struct
{
    token_type type;
    char *data;
} token;

typedef struct
{
    token *token_data;
    int array_lenght;
} TokenArray;

int parse()
{
    return 0;
}

void return_error(char* expected, error_messages_types error)
{
    switch (error)
    {
    case missing_char:
        fprintf(stderr, "Expected {%s}\n", expected);
        break;
    case unexpected:
        fprintf(stderr, "Unexpected {%s}\n",expected);
        break;
    case invalid_operand:
        fprintf(stderr, "Invalid operand {%s}\n", expected);
        break;
    case missmatched_parenthasis:
        fprintf(stderr, "Parenthasis missmathed");
        break;
    default:
        break;
    }
}

int main(int argc, char const *argv[])
{
    return 0;
}
