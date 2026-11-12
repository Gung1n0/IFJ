/*
 * implementation of a compiler for the language IFJ26
 * semantics.c
 * given a ast tree, handles all the analyzing logic
 * xzombot00 Tomáš Zomborský
 */

#include "semantics.h"
#include "../Common/common_functions.h"
#include "symtable.h"
#include "typerules.h"

#define INITIAL_LOCALS_ARRAY_SIZE 8
#define LOCALS_CAPACITY_MULTIPLIER 2

static int main_counter = 0;

typedef struct {
  data_types last_statement;
  bool always_returns;
} analysis_result;

typedef struct {
  sym_table *table;
  sym_function *current_function_entry;

  sym_variable **locals;

  int locals_size;
  int locals_capacity;
} sem_context;

static data_types analyze_expressions(sem_context *c, node *n);
static analysis_result analyze_statement(sem_context *c, node *n);
static analysis_result analyze_block(sem_context *c, node *n);
static data_types analyze_call(sem_context *c, node *n);
static void analyze_function_entry(sym_table *t, sym_function *f);

/// @brief Helper function, to check if a node is a function node
/// @param n Node to check
/// @return Returns true if it is a function node, else false
static bool is_function_node(node *n) { return n->type == node_function; }

/// @brief  Attempts to append a sym_variable pointer to the end of a dynamic
/// array
/// @param c Context of the current function (holds locals array)
/// @param v Variable to be appended
/// @param n Node, used to report on a error
static void append_to_locals_array(sem_context *c, sym_variable *v, node *n) {
  int index = c->locals_size;
  if (index >= c->locals_capacity) {
    c->locals_capacity *= LOCALS_CAPACITY_MULTIPLIER;
    c->locals = realloc(c->locals, c->locals_capacity * sizeof(sym_variable *));
    if (c->locals == NULL)
      sem_error(ERR_INTERNAL, n);
  }

  c->locals[index] = v;
  c->locals_size++;
}
/// @brief Converts parsers "param" to "sym_parameter"
/// @param n Node, used to report on a error
/// @param p Parameter "param" to be converted into "sym_parameter"
/// @return Returns sym_parameter
static sym_parameter param_to_sym_param(node *n, param p) {
  sym_parameter sp;

  if (!is_valid_variable_type(p.types))
    sem_error(ERR_TYPES, n);

  sp.data_type = p.types;

  sp.name = strduplicate(p.name);
  if (sp.name == NULL)
    sem_error(ERR_INTERNAL, n);

  return sp;
}

/// @brief Marks node if it requires to be converted to double
/// @param value Node value
/// @param target Target data type
static void mark_conversion(node *value, data_types target) {
  if (needs_int_to_double(target, value->data_type))
    value->convert_to_double = true;
}

static sem_context *init_sem_context(node *n, sym_table *t) {
  sem_context *c = calloc(1, sizeof(sem_context));
  if (c == NULL)
    sem_error(ERR_INTERNAL, n);

  c->table = t;
  c->locals = calloc(INITIAL_LOCALS_ARRAY_SIZE, sizeof(sym_variable *));
  if (c->locals == NULL)
    sem_error(ERR_INTERNAL, n);

  c->locals_size = 0;
  c->locals_capacity = INITIAL_LOCALS_ARRAY_SIZE;

  return c;
}

/// @brief Computes the type of a given expression
/// @param c Context of the current function
/// @param n Node to be analyzed
/// @return Returns data type of given expression
static data_types compute_expression_type(sem_context *c, node *n) {
  switch (n->type) {
  case node_lit:
    return n->as.lit.type;
  case node_id: {
    sym_variable *var = sym_lookup_variable(c->table, n->as.ident.name);
    if (var == NULL)
      sem_error(ERR_UNDEFINED, n);
    n->var = var;
    return var->data_type;
  }
  case node_unar: {
    data_types operand = analyze_expressions(c, n->as.unar.operand);
    op_result unar_result = unary_operation(n->as.unar.op, operand);
    if (!unar_result.ok)
      sem_error(ERR_TYPES, n);
    return unar_result.result_type;
  }
  case node_binar: {
    data_types left = analyze_expressions(c, n->as.binar.left);
    data_types right = analyze_expressions(c, n->as.binar.right);

    op_result binar_result = binary_operation(n->as.binar.op, left, right);

    if (!binar_result.ok)
      sem_error(ERR_TYPES, n);

    if (binar_result.operand_type == TYPE_DOUBLE) {
      mark_conversion(n->as.binar.left, TYPE_DOUBLE);
      mark_conversion(n->as.binar.right, TYPE_DOUBLE);
    }

    return binar_result.result_type;
  }
  case node_call:
    return analyze_call(c, n);
  default:
    sem_error(ERR_INTERNAL, n);
  }

  return TYPE_UNKNOWN;
}

/// @brief Runs compute_expression_type and sets the nodes type to teh result
/// @param c Context of the current function
/// @param n Node to be analyzed
/// @return Returns the computed data type of the given expression
static data_types analyze_expressions(sem_context *c, node *n) {
  data_types t = compute_expression_type(c, n);
  n->data_type = t;
  return t;
}

/// @brief Analyzes call
/// @param c Context of the current function
/// @param n Node to be analyzed
/// @return Returns call's data type
static data_types analyze_call(sem_context *c, node *n) {
  // look up the function and check that it exists
  char *function_name = n->as.call.name;
  sym_function *function = sym_lookup_function(c->table, function_name);

  if (function == NULL)
    sem_error(ERR_UNDEFINED, n);

  if (n->as.call.size != function->parameter_count)
    sem_error(ERR_ARGUMENTS, n);

  n->func = function;

  // check if the functions arguments are correct
  for (int arg_id = 0; arg_id < function->parameter_count; arg_id++) {
    data_types arg_type = analyze_expressions(c, n->as.call.items[arg_id]);
    if (!is_argument_compatible(function->parameters[arg_id], arg_type))
      sem_error(ERR_ARGUMENTS, n);

    if (function->parameters[arg_id].data_type != TYPE_ANY)
      mark_conversion(n->as.call.items[arg_id],
                      function->parameters[arg_id].data_type);
  }
  switch (function->i_state) {
  case done:
    n->data_type = function->return_type;
    return function->return_type;
  case unvisited: {
    sym_scope_data *scope_data = sym_detach(c->table);
    analyze_function_entry(c->table, function);
    bool success = sym_restore(c->table, scope_data);

    if (!success)
      sem_error(ERR_INTERNAL, n);

    n->data_type = function->return_type;
    return function->return_type;
  }
  case in_progress:
    sem_error(ERR_TYPES, n);
    break;
  default:
    sem_error(ERR_INTERNAL, n);
    break;
  }

  return TYPE_UNKNOWN;
}

/// @brief Analyzes a statement, and chooses what logic to compute depending on
/// type of statement
/// @param c Context of the current function
/// @param n Node to be analyzed
/// @return Returns analysis result, that tells us information about the data
/// type of return
static analysis_result analyze_statement(sem_context *c, node *n) {
  switch (n->type) {
  case node_asign: {
    // check if variable already exists
    sym_variable *target = sym_lookup_variable(c->table, n->as.asign.name);
    if (target == NULL)
      sem_error(ERR_UNDEFINED, n);
    if (!target->is_mutable)
      sem_error(ERR_IMMUTABLE, n);

    // analyze expression, to see if the two data types can be assigned
    data_types value_type = analyze_expressions(c, n->as.asign.init);
    if (!is_assignable(target->data_type, value_type))
      sem_error(ERR_TYPES, n);

    n->var = target;
    mark_conversion(n->as.asign.init, target->data_type);
    return (analysis_result){TYPE_UNIT, false};
  }
  case node_while: {
    // check what type the condition is; has to be a boolean expression
    data_types condition = analyze_expressions(c, n->as.while_n.condition);
    if (condition != TYPE_BOOLEAN)
      sem_error(ERR_TYPES, n->as.while_n.condition);
    analyze_block(c, n->as.while_n.body);
    return (analysis_result){TYPE_UNIT, false};
  }
  case node_if: {
    // check what type the condition is; has to be a boolean expression
    data_types condition = analyze_expressions(c, n->as.if_n.condition);
    if (condition != TYPE_BOOLEAN)
      sem_error(ERR_TYPES, n->as.if_n.condition);

    // analyze if_then and else branches
    analysis_result then_block_analysis_result = {TYPE_UNIT, false};
    analysis_result else_block_analysis_result = {TYPE_UNIT, false};

    then_block_analysis_result = analyze_block(c, n->as.if_n.then);
    if (n->as.if_n.else_n != NULL)
      else_block_analysis_result = analyze_block(c, n->as.if_n.else_n);

    bool then_ret = then_block_analysis_result.always_returns;
    bool else_ret = else_block_analysis_result.always_returns;

    bool then_ok =
        then_ret || then_block_analysis_result.last_statement == TYPE_UNIT;
    bool else_ok =
        else_ret || else_block_analysis_result.last_statement == TYPE_UNIT;

    data_types if_type = (then_ok && else_ok) ? TYPE_UNIT : TYPE_UNKNOWN;
    bool always_returns = then_ret && else_ret;

    return (analysis_result){if_type, always_returns};
  }
  case node_return: {
    // if function doesn't have a explicit return type, deny the return
    if (!c->current_function_entry->has_explicit_return_type)
      sem_error(ERR_TYPES, n);

    data_types declared = c->current_function_entry->return_type;

    if (n->as.ret.value == NULL) {
      // bare return
      if (declared != TYPE_UNIT)
        sem_error(ERR_TYPES, n);
    } else {
      // returns a data type, check if it can be returned
      data_types value_type = analyze_expressions(c, n->as.ret.value);
      if (!is_return_compatible(c->current_function_entry->return_type,
                                value_type))
        sem_error(ERR_TYPES, n);
      if (declared != TYPE_UNIT)
        mark_conversion(n->as.ret.value, declared);
    }
    return (analysis_result){c->current_function_entry->return_type, true};
  }
  case node_expression:
    // analyze the expression and return it's data type
    return (analysis_result){
        analyze_expressions(c, n->as.expression.expression), false};
  case node_val_var: {
    // initializing a new variable
    data_types init_type = analyze_expressions(c, n->as.val_var.init);
    data_types final_type;

    if (n->as.val_var.type != TYPE_UNKNOWN) {
      final_type = n->as.val_var.type;
      if (!is_valid_variable_type(final_type) ||
          !is_assignable(final_type, init_type))
        sem_error(ERR_TYPES, n);
    } else {
      final_type = init_type;
      if (!is_valid_variable_type(final_type))
        sem_error(ERR_TYPES, n);
    }

    // allocate the variable
    sym_variable *var = calloc(1, sizeof(sym_variable));
    if (var == NULL)
      sem_error(ERR_INTERNAL, n);

    var->data_type = final_type;
    var->is_mutable = n->as.val_var.changable;
    var->is_parameter = false;

    var->name = strduplicate(n->as.val_var.name);
    if (var->name == NULL)
      sem_error(ERR_INTERNAL, n);

    error_code code = sym_declare_variable(c->table, var);
    if (code != ERR_NONE)
      sem_error(code, n);

    n->var = var;
    mark_conversion(n->as.val_var.init, final_type);

    append_to_locals_array(c, var, n);
    return (analysis_result){TYPE_UNIT, false};
  }
  case node_call:
    // analyze the call and return it's return type
    return (analysis_result){analyze_call(c, n), false};
  default:
    sem_error(ERR_INTERNAL, n);
  }
  // dummy code, to keep the compiler from throwing warnings
  return (analysis_result){TYPE_UNIT, false};
}

/// @brief Analyzes all statements in the block
/// @param c Context of the given function
/// @param n Node to be analyzed
/// @return Returns analysis result, that tells us information about the data
/// type of return
static analysis_result analyze_block(sem_context *c, node *n) {
  if (!sym_enter_scope(c->table))
    sem_error(ERR_INTERNAL, n);

  analysis_result result = {TYPE_UNKNOWN, false};

  for (int statement_id = 0; statement_id < n->as.block.size; statement_id++)
    result = analyze_statement(c, n->as.block.items[statement_id]);

  if (!sym_exit_scope(c->table))
    sem_error(ERR_INTERNAL, n);

  return result;
}

/// @brief Declares a function, and handles logic related to delcaring a
/// function (such as there can only be 1 main)
/// @param t Sym_table, where we store the declared function
/// @param f Function node to be declared
static void declare_function(sym_table *t, node *f) {
  sym_function *function = calloc(1, sizeof(sym_function));
  if (function == NULL)
    sem_error(ERR_INTERNAL, f);

  function->name = strduplicate(f->as.function.name);
  if (function->name == NULL)
    sem_error(ERR_INTERNAL, f);

  function->parameter_count = f->as.function.param_count;

  if (f->as.function.param_count != 0) {
    function->parameters =
        calloc(f->as.function.param_count, sizeof(sym_parameter));
    if (function->parameters == NULL)
      sem_error(ERR_INTERNAL, f);
    for (int param_id = 0; param_id < f->as.function.param_count; param_id++)
      function->parameters[param_id] =
          param_to_sym_param(f, f->as.function.params[param_id]);
  }

  function->has_explicit_return_type = f->as.function.explicit;

  if (function->has_explicit_return_type) {
    function->i_state = done;
    if (is_valid_variable_type(f->as.function.return_type))
      function->return_type = f->as.function.return_type;
    else
      sem_error(ERR_TYPES, f);
  } else {
    function->return_type = TYPE_UNKNOWN;
    function->i_state = unvisited;
  }

  function->is_main = f->as.function.is_main;
  function->is_built_in = false;

  if (function->is_main) {
    // there can only be 1 main
    if (main_counter >= 1)
      sem_error(ERR_REDEFINITION, f);

    if (function->has_explicit_return_type &&
        function->return_type != TYPE_UNIT)
      sem_error(ERR_TYPES, f);

    // main can't have parameters
    if (function->parameter_count != 0)
      sem_error(ERR_ARGUMENTS, f);

    main_counter++;
  }

  error_code code = sym_declare_function(t, function, f);
  if (code != ERR_NONE)
    sem_error(code, f);
}
/// @brief Attemps to analyze a function by declaring all of it's parameters as
/// variables, analyzing it's block (body) to see if the function ever returns
/// correct data types
/// @param c Context of the fucntion
/// @param n Function node
static void analyze_function(sem_context *c, node *n) {
  if (!sym_enter_scope(c->table))
    sem_error(ERR_INTERNAL, n);

  n->func = c->current_function_entry;

  if (!c->current_function_entry->has_explicit_return_type)
    c->current_function_entry->i_state = in_progress;

  // declare parameters as variables
  for (int param_id = 0; param_id < n->as.function.param_count; param_id++) {
    sym_variable *var = calloc(1, sizeof(sym_variable));

    if (var == NULL)
      sem_error(ERR_INTERNAL, n);

    var->is_mutable = false;
    var->is_parameter = true;
    var->data_type = n->as.function.params[param_id].types;

    var->name = strduplicate(n->as.function.params[param_id].name);
    if (var->name == NULL)
      sem_error(ERR_INTERNAL, n);

    error_code code = sym_declare_variable(c->table, var);
    if (code != ERR_NONE)
      sem_error(code, n);

    append_to_locals_array(c, var, n);
  }
  analysis_result block_result = analyze_block(c, n->as.function.body);

  // logic to check if the function body returns correct data types
  if (!block_result.always_returns) {
    if (c->current_function_entry->has_explicit_return_type) {
      data_types declared = c->current_function_entry->return_type;
      if (!is_return_compatible(declared, block_result.last_statement))
        sem_error(ERR_TYPES, n);

      if (declared != TYPE_UNIT) {
        node *body = n->as.function.body;
        node *last = body->as.block.items[body->as.block.size - 1];
        if (last->type == node_expression)
          mark_conversion(last->as.expression.expression, declared);
        else if (last->type == node_call)
          mark_conversion(last, declared);
      }

    } else {
      if (!is_valid_variable_type(block_result.last_statement))
        sem_error(ERR_TYPES, n);

      c->current_function_entry->return_type = block_result.last_statement;

      if (c->current_function_entry->is_main &&
          block_result.last_statement != TYPE_UNIT)
        sem_error(ERR_TYPES, n);
    }
  }
  c->current_function_entry->i_state = done;

  if (!sym_exit_scope(c->table))
    sem_error(ERR_INTERNAL, n);
}
/// @brief Creates a context object, then calls analyze_function
/// @param t Sym_table, used to create context
/// @param f Function to be analyzed
static void analyze_function_entry(sym_table *t, sym_function *f) {
  if (f->definition == NULL) {
    // no node to report on, so we have to exit manually
    const char *message = error_to_message(ERR_INTERNAL);
    fprintf(stderr, "%s", message);
    exit(ERR_INTERNAL);
  }

  sem_context *c = init_sem_context(f->definition, t);
  c->current_function_entry = f;
  analyze_function(c, f->definition);

  f->locals = c->locals;
  f->locals_count = c->locals_size;
  free(c);
}
/// @brief Analyzes the ast tree using 2 passes, one pass for analyzing all
/// function headeres (so functions can be used even if they weren't defined
/// yet), and then the second pass for actually analyzing the bodies of all
/// functions
void sem_analyze(node *program, sym_table *table) {
  if (program->type != node_program)
    sem_error(ERR_INTERNAL, program);

  main_counter = 0;

  // first pass to locate all the functions
  for (int i = 0; i < program->as.program.size; i++) {
    node *current = program->as.program.items[i];

    bool is_function = is_function_node(current);
    if (!is_function)
      sem_error(ERR_INTERNAL, current);
    declare_function(table, current);
  }

  // no main found
  if (main_counter == 0)
    sem_error(ERR_UNDEFINED, program);

  // second pass, analyze the bodies of all the functions
  for (int i = 0; i < program->as.program.size; i++) {
    node *current = program->as.program.items[i];

    bool is_function = is_function_node(current);
    if (!is_function)
      sem_error(ERR_INTERNAL, current);

    char *function_name = current->as.function.name;

    sym_function *function_entry = sym_lookup_function(table, function_name);

    if (function_entry == NULL)
      sem_error(ERR_INTERNAL, program);

    if (!function_entry->has_explicit_return_type &&
        function_entry->i_state != unvisited)
      continue;

    analyze_function_entry(table, function_entry);
  }
}