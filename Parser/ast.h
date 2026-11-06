#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Constants.h"
#include "token.h"

typedef struct
{
    char *name;
    data_types type;
    int row;
    int column;
} AST;

typedef struct
{
    AST *data;
    int size;
} AST_array;

typedef struct
{
    char *name;
    data_types types;
    int row;
    int column;
} param;

typedef struct node node;

struct node
{
    node_type type;
    int row;
    int column;
    union
    {
        struct
        {
            node **items;
            int size;
            int cap;
        } program;
        struct
        {
            char *name;
            data_types return_type;
            bool explicit;
            bool is_main;
            param *params;
            int param_count;
            node *body;
        } function;
        struct
        {
            node **items;
            int size;
            int cap;
        } block;
        struct
        {
            char *name;
            bool changable;
            data_types type;
            node *init;
        } val_var;
        struct
        {
            char *name;
            node *init;
        } asign;
        struct
        {
            node *expression;
        } expression;
        struct
        {
            node *value;
        } ret;
        struct
        {
            node *condition;
            node *then;
            node *else_n;
        } if_n;
        struct
        {
            node *condition;
            node *body;
        } while_n;
        struct
        {
            data_types type;
            int i;
            double d;
            char *s;
        } lit;
        struct
        {
            char *name;
        } ident;
        struct
        {
            operator op;
            node *operand;
        } unar;
        struct
        {
            operator op;
            node *left;
            node *right;
        } binar;
        struct
        {
            char *name;
            node **items;
            int size;
            int cap;
        } call;
    } idk;
};
// prints error message and exists with error code
void print_error(const char *message, int error_code, int row, int column)
{
    fprintf(stderr, "%s at row %i column %i\n", message, row, column);
    exit(error_code);
}

// initialize AST array
AST_array construct_AST_array(int size)
{
    AST_array t;
    t.data = (AST *)malloc(size * sizeof(AST));
    if (t.data != NULL)
    {
        t.size = size;
    }
    else
    {
        t.size = 0;
    }
    return t;
}

// destroy AST array
void destruct_AST_array(AST_array *t)
{
    for (int i = 0; i < t->size; i++)
        free(t->data[i].name);

    free(t->data);
    t->data = NULL;
}

// initialize AST array
void init_AST_array(AST_array *arr)
{
    arr->size = 0;
    arr->data = NULL;
}

// enlarge array of AST by one
void enlarge_AST_array_one(AST_array *arr)
{
    if (arr == NULL)
        return;

    AST *tmp = realloc(arr->data, (arr->size + 1) * sizeof(AST));
    if (tmp == NULL)
        print_error("realloc failed", INTERNAL_ERROR, 0, 0);
    arr->data = tmp;
    arr->size++;
    return;
}

#endif