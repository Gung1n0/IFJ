#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Enums.h"
#include "Constants.h"

typedef struct 
{
    char *name;
    data_types type;
    int row;
    int column;
}AST;

typedef struct
{
    AST *data;
    int size;
} AST_array;

typedef struct
{
    char *name;
    int row;
    int column;
    AST_array *ast_arr;
    data_types return_type;
    bool explicit;
    bool is_main;
    char* body;
}node;

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

    bool failed = realloc(arr->data, (arr->size + 1)*sizeof(AST));
    if (!failed)
        print_error("realloc failed", INTERNAL_ERROR, 0,0);
    
    arr->size += 1;
    return;
}



#endif