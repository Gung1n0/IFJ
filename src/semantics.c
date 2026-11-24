/*
 * implementation of a compiler for the language IFJ26
 * semantics.c
 * given a ast tree, handles all the analyzing logic
 * xzombot00 Tomáš Zomborský
 */

#include "semantics.h"
#include "common_functions.h"
#include "symtable.h"
#include "typerules.h"

#define INITIAL_LOCALS_ARRAY_SIZE 8
#define LOCALS_CAPACITY_MULTIPLIER 2

// CONTROL extension
// "$" symbol isn't allowed in the ifj26 indentifiers, so the user can't type it
#define END_VAR_NAME "$for_end"
#define STEP_VAR_NAME "$for_step"

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

typedef struct {
  sym_function *from;
  sym_function *to;
} call_edge;

static call_edge *edges = NULL;
static int edge_count = 0;
static int edge_capacity = 0;

static data_types analyze_expressions(sem_context *c, node *n);
static analysis_result analyze_statement(sem_context *c, node *n);
static analysis_result analyze_block(sem_context *c, node *n);
static data_types analyze_call(sem_context *c, node *n);
static void analyze_function_entry(sym_table *t, sym_function *f);

/// @brief Prints error message, points to the fault node and exits with a
/// error_code
/// @param err Error code
/// @param n Node to report on
static void sem_error(error_code err, node *n) {
  const char *message = error_to_message(err);
  fprintf(stderr, "%s at row %i column %i\n", message, n->row, n->column);
  exit(err);
}

/// @brief Calls sem_error if given pointer is NULL
/// @param ptr Pointer to be checked
/// @param n Node to be reported on
static void print_err_if_null(void *ptr, node *n) {
  if (ptr == NULL)
    sem_error(ERR_INTERNAL, n);
}

/// @brief Helper function, to check if a node is a function node
/// @param n Node to check
/// @return Returns true if it is a function node, else false
static bool is_function_node(node *n) { return n->type == node_function; }

/// @brief Converts data type to a string version, used for EXTKW
/// @param t Type of variable
/// @return Returns type of variable in string form
static const char *type_to_string(data_types t) {
  switch (t) {
  case TYPE_INT:
    return "Int";
  case TYPE_DOUBLE:
    return "Double";
  case TYPE_STRING:
    return "String";
  case TYPE_BOOLEAN:
    return "Boolean";
  case TYPE_UNIT:
    return "Unit";
  default:
    return "";
  }
}

/// @brief Creates name (key) of a extension (EXTKW)
/// @param t Data type of the reciever, used for creating the name
/// @param name Name of method
/// @param err_node Node to report on in case of failiure
/// @return Returns a string name (key)
static char *create_method_name(data_types t, const char *name,
                                node *err_node) {
  const char *type_name = type_to_string(t);
  size_t length = strlen(type_name) + 1 + strlen(name) + 1;
  char *ptr = calloc(length, sizeof(char));

  print_err_if_null(ptr, err_node);

  if (ptr != NULL)
    snprintf(ptr, length, "%s$%s", type_name, name);
  return ptr;
}

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
    print_err_if_null(c->locals, n);
  }

  c->locals[index] = v;
  c->locals_size++;
}

/// @brief Creates an immutable variable, declares it in the current scope and
/// appends it to the function's locals (CONTROL: loop and hidden variables)
/// @param c Context of the current function
/// @param name Name of the variable (copied)
/// @param type Data type of the variable
/// @param n Node, used to report on a error
/// @return Returns the declared variable
static sym_variable *declare_local_variable(sem_context *c, const char *name,
                                            data_types type, node *n) {
  // heap, not stack: the symtable keeps the pointer and frees it at the end
  sym_variable *var = calloc(1, sizeof(sym_variable));
  print_err_if_null(var, n);

  var->data_type = type;
  var->is_mutable = false;
  var->is_parameter = false;

  // own copy of the name, the symtable frees it
  var->name = strduplicate(name);
  print_err_if_null(var->name, n);

  error_code code = sym_declare_variable(c->table, var);
  if (code != ERR_NONE)
    sem_error(code, n);

  append_to_locals_array(c, var, n);
  return var;
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
  print_err_if_null(sp.name, n);

  return sp;
}

/// @brief Records a call from one function to another into the global array of
/// call edges, growing the array when it is full. The edges are used after the
/// analysis to detect recursion in functions without an explicit return type
/// @param from Function that contains the call (the caller)
/// @param to Function that is being called (the callee)
/// @param n Node, used to report on a error
static void record_call_edge(sym_function *from, sym_function *to, node *n) {
  if (edge_count >= edge_capacity) {
    int new_capacity = (edge_capacity == 0) ? 16 : edge_capacity * 2;
    call_edge *tmp = realloc(edges, new_capacity * sizeof(call_edge));
    print_err_if_null(tmp, n);
    edges = tmp;
    edge_capacity = new_capacity;
  }
  edges[edge_count++] = (call_edge){from, to};
}

/// @brief Depth-first search over the recorded call edges, checks if the
/// target function can be reached by following calls starting at cur
/// @param cur Function the search starts from
/// @param target Function we are looking for
/// @param visited Array of functions that were already searched through, so
/// the search can't loop forever on cycles that don't contain the target
/// @param visited_count Number of functions currently stored in visited
/// @return Returns true if target is reachable from cur, else false
static bool reaches(sym_function *cur, sym_function *target,
                    sym_function **visited, int *visited_count) {
  for (int i = 0; i < edge_count; i++) {
    if (edges[i].from != cur)
      continue;

    sym_function *next = edges[i].to;
    if (next == target)
      return true;

    bool seen = false;
    for (int v = 0; v < *visited_count; v++) {
      if (visited[v] == next) {
        seen = true;
        break;
      }
    }
    if (seen)
      continue;

    visited[(*visited_count)++] = next;
    if (reaches(next, target, visited, visited_count))
      return true;
  }
  return false;
}

/// @brief Checks that every function that takes part in recursion (direct or
/// indirect) has an explicit return type. Has to run after all function bodies
/// were analyzed, so that all the call edges are already recorded
/// @param table Sym_table, used to look up the functions
/// @param program Program node, used to iterate over the functions and to
/// report on a error
static void check_untyped_recursion(sym_table *table, node *program) {
  // each visited entry is a distinct edge target, so edge_count is enough
  sym_function **visited = calloc(edge_count + 1, sizeof(sym_function *));
  print_err_if_null(visited, program);

  for (int i = 0; i < program->as.program.size; i++) {
    node *current = program->as.program.items[i];

    if (current->type == node_function) {
      sym_function *f = sym_lookup_function(table, current->as.function.name);
      print_err_if_null(f, program);

      if (f->has_explicit_return_type)
        continue;

      int visited_count = 0;
      if (reaches(f, f, visited, &visited_count))
        sem_error(ERR_TYPES, current);
    } else if (current->type == node_extension) {

      for (int m_index = 0; m_index < current->as.extension.size; m_index++) {
        sym_function *m = sym_lookup_function(
            table, current->as.extension.methods[m_index]->func->name);
        print_err_if_null(m, program);
        if (m->has_explicit_return_type)
          continue;

        int m_visited_count = 0;
        if (reaches(m, m, visited, &m_visited_count))
          sem_error(ERR_TYPES, current);
      }
    }
  }

  free(visited);
  free(edges);
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
  print_err_if_null(c, n);

  c->table = t;
  c->locals = calloc(INITIAL_LOCALS_ARRAY_SIZE, sizeof(sym_variable *));
  print_err_if_null(c->locals, n);

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
    if (var == NULL) {
      // PARAMLISTS
      sym_function *func = sym_lookup_function(c->table, n->as.ident.name);
      if (func == NULL)
        sem_error(ERR_UNDEFINED, n);
      if (func->lists_count != 0)
        sem_error(ERR_ARGUMENTS, n);

      // transform node_id to node_call
      char *tmp_name = n->as.ident.name;

      n->type = node_call;

      n->as.call.name = tmp_name;
      n->as.call.items = NULL;
      n->as.call.size = 0;
      n->as.call.cap = 0;
      n->as.call.lists_count = 0;
      n->as.call.list_sizes = NULL;

      return analyze_call(c, n);
    }

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
  // VALMEM extension
  case node_member: { // TODO next
    data_types reciever_type = analyze_expressions(c, n->as.member.obj);
    op_result member_results = look_up_member_data_type(n->as.member.name);

    // not a VALMEM method
    if (member_results.ok) {
      if (member_results.operand_type == TYPE_ANY &&
          is_valid_variable_type(reciever_type))
        return member_results.result_type;

      if (member_results.operand_type != reciever_type)
        sem_error(ERR_TYPES, n);
    } else {
      // extension
      char *method_name =
          create_method_name(reciever_type, n->as.member.name, n);
      sym_function *func = sym_lookup_function(c->table, method_name);

      // rewirte node into
      if (func == NULL)
        sem_error(ERR_UNDEFINED, n);

      node **items = calloc(1, sizeof(node *));
      int *sizes = calloc(1, sizeof(int));

      print_err_if_null(items, n);
      print_err_if_null(sizes, n);

      free(n->as.member.name);

      items[0] = n->as.member.obj;
      sizes[0] = 1;

      n->type = node_call;
      n->as.call.name = method_name;
      n->as.call.items = items;
      n->as.call.size = 1;
      n->as.call.cap = 1;
      n->as.call.lists_count = 1;
      n->as.call.list_sizes = sizes;

      return analyze_call(c, n);
    }

    return member_results.result_type;
  }
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

  // PARAMLISTS, check if lists match
  if (function->lists_count != n->as.call.lists_count)
    sem_error(ERR_ARGUMENTS, n);

  for (int i = 0; i < function->lists_count; i++)
    if (function->list_sizes[i] != n->as.call.list_sizes[i])
      sem_error(ERR_ARGUMENTS, n);

  n->func = function;
  record_call_edge(c->current_function_entry, function, n);

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
    print_err_if_null(var, n);

    var->data_type = final_type;
    var->is_mutable = n->as.val_var.changable;
    var->is_parameter = false;

    var->name = strduplicate(n->as.val_var.name);
    print_err_if_null(var->name, n);

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
  case node_for: {
    // CONTROL: bounds and step are analyzed in the OUTER scope, before the
    // loop variable exists (in for i <- 0 until i the bound is the outer i)
    data_types from_type = analyze_expressions(c, n->as.for_n.from);
    if (from_type != TYPE_INT)
      sem_error(ERR_TYPES, n->as.for_n.from);

    data_types to_type = analyze_expressions(c, n->as.for_n.to);
    if (to_type != TYPE_INT)
      sem_error(ERR_TYPES, n->as.for_n.to);

    node *step = n->as.for_n.step;
    if (step != NULL) {
      data_types step_type = analyze_expressions(c, step);
      if (step_type != TYPE_INT)
        sem_error(ERR_TYPES, step);

      // zero step is checked at compile time for literals only
      if (step->type == node_lit && step->as.lit.i == 0)
        sem_error(ERR_OTHER_SEMANTIC, step);
    }

    if (!sym_enter_scope(c->table))
      sem_error(ERR_INTERNAL, n);

    // hidden variables, the end bound and the step are evaluated only once
    n->as.for_n.end_var = declare_local_variable(c, END_VAR_NAME, TYPE_INT, n);
    n->as.for_n.step_var = NULL;
    if (step != NULL)
      n->as.for_n.step_var =
          declare_local_variable(c, STEP_VAR_NAME, TYPE_INT, n);

    // the loop variable: a new immutable Int
    n->var = declare_local_variable(c, n->as.for_n.var_name, TYPE_INT, n);

    // guards can use the loop variable and must be Boolean
    for (int g_index = 0; g_index < n->as.for_n.guard_count; g_index++) {
      node *guard = n->as.for_n.guards[g_index];
      if (analyze_expressions(c, guard) != TYPE_BOOLEAN)
        sem_error(ERR_TYPES, guard);
    }

    analyze_block(c, n->as.for_n.body);

    if (!sym_exit_scope(c->table))
      sem_error(ERR_INTERNAL, n);

    // the body may run zero times, so a for never "always returns"
    return (analysis_result){TYPE_UNIT, false};
  }
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
/// @param ext EXTKW node, leave NULL for ordinary functions
static void declare_function(sym_table *t, node *f, node *ext) {
  sym_function *function = calloc(1, sizeof(sym_function));
  print_err_if_null(function, f);

  bool has_extension = ext != NULL;
  int extra = has_extension ? 1 : 0;

  function->is_extension = has_extension;

  if (!has_extension)
    function->name = strduplicate(f->as.function.name);
  else
    function->name =
        create_method_name(ext->as.extension.type, f->as.function.name, f);

  print_err_if_null(function->name, f);

  // parameters
  // own parameters + the receiver (for extension methods)
  function->parameter_count = f->as.function.param_count + extra;
  function->parameters = NULL;

  if (function->parameter_count != 0) {
    function->parameters =
        calloc(function->parameter_count, sizeof(sym_parameter));
    print_err_if_null(function->parameters, f);

    if (has_extension) {
      if (!is_valid_variable_type(ext->as.extension.type))
        sem_error(ERR_TYPES, ext);

      function->parameters[0].data_type = ext->as.extension.type;
      function->parameters[0].name = strduplicate(ext->as.extension.reciever);
      print_err_if_null(function->parameters[0].name, f);
    }

    // loop over the method's own parameters from 0,
    for (int param_id = 0; param_id < f->as.function.param_count; param_id++)
      function->parameters[param_id + extra] =
          param_to_sym_param(f, f->as.function.params[param_id]);
  }

  // PARAMLISTS
  function->lists_count = f->as.function.lists_count + extra;
  function->list_sizes = NULL;

  if (function->lists_count > 0) {
    function->list_sizes = calloc(function->lists_count, sizeof(int));
    print_err_if_null(function->list_sizes, f);

    if (has_extension)
      function->list_sizes[0] = 1; // the receiver's list

    for (int p_index = 0; p_index < f->as.function.param_count; p_index++) {
      // shift first, then check the index we will actually use
      int list_index = f->as.function.params[p_index].list + extra;
      if (list_index < 0 || list_index >= function->lists_count)
        sem_error(ERR_INTERNAL, f);

      function->list_sizes[list_index]++;
    }
  }

  // return type
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

  // main
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

  f->func = function;
  error_code code = sym_declare_function(t, function, f);
  if (code != ERR_NONE)
    sem_error(code, f);
}
/// @brief Attemps to analyze a function by declaring all of it's parameters
/// as variables, analyzing it's block (body) to see if the function ever
/// returns correct data types
/// @param c Context of the fucntion
/// @param n Function node
static void analyze_function(sem_context *c, node *n) {
  if (!sym_enter_scope(c->table))
    sem_error(ERR_INTERNAL, n);

  n->func = c->current_function_entry;

  if (!c->current_function_entry->has_explicit_return_type)
    c->current_function_entry->i_state = in_progress;

  if (c->current_function_entry->is_extension) {
    sym_variable *var = calloc(1, sizeof(sym_variable));

    print_err_if_null(var, n);

    var->is_mutable = false;
    var->is_parameter = true;
    var->data_type = c->current_function_entry->parameters[0].data_type;
    var->name = strduplicate(c->current_function_entry->parameters[0].name);

    print_err_if_null(var->name, n);

    error_code code = sym_declare_variable(c->table, var);
    if (code != ERR_NONE)
      sem_error(code, n);

    append_to_locals_array(c, var, n);
  }

  // declare parameters as variables
  for (int param_id = 0; param_id < n->as.function.param_count; param_id++) {
    sym_variable *var = calloc(1, sizeof(sym_variable));

    print_err_if_null(var, n);

    var->is_mutable = false;
    var->is_parameter = true;
    var->data_type = n->as.function.params[param_id].types;

    var->name = strduplicate(n->as.function.params[param_id].name);
    print_err_if_null(var->name, n);

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
  edge_count = 0;

  // first pass to locate all the functions
  for (int i = 0; i < program->as.program.size; i++) {
    node *current = program->as.program.items[i];
    bool is_function = is_function_node(current);

    if (current->type == node_extension) {
      // add all methods
      for (int m = 0; m < current->as.extension.size; m++)
        declare_function(table, current->as.extension.methods[m], current);
    } else {
      if (!is_function)
        sem_error(ERR_INTERNAL, current);
      declare_function(table, current, NULL);
    }
  }

  // no main found
  if (main_counter == 0)
    sem_error(ERR_UNDEFINED, program);

  // second pass, analyze the bodies of all the functions
  for (int i = 0; i < program->as.program.size; i++) {
    node *current = program->as.program.items[i];

    bool is_function = is_function_node(current);

    if (current->type == node_extension) {
      // analyze all methods
      for (int m = 0; m < current->as.extension.size; m++) {
        char *method_name = current->as.extension.methods[m]->func->name;
        sym_function *method_entry = sym_lookup_function(table, method_name);

        print_err_if_null(method_entry, program);

        if (!method_entry->has_explicit_return_type &&
            method_entry->i_state != unvisited)
          continue;

        analyze_function_entry(table, method_entry);
      }
    } else {
      if (!is_function)
        sem_error(ERR_INTERNAL, current);

      sym_function *function_entry = current->func;
      if (!function_entry->has_explicit_return_type &&
          function_entry->i_state != unvisited)
        continue;

      analyze_function_entry(table, current->func);
    }
  }
  check_untyped_recursion(table, program);
}