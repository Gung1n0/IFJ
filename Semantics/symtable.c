#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "symtable.h"
#include "hashtable.h"

// TODO: rework arrays to be either linked lists or a dynamic arrays
//       for unlimited variables and functions
#define MAX_VARIABLE_CAPACITY 200
#define MAX_FUNCTION_CAPACITY 100

typedef struct sym_scope_data sym_scope_data;

struct sym_scope_data
{
    ht_table *table;
    sym_scope_data *parent;
};

struct sym_table
{
    ht_table *function_table;
    sym_scope_data *innermost_scope;

    sym_variable **variable_entries;
    sym_function **function_entries;

    int variable_count;
    int function_count;

    int variable_id_counter;
};

// sym_scope_data *sym_init_scope()
// {
//     sym_scope_data *scope = calloc(1, sizeof(sym_scope_data));

//     if (scope == NULL)
//         return NULL;

//     scope->table = ht_init();

//     if (scope->table == NULL)
//     {
//         free(scope);
//         return NULL;
//     }

//     scope->parent = NULL;

//     return scope;
// }

sym_table *sym_init(void)
{
    sym_table *t = calloc(1, sizeof(sym_table));

    if (t == NULL)
        return NULL;

    t->function_table = ht_init();

    if (t->function_table == NULL)
    {
        sym_dispose(t);
        return NULL;
    }

    t->innermost_scope = NULL;

    t->function_entries = calloc(MAX_FUNCTION_CAPACITY, sizeof(sym_function *));

    if (t->function_entries == NULL)
    {
        sym_dispose(t);
        return NULL;
    }

    t->variable_entries = calloc(MAX_VARIABLE_CAPACITY, sizeof(sym_variable *));

    if (t->variable_entries == NULL)
    {
        sym_dispose(t);
        return NULL;
    }

    t->variable_count = 0;
    t->function_count = 0;

    t->variable_id_counter = 0;
    return t;
}

void sym_dispose(sym_table *t)
{
    if (t == NULL)
        return;

    if (t->function_table != NULL)
        ht_dispose(t->function_table);

    if (t->innermost_scope != NULL)
    {
        sym_scope_data *next = t->innermost_scope;
        sym_scope_data *tmp;

        while (next != NULL)
        {
            ht_dispose(next->table);
            tmp = next;
            next = next->parent;
            free(tmp);
        }
    }

    if (t->function_entries != NULL)
    {
        for (int i = 0; i < t->function_count; i++)
        {
            free(t->function_entries[i]->name);

            for (int j = 0; j < t->function_entries[i]->parameter_count; j++)
                free(t->function_entries[i]->parameters[j].name);

            free(t->function_entries[i]->parameters);
            free(t->function_entries[i]);
        }
        free(t->function_entries);
    }

    if (t->variable_entries != NULL)
    {
        for (int i = 0; i < t->variable_count; i++)
        {
            free(t->variable_entries[i]->name);
            free(t->variable_entries[i]);
        }
        free(t->variable_entries);
    }

    free(t);
}
