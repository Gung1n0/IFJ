#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Constants.h"
#include "token.h"
#include "Enums.h"

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
            node *ret; // maybe not needed
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
    } as;
};

// prints error message and exists with error code
void print_error(const char *message, int error_code, int row, int column)
{
    fprintf(stderr, "%s at row %i column %i\n", message, row, column);
    exit(error_code);
}

typedef struct
{
    item_type type;
    union{
        node *n;
        operator op;
        bracket_type b;
    } ast;
} stack_item;

typedef struct
{
    stack_item **items;
    int size;
    int cap;
} stack;

typedef struct{
     actions table[15][15];
}pre_talbe;

#endif