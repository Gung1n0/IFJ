#ifndef AST_H
#define AST_H

#include "constants.h"
#include "token.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  char *name;
  data_types types;
  int row;
  int column;
  int list; // PARAMSLIST extention, idx of list
} param;

typedef struct node node;

struct node {
  node_type type;
  int row;
  int column;

  data_types data_type;
  bool convert_to_double;
  struct sym_variable *var;
  struct sym_function *func;
  union {
    struct {
      node **items;
      int size;
      int cap;
    } program;
    struct {
      char *name;
      data_types return_type;
      bool explicit;
      bool is_main;
      param *params;
      int param_count;
      node *body;
      bool no_params;  // paramlistls extention
      int lists_count; // paramlistls extention
    } function;
    struct {
      node **items;
      int size;
      int cap;
    } block;
    struct {
      char *name;
      bool changable;
      data_types type;
      node *init;
    } val_var;
    struct {
      char *name;
      node *init;
    } asign;
    struct {
      node *expression;
    } expression;
    struct {
      node *value;
    } ret;
    struct {
      node *condition;
      node *then;
      node *else_n;
    } if_n;
    struct {
      node *condition;
      node *body;
      node *ret; // maybe not needed
    } while_n;
    struct {
      data_types type;
      int i;
      double d;
      char *s;
      bool b; // BOOLTHEN extention
    } lit;
    struct {
      char *name;
    } ident;
    struct {
      operator op;
      node *operand;
    } unar;
    struct {
      operator op;
      node *left;
      node *right;
    } binar;
    struct {
      char *name;
      node **items;
      int size;
      int cap;
      int lists_count; // EXTKW extention
      int *list_sizes; // EXTKW extention
    } call;
    struct {
      node *obj;
      char *name;
      node **items;
      int size;
      int cap;
      int lists_count;
      int *list_sizes;
    } member; // VALMEM EXTENTION
    struct {
      char *reciever;
      data_types type;
      node **methods;
      int size;
    } extension; // EXTKW extention
    struct {
      char *var_name;
      node *from;
      node *to;
      node *step;
      bool inclusive;
      node **guards;
      int guard_count;
      int guard_cap;
      node *body;

      // filled in by semantics, not by the parser
      struct sym_variable *end_var;
      struct sym_variable *step_var;
    } for_n; // CONTROL extension

    struct {
      node **parts;
      int size;
    } interp;
  } as;
};

// prints error message and exists with error code
static inline void print_error(const char *message, int error_code, int row,
                               int column) {
  fprintf(stderr, "%s at row %i column %i\n", message, row, column);
  exit(error_code);
}

typedef struct {
  item_type type;
  operator oper; // však periem
  union {
    node *n;
    int term;
  } ast;
} stack_item;

typedef struct {
  stack_item **items;
  int size;
  int cap;
} stack;

typedef struct {
  actions table[15][15];
} pre_table;

#endif