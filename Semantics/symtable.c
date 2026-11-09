#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "symtable.h"
#include "hashtable.h"
#include "builtins.h"

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

static char *strduplicate(const char *s)
{
    char *p = malloc(strlen(s) + 1);
    if (p != NULL)
        strcpy(p, s);
    return p;
}

static void free_function_entry(sym_function *f)
{
    if (f == NULL)
        return;

    if (f->parameters != NULL)
    {
        for (int j = 0; j < f->parameter_count; j++)
            free(f->parameters[j].name);
        free(f->parameters);
    }
    free(f->name);
    free(f);
}

static error_code register_built_in_function(sym_table *t, const char *name, const sym_parameter *params, int param_count, data_types return_value)
{
    sym_function *function = calloc(1, sizeof(sym_function));
    if (function == NULL)
        return ERR_INTERNAL;

    // TODO: rename
    function->name = strduplicate(name);
    if (function->name == NULL)
    {
        free_function_entry(function);
        return ERR_INTERNAL;
    }

    if (param_count > 0)
    {
        function->parameters = calloc(param_count, sizeof(sym_parameter));
        if (function->parameters == NULL)
        {
            free_function_entry(function);
            return ERR_INTERNAL;
        }

        for (int i = 0; i < param_count; i++)
        {
            function->parameters[i].data_type = params[i].data_type;
            function->parameters[i].name = strduplicate(params[i].name);
            if (function->parameters[i].name == NULL)
            {
                function->parameter_count = i;
                free_function_entry(function);
                return ERR_INTERNAL;
            }
        }
    }

    function->parameter_count = param_count;
    function->has_explicit_return_type = true;
    function->return_type = return_value;
    function->i_state = done;
    function->is_built_in = true;

    error_code code = sym_declare_function(t, function);
    if (code != ERR_NONE)
        free_function_entry(function);
    return code;
}

#define REGISTER_BUILTIN(P) \
    register_built_in_function(t, P##_NAME, P##_PARAMS, P##_PARAM_COUNT, P##_RETURN)

static bool register_all_built_in_functions(sym_table *t)
{
    return REGISTER_BUILTIN(PRINT) == ERR_NONE &&
           REGISTER_BUILTIN(READLINE) == ERR_NONE &&
           REGISTER_BUILTIN(READINT) == ERR_NONE &&
           REGISTER_BUILTIN(READDOUBLE) == ERR_NONE &&
           REGISTER_BUILTIN(TOSTR) == ERR_NONE &&
           REGISTER_BUILTIN(TOINT) == ERR_NONE &&
           REGISTER_BUILTIN(STRLENGTH) == ERR_NONE &&
           REGISTER_BUILTIN(SUBSTR) == ERR_NONE &&
           REGISTER_BUILTIN(ORD) == ERR_NONE &&
           REGISTER_BUILTIN(CHR) == ERR_NONE;
}

static sym_scope_data *sym_init_scope(void)
{
    sym_scope_data *scope = calloc(1, sizeof(sym_scope_data));

    if (scope == NULL)
        return NULL;

    scope->table = ht_init();

    if (scope->table == NULL)
    {
        free(scope);
        return NULL;
    }

    scope->parent = NULL;

    return scope;
}

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

    if (!register_all_built_in_functions(t))
    {
        sym_dispose(t);
        return NULL;
    }
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
            free_function_entry(t->function_entries[i]);
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
bool sym_enter_scope(sym_table *t)
{
    sym_scope_data *scope = sym_init_scope();
    if (scope == NULL)
        return false;

    scope->parent = t->innermost_scope;
    t->innermost_scope = scope;

    return true;
}

bool sym_exit_scope(sym_table *t)
{
    if (t->innermost_scope == NULL)
        return false;

    ht_dispose(t->innermost_scope->table);
    sym_scope_data *tmp = t->innermost_scope->parent;
    free(t->innermost_scope);

    t->innermost_scope = tmp;

    return true;
}

error_code sym_declare_variable(sym_table *t, sym_variable *var)
{
    if (t->innermost_scope == NULL)
        return ERR_INTERNAL;

    bool exists = ht_search(t->innermost_scope->table, var->name);

    if (exists)
        return ERR_REDEFINITION;
    else
    {
        if (t->variable_count >= MAX_VARIABLE_CAPACITY - 1)
            return ERR_INTERNAL;

        bool success = ht_insert(t->innermost_scope->table, var->name, var);
        if (success)
        {
            var->id = t->variable_id_counter;

            t->variable_entries[t->variable_count] = var;
            t->variable_count++;
            t->variable_id_counter++;

            return ERR_NONE;
        }
        return ERR_INTERNAL;
    }
}

sym_variable *sym_lookup_variable(sym_table *t, const char *name)
{
    if (t->innermost_scope == NULL)
        return NULL;

    sym_variable *data = NULL;
    sym_scope_data *ptr = t->innermost_scope;

    while (ptr != NULL)
    {
        data = ht_get_data(ptr->table, name);
        if (data != NULL)
            return data;
        ptr = ptr->parent;
    }
    // return error 3
    return NULL;
}

error_code sym_declare_function(sym_table *t, sym_function *func)
{
    bool exists = ht_search(t->function_table, func->name);

    if (exists)
        return ERR_REDEFINITION;
    else
    {
        if (t->function_count >= MAX_FUNCTION_CAPACITY - 1)
            return ERR_INTERNAL;

        bool success = ht_insert(t->function_table, func->name, func);
        if (success)
        {
            t->function_entries[t->function_count] = func;
            t->function_count++;
            return ERR_NONE;
        }
        return ERR_INTERNAL;
    }
}

sym_function *sym_lookup_function(sym_table *t, const char *name)
{
    sym_function *data = ht_get_data(t->function_table, name);
    if (data == NULL)
    {
        // return error 3
        return NULL;
    }
    return data;
}