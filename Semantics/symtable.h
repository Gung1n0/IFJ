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

#endif
