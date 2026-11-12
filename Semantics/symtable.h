/*
 * implementation of a compiler for the language IFJ26
 * symtable.h
 * xzombot00 Tomáš Zomborský
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "../Common/enums.h"
#include <stdbool.h>

typedef struct sym_table sym_table;
typedef struct sym_scope_data sym_scope_data;

struct node;

typedef struct sym_variable {
  int id;
  char *name; // owns it's copy of name
  data_types data_type;

  bool is_mutable;
  bool is_parameter;
} sym_variable;

typedef struct {
  char *name;
  data_types data_type;
} sym_parameter;

typedef struct sym_function {
  char *name; // owns it's copy of name

  int parameter_count;
  sym_parameter *parameters; // owns it's copy of parameters

  data_types return_type;

  bool has_explicit_return_type;
  bool is_built_in;
  bool is_main;

  inference_state i_state;
  struct node *definition;

  sym_variable **locals;
  int locals_count;
} sym_function;

/// @brief Attemps to initialize sym_table with default values, and calls
/// register_all_built_in_functions, if something fails, free all allocated
/// pointers
/// @param (void) Nothing
/// @return Returns sym_table pointer, if something along the way failed, reutrn
/// NULL
sym_table *sym_init(void);

/// @brief Frees all pointers allocated by the sym_table
/// @param t Sym_table to free
void sym_dispose(sym_table *t);

/// @brief Creates a new scope, and sets it as the "root" of the linked list
/// @param t Sym_table, where a new scope should be entered
/// @return Returns true if a new scope has been successfully entered, else
/// false
bool sym_enter_scope(sym_table *t);

/// @brief Frees the innermost scope, "root" of the linked list, and sets it's
/// parent to the innermost ("root")
/// @param t Sym_table, where a scope should be exited
/// @return Returns true if scope has been successfully exited, false if it
/// wasn't (there was no scope to exit)
bool sym_exit_scope(sym_table *t);

/// @brief Attempts to declare a new variable in the innermost scopes ht_table
/// @param t Sym_table, where a new variable should be delcared
/// @param var Variable to be declared
/// @return Returns error code indicating whether the variable was added or not
/// (ERR_NONE if it was added, ERR_REDEFINITION if a variable of that name
/// already exists, else ERR_INTERNAL)
error_code sym_declare_variable(sym_table *t, sym_variable *var);

/// @brief Attempts to declare a new function in the function table
/// @param t Sym_table, where a new function should be delcared
/// @param var Variable to be declared
/// @return Returns error code indicating whether the function was added or not
/// (ERR_NONE if it was added, ERR_REDEFINITION if a function of that name
/// already exists, else ERR_INTERNAL)
error_code sym_declare_function(sym_table *t, sym_function *func,
                                struct node *def);

/// @brief Attemps to look up a variable given it's name by traversing and
/// checking all the ht_tables of all scopes
/// @param t Sym_table, where the variable should exist
/// @param name Name of variable
/// @return Returns a pointer to the variable, if it was found, else NULL
sym_variable *sym_lookup_variable(sym_table *t, const char *name);

/// @brief Attemps to look up a function given it's name
/// @param t Sym_table, where the function should exist
/// @param name Name of function
/// @return Returns a pointer to the function, if it was found, else NULL
sym_function *sym_lookup_function(sym_table *t, const char *name);

/// @brief Detaches all the scopes from the sym_table
/// @param t Sym_table, from which all the scopes should be detached
/// @return Pointer to the innermost scope
sym_scope_data *sym_detach(sym_table *t);

/// @brief Reattaches all the scopes back to the sym_table
/// @param t Sym_table, to which the scopes should be attached
/// @param data Innermost scope that should be attached
/// @return Returns true if attachment went successfully, else false
bool sym_restore(sym_table *t, sym_scope_data *data);

#endif
