/*
 * implementation of a compiler for the language IFJ26
 * symtable.c
 * implementation of the sym_table adt (built on ht_table)
 * xzombot00 Tomáš Zomborský
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Common/common_functions.h"
#include "builtins.h"
#include "hashtable.h"
#include "symtable.h"

#define INITIAL_FUNCTION_ARRAY_CAPACITY 8
#define FUNCTION_CAPACITY_MULTIPLIER 2

#define INITIAL_VARIABLE_ARRAY_CAPACITY 8
#define VARIABLE_CAPACITY_MULTIPLIER 2

struct sym_scope_data {
  ht_table *table;
  sym_scope_data *parent;
};

struct sym_table {
  ht_table *function_table;
  sym_scope_data *innermost_scope;

  sym_variable **variable_entries;
  sym_function **function_entries;

  int variable_size;
  int variable_capacity;

  int function_size;
  int function_capacity;

  int variable_id_counter;
};

/// @brief Attempts to append a sym_function pointer to the end of a dynamic
/// array
/// @param t Symtable, that owns the function array
/// @param f Function to be appended
/// @return Returns true if function has been successfully appended, else false
static bool append_to_function_array(sym_table *t, sym_function *f) {
  int index = t->function_size;
  if (index >= t->function_capacity) {
    t->function_capacity *= FUNCTION_CAPACITY_MULTIPLIER;
    t->function_entries = realloc(
        t->function_entries, t->function_capacity * sizeof(sym_function *));
    if (t->function_entries == NULL)
      return false;
  }

  t->function_entries[index] = f;
  t->function_size++;

  return true;
}

/// @brief Attempts to append a sym_variable pointer to the end of a dynamic
/// array
/// @param t Symtable, that owns the variable array
/// @param v Variable to be appended
/// @return Returns true if variable has been successfully appended, else false
static bool append_to_variable_array(sym_table *t, sym_variable *v) {
  int index = t->variable_size;
  if (index >= t->variable_capacity) {
    t->variable_capacity *= VARIABLE_CAPACITY_MULTIPLIER;
    t->variable_entries = realloc(
        t->variable_entries, t->variable_capacity * sizeof(sym_variable *));
    if (t->variable_entries == NULL)
      return false;
  }

  t->variable_entries[index] = v;
  t->variable_size++;
  t->variable_id_counter++;

  return true;
}

/// @brief Frees data of a function entry
/// @param f Function to be freed
static void free_function_entry(sym_function *f) {
  if (f == NULL)
    return;

  if (f->parameters != NULL) {
    for (int j = 0; j < f->parameter_count; j++)
      free(f->parameters[j].name);
    free(f->parameters);
  }
  free(f->locals);
  free(f->name);
  free(f);
}

/// @brief Takes in data about a built in function, and registers it
/// @param t Table, into which the function should be registered
/// @param name Name of the function
/// @param params Parameter array of the function
/// @param param_count Number of parameters of the function
/// @param return_value Return value of the function
/// @return Returns error code, signaling success
static error_code register_built_in_function(sym_table *t, const char *name,
                                             const sym_parameter *params,
                                             int param_count,
                                             data_types return_value) {
  sym_function *function = calloc(1, sizeof(sym_function));
  if (function == NULL)
    return ERR_INTERNAL;

  function->name = strduplicate(name);
  if (function->name == NULL) {
    free_function_entry(function);
    return ERR_INTERNAL;
  }

  if (param_count > 0) {
    function->parameters = calloc(param_count, sizeof(sym_parameter));
    if (function->parameters == NULL) {
      free_function_entry(function);
      return ERR_INTERNAL;
    }

    for (int i = 0; i < param_count; i++) {
      function->parameters[i].data_type = params[i].data_type;
      function->parameters[i].name = strduplicate(params[i].name);
      if (function->parameters[i].name == NULL) {
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

  error_code code = sym_declare_function(t, function, NULL);
  if (code != ERR_NONE)
    free_function_entry(function);
  return code;
}

#define REGISTER_BUILTIN(P)                                                    \
  register_built_in_function(t, P##_NAME, P##_PARAMS, P##_PARAM_COUNT,         \
                             P##_RETURN)
/// @brief Function, that takes in data from bulitins.h and runs
/// register_built_in_function for each function
/// @param t Table, into which all functions should be registered
/// @return Returns true all functions have been successfully registered
static bool register_all_built_in_functions(sym_table *t) {
  return REGISTER_BUILTIN(PRINT) == ERR_NONE &&
         REGISTER_BUILTIN(READLINE) == ERR_NONE &&
         REGISTER_BUILTIN(READINT) == ERR_NONE &&
         REGISTER_BUILTIN(READDOUBLE) == ERR_NONE &&
         REGISTER_BUILTIN(TOSTR) == ERR_NONE &&
         REGISTER_BUILTIN(TOINT) == ERR_NONE &&
         REGISTER_BUILTIN(STRLENGTH) == ERR_NONE &&
         REGISTER_BUILTIN(SUBSTR) == ERR_NONE &&
         REGISTER_BUILTIN(ORD) == ERR_NONE && REGISTER_BUILTIN(CHR) == ERR_NONE;
}

/// @brief Initializes sym_scope_data
/// @param (void) Nothing
/// @return Returns pointer if successful, otherwise returns NULL
static sym_scope_data *sym_init_scope(void) {
  sym_scope_data *scope = calloc(1, sizeof(sym_scope_data));

  if (scope == NULL)
    return NULL;

  scope->table = ht_init();

  if (scope->table == NULL) {
    free(scope);
    return NULL;
  }

  scope->parent = NULL;

  return scope;
}

/// @brief Attemps to initialize sym_table with default values, and calls
/// register_all_built_in_functions, if something fails, free all allocated
/// pointers
sym_table *sym_init(void) {
  sym_table *t = calloc(1, sizeof(sym_table));

  if (t == NULL)
    return NULL;

  t->function_table = ht_init();

  if (t->function_table == NULL) {
    sym_dispose(t);
    return NULL;
  }

  t->innermost_scope = NULL;

  t->function_entries =
      calloc(INITIAL_FUNCTION_ARRAY_CAPACITY, sizeof(sym_function *));
  t->function_capacity = INITIAL_FUNCTION_ARRAY_CAPACITY;

  if (t->function_entries == NULL) {
    sym_dispose(t);
    return NULL;
  }

  t->variable_entries =
      calloc(INITIAL_VARIABLE_ARRAY_CAPACITY, sizeof(sym_variable *));
  t->variable_capacity = INITIAL_VARIABLE_ARRAY_CAPACITY;

  if (t->variable_entries == NULL) {
    sym_dispose(t);
    return NULL;
  }

  t->variable_size = 0;
  t->function_size = 0;

  t->variable_id_counter = 0;

  if (!register_all_built_in_functions(t)) {
    sym_dispose(t);
    return NULL;
  }
  return t;
}

/// @brief Frees all pointers allocated by the sym_table
void sym_dispose(sym_table *t) {
  if (t == NULL)
    return;

  if (t->function_table != NULL)
    ht_dispose(t->function_table);

  if (t->innermost_scope != NULL) {
    sym_scope_data *next = t->innermost_scope;
    sym_scope_data *tmp;

    while (next != NULL) {
      ht_dispose(next->table);
      tmp = next;
      next = next->parent;
      free(tmp);
    }
  }

  if (t->function_entries != NULL) {
    for (int i = 0; i < t->function_size; i++)
      free_function_entry(t->function_entries[i]);
    free(t->function_entries);
  }

  if (t->variable_entries != NULL) {
    for (int i = 0; i < t->variable_size; i++) {
      free(t->variable_entries[i]->name);
      free(t->variable_entries[i]);
    }
    free(t->variable_entries);
  }

  free(t);
}

/// @brief Creates a new scope, and sets it as the "root" of the linked list
bool sym_enter_scope(sym_table *t) {
  sym_scope_data *scope = sym_init_scope();
  if (scope == NULL)
    return false;

  scope->parent = t->innermost_scope;
  t->innermost_scope = scope;

  return true;
}

/// @brief Frees the innermost scope, "root" of the linked list, and sets it's
/// parent to the innermost ("root")
bool sym_exit_scope(sym_table *t) {
  if (t->innermost_scope == NULL)
    return false;

  ht_dispose(t->innermost_scope->table);
  sym_scope_data *tmp = t->innermost_scope->parent;
  free(t->innermost_scope);

  t->innermost_scope = tmp;

  return true;
}

/// @brief Attempts to declare a new variable in the innermost scopes ht_table
error_code sym_declare_variable(sym_table *t, sym_variable *var) {
  if (t->innermost_scope == NULL)
    return ERR_INTERNAL;

  bool exists = ht_search(t->innermost_scope->table, var->name);

  if (exists)
    return ERR_REDEFINITION;
  else {
    bool success = ht_insert(t->innermost_scope->table, var->name, var);
    if (success) {
      var->id = t->variable_id_counter;

      if (!append_to_variable_array(t, var))
        return ERR_INTERNAL;
      return ERR_NONE;
    }
    return ERR_INTERNAL;
  }
}

/// @brief Attemps to look up a variable given it's name by traversing and
/// checking all the ht_tables of all scopes
sym_variable *sym_lookup_variable(sym_table *t, const char *name) {
  if (t->innermost_scope == NULL)
    return NULL;

  sym_variable *data = NULL;
  sym_scope_data *ptr = t->innermost_scope;

  while (ptr != NULL) {
    data = ht_get_data(ptr->table, name);
    if (data != NULL)
      return data;
    ptr = ptr->parent;
  }
  return NULL;
}
/// @brief Attempts to declare a new function in the function table
error_code sym_declare_function(sym_table *t, sym_function *func,
                                struct node *def) {
  bool exists = ht_search(t->function_table, func->name);

  if (exists)
    return ERR_REDEFINITION;
  else {
    func->definition = def;

    bool success = ht_insert(t->function_table, func->name, func);
    if (success) {
      if (!append_to_function_array(t, func))
        return ERR_INTERNAL;
      return ERR_NONE;
    }
    return ERR_INTERNAL;
  }
}

/// @brief Attemps to look up a function given it's name
sym_function *sym_lookup_function(sym_table *t, const char *name) {
  sym_function *data = ht_get_data(t->function_table, name);
  if (data == NULL) {
    return NULL;
  }
  return data;
}

/// @brief Detaches all the scopes from the sym_table
sym_scope_data *sym_detach(sym_table *t) {
  sym_scope_data *tmp_inner_scope = t->innermost_scope;
  t->innermost_scope = NULL;
  return tmp_inner_scope;
}
/// @brief Reattaches all the scopes back to the sym_table
bool sym_restore(sym_table *t, sym_scope_data *data) {
  if (t->innermost_scope != NULL)
    return false;

  t->innermost_scope = data;
  return true;
}