#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "enums.h"
#include <stdbool.h>

typedef struct sym_table sym_table;

typedef struct
{
    int id;
    char *name; // owns it's copy of name
    data_types data_type;

    bool is_mutable;
    bool is_parameter;
} sym_variable;

typedef struct
{
    char *name;
    data_types data_type;
} sym_parameter;

typedef struct
{
    char *name; // owns it's copy of name

    int parameter_count;
    sym_parameter *parameters; // owns it's copy of parameters

    data_types return_type;

    bool has_explicit_return_type;
    bool is_built_in;
    bool is_main;

    inference_state i_state;
} sym_function;

sym_table *sym_init(void);

void sym_dispose(sym_table *t);

void sym_enter_scope(sym_table *t);

bool sym_exit_scope(sym_table *t);

bool sym_declare_variable(sym_table *t, sym_variable *var);

bool sym_declare_function(sym_table *t, sym_function *func);

sym_variable *sym_lookup_variable(sym_table *t, const char *name);

sym_function *sym_lookup_function(sym_table *t, const char *name);

#endif
