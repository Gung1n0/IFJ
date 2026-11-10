#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "../Common/enums.h"
#include <stdbool.h>

typedef struct sym_table sym_table;
typedef struct sym_scope_data sym_scope_data;

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
    struct node *definition;
} sym_function;

sym_table *sym_init(void);

void sym_dispose(sym_table *t);

bool sym_enter_scope(sym_table *t);

bool sym_exit_scope(sym_table *t);

error_code sym_declare_variable(sym_table *t, sym_variable *var);

error_code sym_declare_function(sym_table *t, sym_function *func, struct node *def);

sym_variable *sym_lookup_variable(sym_table *t, const char *name);

sym_function *sym_lookup_function(sym_table *t, const char *name);

sym_scope_data *sym_detach(sym_table *t);

bool sym_restore(sym_table *t, sym_scope_data *data);

#endif
