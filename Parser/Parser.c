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
} token_array;

// prints error message and exists with error code
void print_error(const char *message, int error_code)
{
    fprintf(stderr, "%s\n", message);
    exit(error_code);
}

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

// enlarge array of tokens by one
void enlarge_token_array_one(token_array *arr)
{
    if (arr == NULL)
        return;

    bool failed = realloc(arr, arr->size + 1);
    if (failed)
        print_error("realloc failed", INTERNAL_ERROR);
    return;
}

// destroy token
void destruct_token(token *t)
{
    free(t->data);
    t->data = NULL;
}

// go to next token
void skip_token(token_array *arr)
{
    if (arr != NULL && arr->pos < arr->size - 1)
        arr->pos++;
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
    return NULL; // Not needed but gcc throws warning without this
}

// peek at first token
token *peek_token_array(token_array *arr, int offset)
{
    if (arr->size > 0 && arr->pos + offset < arr->size)
    {
        return &arr->token_data[offset + arr->pos];
    }
    print_error("syntax error", SYNTAX_ERROR);
    return NULL; // Not needed but gcc throws warning without this
}

// works only when all files end with EOF, needs to be checked with ado
void parse_prologue(token_array *arr)
{
    if (arr->size > 0)
    {
        token *curr = peek_token_array(arr, 0);
        // go trough tokens until i get prologue, or syntax error

        while (curr->type != token_keyword)
        {
            if (curr->type == token_newline)
            {
                skip_token(arr);
                curr = &arr->token_data[arr->pos];
            }
            else
                print_error("expected prologue", SYNTAX_ERROR);
            if (arr->pos == arr->size - 1)
                print_error("expected prologue", SYNTAX_ERROR);
        }

        if (curr->data == NULL || strcmp(curr->data, "import") != 0)
            print_error("expected 'import'", SYNTAX_ERROR);

        skip_token(arr);

        get_token_from_array(arr, token_identifier, "ifj26");

        get_token_from_array(arr, token_specialchar, ".");

        get_token_from_array(arr, token_specialchar, "*");

        get_token_from_array(arr, token_newline, NULL);
        return;
    }
    print_error("expected prologue", SYNTAX_ERROR);
}

void parse_function_body(token_array *arr, token *curr)
{
    curr = peek_token_array(arr, 0); // temp just for gcc to not throw warning
    curr->type = token_bracket; //temp just for gcc to not throw warning
}

void parse_params(token_array *arr, token *curr)
{
    while (curr->type != token_bracket)
    {
        if (curr->data != NULL)
        {
            if (curr->type == token_keyword &&
                (strcmp(curr->data, "Int") == 0 || strcmp(curr->data, "Unit") == 0 || strcmp(curr->data, "Double") == 0 || strcmp(curr->data, "String") == 0))
            {
                skip_token(arr);
                curr = peek_token_array(arr, 0);
                if (curr->data == NULL)
                    print_error("Syntax error, expected ':'", SYNTAX_ERROR);
                if (curr->type != token_specialchar || strcmp(curr->data, ":") != 0)
                    print_error("Syntax error, expected ':'", SYNTAX_ERROR);

                skip_token(arr);
                curr = peek_token_array(arr, 0);

                if (curr->type != token_identifier || curr->data == NULL)
                    print_error("Syntax error, expected identifier", SYNTAX_ERROR);

                skip_token(arr);
                curr = peek_token_array(arr, 0);
            }
            else
                print_error("Syntax error, expected keyword", SYNTAX_ERROR);
        }
        else
            print_error("Syntax error, expected identifier", SYNTAX_ERROR);
    }
}

// parses head of a function: (n: Int):Int =
void parse_function_params(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);
    if (curr->type != token_identifier || curr->data == NULL)
        print_error("Syntax error, expected identifier", SYNTAX_ERROR);
    skip_token(arr);
    curr = &arr->token_data[arr->pos];

    get_token_from_array(arr, token_bracket, "(");

    curr = peek_token_array(arr, 0);
    if (curr->type == token_newline)
    {
        get_token_from_array(arr, token_bracket, ")");
    }
    else
    {
        parse_params(arr, curr);
        get_token_from_array(arr, token_bracket, ")");
    }

    get_token_from_array(arr, token_specialchar, ":");

    curr = peek_token_array(arr, 0);

    if (curr->data == NULL)
        print_error("Syntax error, expected identifier", SYNTAX_ERROR);

    if (!(curr->type == token_keyword &&
          (strcmp(curr->data, "Int") == 0 || strcmp(curr->data, "Unit") == 0 || strcmp(curr->data, "Double") == 0 || strcmp(curr->data, "String") == 0)))
        print_error("Syntax error, expected identifier", SYNTAX_ERROR);

    get_token_from_array(arr, token_relation_operator, "=");

    curr = peek_token_array(arr, 0);
    if (curr->type == token_newline && curr->data == NULL)
    {
        skip_token(arr);
        curr = peek_token_array(arr, 0);
    }

    parse_function_body(arr, curr);
    return;
}

// parses def function
void parse_def(token_array *arr, token *curr)
{
    if (curr->data == NULL)
        print_error("Syntax error", SYNTAX_ERROR);

    if (strcmp(curr->data, "def") == 0)
    {
        skip_token(arr);
        parse_function_params(arr);
    }
    else
        print_error("Syntax error", SYNTAX_ERROR);
}

// parses @main def function
void parse_main(token_array *arr, token *curr)
{
    if (strcmp(curr->data, "@") == 0)
    {
        skip_token(arr);
        curr = &arr->token_data[arr->pos];
        if (curr->type == token_identifier && curr->data != NULL)
        {
            if (strcmp(curr->data, "main") == 0)
            {
                skip_token(arr);

                curr = peek_token_array(arr, 0);
                if (curr->type == token_newline && curr->data == NULL)
                {
                    skip_token(arr);
                    curr = peek_token_array(arr, 0);
                }

                if (curr->type == token_keyword && curr->data != NULL)
                    parse_def(arr, curr);
                else
                    print_error("Syntax error", SYNTAX_ERROR);
            }
            else
                print_error("Syntax error, expected 'main'", SYNTAX_ERROR);
        }
        else
            print_error("Syntax error, expected 'main'", SYNTAX_ERROR);
    }
    else
        print_error("Syntax error, expected '@'", SYNTAX_ERROR);
}

//
void parse_head_funtion(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);
    while (curr->type != token_eof)
    {
        if (curr->type == token_keyword && curr->data != NULL)
        {
            parse_def(arr, curr);
        }
        else if (curr->type == token_specialchar && curr->data != NULL)
        {
            parse_main(arr, curr);
        }
        else
            print_error("Syntax error", SYNTAX_ERROR);

        curr = &arr->token_data[arr->pos];
    }
    return;
}

int main(int argc, char const *argv[])
{
    printf("%i %s dočasne aby gcc nehadzal warrning \n", argc, argv[0]);
    // parse_prologue(arr);
    // parse_function(arr);
    return 0;
}
