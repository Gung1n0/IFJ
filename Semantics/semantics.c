#include "semantics.h"
#include "symtable.h"
#include "typerules.h"
#include "../Common/common_functions.h"

#define INITIAL_LOCALS_ARRAY_SIZE 8
#define LOCALS_CAPACITY_MULTIPLIER 2

static int main_counter = 0;

typedef struct
{
    data_types last_statement;
    bool always_returns;
} analysis_result;

typedef struct
{
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

static const char *error_to_message(error_code err)
{
    // TODO: rework switch with actual error messages
    switch (err)
    {
    case ERR_ARGUMENTS:
        return "argument error";
    case ERR_IMMUTABLE:
        return "immutable error";
    case ERR_INTERNAL:
        return "internal error";
    case ERR_LEXICAL:
        return "lexical error";
    case ERR_NONE:
        return "no error";
    case ERR_OTHER_SEMANTIC:
        return "other semantic error";
    case ERR_REDEFINITION:
        return "redifinition error";
    case ERR_SYNTAX:
        return "syntax error";
    case ERR_TYPES:
        return "types error";
    case ERR_UNDEFINED:
        return "undefined error";
    default:
        return "";
    }
}

static bool is_function_node(node *n)
{
    return n->type == node_function;
}

static void sem_error(error_code err, node *n)
{
    const char *message = error_to_message(err);
    fprintf(stderr, "%s at row %i column %i\n", message, n->row, n->column);
    exit(err);
}

static void append_to_locals_array(sem_context *c, sym_variable *v, node *n)
{
    int index = c->locals_size;
    if (index >= c->locals_capacity)
    {
        c->locals_capacity *= LOCALS_CAPACITY_MULTIPLIER;
        c->locals = realloc(c->locals, c->locals_capacity * sizeof(sym_variable *));
        if (c->locals == NULL)
            sem_error(ERR_INTERNAL, n);
    }

    c->locals[index] = v;
    c->locals_size++;
}

static sym_parameter param_to_sym_param(node *n, param p)
{
    sym_parameter sp;

    // if the parser is correct, then this check should always pass
    if (!is_valid_variable_type(p.types))
        sem_error(ERR_TYPES, n);

    sp.data_type = p.types;

    // TODO: replace with own, custom strdup function
    sp.name = strduplicate(p.name);
    if (sp.name == NULL)
        sem_error(ERR_INTERNAL, n);

    return sp;
}
static sem_context *init_sem_context(node *n, sym_table *t)
{
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

static data_types analyze_expressions(sem_context *c, node *n)
{
    switch (n->type)
    {
    case node_lit:
        return n->as.lit.type;
    case node_id:
    {
        sym_variable *var = sym_lookup_variable(c->table, n->as.ident.name);
        if (var == NULL)
            sem_error(ERR_UNDEFINED, n);
        return var->data_type;
    }
    case node_unar:
    {
        data_types operand = analyze_expressions(c, n->as.unar.operand);
        op_result unar_result = unary_operation(n->as.unar.op, operand);
        if (!unar_result.ok)
            sem_error(ERR_TYPES, n);
        return unar_result.result_type;
    }
    case node_binar:
    {
        data_types left = analyze_expressions(c, n->as.binar.left);
        data_types right = analyze_expressions(c, n->as.binar.right);

        op_result binar_result = binary_operation(n->as.binar.op, left, right);

        if (!binar_result.ok)
            sem_error(ERR_TYPES, n);
        return binar_result.result_type;
    }
    case node_call:
        return analyze_call(c, n);
    default:
        sem_error(ERR_INTERNAL, n);
    }
    return TYPE_UNKNOWN;
}

static data_types analyze_call(sem_context *c, node *n)
{
    char *function_name = n->as.call.name;
    sym_function *function = sym_lookup_function(c->table, function_name);

    if (function == NULL)
        sem_error(ERR_UNDEFINED, n);

    if (n->as.call.size != function->parameter_count)
        sem_error(ERR_ARGUMENTS, n);

    for (int arg_id = 0; arg_id < function->parameter_count; arg_id++)
    {
        data_types arg_type = analyze_expressions(c, n->as.call.items[arg_id]);
        if (!is_argument_compatible(function->parameters[arg_id], arg_type))
            sem_error(ERR_ARGUMENTS, n);
    }
    switch (function->i_state)
    {
    case done:
        return function->return_type;
    case unvisited:
    {
        sym_scope_data *scope_data = sym_detach(c->table);
        analyze_function_entry(c->table, function);
        bool success = sym_restore(c->table, scope_data);

        if (!success)
            sem_error(ERR_INTERNAL, n);

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

static analysis_result analyze_statement(sem_context *c, node *n)
{
    switch (n->type)
    {
    case node_asign:
    {
        sym_variable *target = sym_lookup_variable(c->table, n->as.asign.name);
        if (target == NULL)
            sem_error(ERR_UNDEFINED, n);
        if (!target->is_mutable)
            sem_error(ERR_IMMUTABLE, n);

        data_types value_type = analyze_expressions(c, n->as.asign.init);
        if (!is_assignable(target->data_type, value_type))
            sem_error(ERR_TYPES, n);
        return (analysis_result){TYPE_UNIT, false};
    }
    case node_while:
    {
        data_types condition = analyze_expressions(c, n->as.while_n.condition);
        if (condition != TYPE_BOOLEAN)
            sem_error(ERR_TYPES, n->as.while_n.condition);
        analyze_block(c, n->as.while_n.body);
        return (analysis_result){TYPE_UNIT, false};
    }
    case node_if:
    {
        data_types condition = analyze_expressions(c, n->as.if_n.condition);
        if (condition != TYPE_BOOLEAN)
            sem_error(ERR_TYPES, n->as.if_n.condition);

        analysis_result then_block_analysis_result = {TYPE_UNIT, false};
        analysis_result else_block_analysis_result = {TYPE_UNIT, false};

        then_block_analysis_result = analyze_block(c, n->as.if_n.then);
        if (n->as.if_n.else_n != NULL)
            else_block_analysis_result = analyze_block(c, n->as.if_n.else_n);

        bool then_ret = then_block_analysis_result.always_returns;
        bool else_ret = else_block_analysis_result.always_returns;

        bool then_ok = then_ret || then_block_analysis_result.last_statement == TYPE_UNIT;
        bool else_ok = else_ret || else_block_analysis_result.last_statement == TYPE_UNIT;

        data_types if_type = (then_ok && else_ok) ? TYPE_UNIT : TYPE_UNKNOWN;
        bool always_returns = then_ret && else_ret;

        return (analysis_result){if_type, always_returns};
    }
    case node_return:
    {
        if (!c->current_function_entry->has_explicit_return_type)
            sem_error(ERR_TYPES, n);

        data_types declared = c->current_function_entry->return_type;

        if (n->as.ret.value == NULL)
        {
            // bare return
            if (declared != TYPE_UNIT)
                sem_error(ERR_TYPES, n);
        }
        else
        {
            data_types value_type = analyze_expressions(c, n->as.ret.value);
            if (!is_return_compatible(c->current_function_entry->return_type, value_type))
                sem_error(ERR_TYPES, n);
        }
        return (analysis_result){c->current_function_entry->return_type, true};
    }
    case node_expression:
        return (analysis_result){analyze_expressions(c, n->as.expression.expression), false};
    case node_val_var:
    {
        data_types init_type = analyze_expressions(c, n->as.val_var.init);
        data_types final_type;

        if (n->as.val_var.type != TYPE_UNKNOWN)
        {
            final_type = n->as.val_var.type;
            if (!is_valid_variable_type(final_type) || !is_assignable(final_type, init_type))
                sem_error(ERR_TYPES, n);
        }
        else
        {
            final_type = init_type;
            if (!is_valid_variable_type(final_type))
                sem_error(ERR_TYPES, n);
        }

        sym_variable *var = calloc(1, sizeof(sym_variable));
        if (var == NULL)
            sem_error(ERR_INTERNAL, n);

        var->data_type = final_type;
        var->is_mutable = n->as.val_var.changable;
        var->is_parameter = false;

        // TODO: replace
        var->name = strduplicate(n->as.val_var.name);
        if (var->name == NULL)
            sem_error(ERR_INTERNAL, n);

        error_code code = sym_declare_variable(c->table, var);
        if (code != ERR_NONE)
            sem_error(code, n);

        append_to_locals_array(c, var, n);
        return (analysis_result){TYPE_UNIT, false};
    }
    case node_call:
        return (analysis_result){analyze_call(c, n), false};
    default:
        sem_error(ERR_INTERNAL, n);
    }
    return (analysis_result){TYPE_UNIT, false};
}

static analysis_result analyze_block(sem_context *c, node *n)
{
    if (!sym_enter_scope(c->table))
        sem_error(ERR_INTERNAL, n);

    analysis_result result = {TYPE_UNKNOWN, false};

    for (int statement_id = 0; statement_id < n->as.block.size; statement_id++)
        result = analyze_statement(c, n->as.block.items[statement_id]);

    if (!sym_exit_scope(c->table))
        sem_error(ERR_INTERNAL, n);

    return result;
}

static void declare_function(sym_table *t, node *f)
{
    sym_function *function = calloc(1, sizeof(sym_function));
    if (function == NULL)
        sem_error(ERR_INTERNAL, f);

    // TODO: replace with own, custom strdup function
    function->name = strduplicate(f->as.function.name);
    if (function->name == NULL)
        sem_error(ERR_INTERNAL, f);

    function->parameter_count = f->as.function.param_count;

    if (f->as.function.param_count != 0)
    {
        function->parameters = calloc(f->as.function.param_count, sizeof(sym_parameter));
        if (function->parameters == NULL)
            sem_error(ERR_INTERNAL, f);
        for (int param_id = 0; param_id < f->as.function.param_count; param_id++)
            function->parameters[param_id] = param_to_sym_param(f, f->as.function.params[param_id]);
    }

    function->has_explicit_return_type = f->as.function.explicit;

    if (function->has_explicit_return_type)
    {
        function->i_state = done;
        if (is_valid_variable_type(f->as.function.return_type))
            function->return_type = f->as.function.return_type;
        else
            sem_error(ERR_TYPES, f);
    }
    else
    {
        function->return_type = TYPE_UNKNOWN;
        function->i_state = unvisited;
    }

    function->is_main = f->as.function.is_main;
    function->is_built_in = false;

    if (function->is_main)
    {
        // there can only be 1 main
        if (main_counter >= 1)
            sem_error(ERR_REDEFINITION, f);

        if (function->has_explicit_return_type && function->return_type != TYPE_UNIT)
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

static void analyze_function(sem_context *c, node *n)
{
    if (!sym_enter_scope(c->table))
        sem_error(ERR_INTERNAL, n);

    if (!c->current_function_entry->has_explicit_return_type)
        c->current_function_entry->i_state = in_progress;

    for (int param_id = 0; param_id < n->as.function.param_count; param_id++)
    {
        sym_variable *var = calloc(1, sizeof(sym_variable));

        if (var == NULL)
            sem_error(ERR_INTERNAL, n);

        var->is_mutable = false;
        var->is_parameter = true;
        var->data_type = n->as.function.params[param_id].types;

        // TODO: replace with own, custom strdup function
        var->name = strduplicate(n->as.function.params[param_id].name);
        if (var->name == NULL)
            sem_error(ERR_INTERNAL, n);

        error_code code = sym_declare_variable(c->table, var);
        if (code != ERR_NONE)
            sem_error(code, n);

        append_to_locals_array(c, var, n);
    }
    analysis_result block_result = analyze_block(c, n->as.function.body);

    if (!block_result.always_returns)
    {
        if (c->current_function_entry->has_explicit_return_type)
        {
            data_types declared = c->current_function_entry->return_type;
            if (!is_return_compatible(declared, block_result.last_statement))
                sem_error(ERR_TYPES, n);
        }
        else
        {
            if (!is_valid_variable_type(block_result.last_statement))
                sem_error(ERR_TYPES, n);

            c->current_function_entry->return_type = block_result.last_statement;

            if (c->current_function_entry->is_main && block_result.last_statement != TYPE_UNIT)
                sem_error(ERR_TYPES, n);
        }
    }
    c->current_function_entry->i_state = done;

    if (!sym_exit_scope(c->table))
        sem_error(ERR_INTERNAL, n);
}

static void analyze_function_entry(sym_table *t, sym_function *f)
{
    if (f->definition == NULL)
    {
        // no node to report on, so we have to exit manually
        const char *message = error_to_message(ERR_INTERNAL);
        fprintf(stderr, "%s", message);
        exit(ERR_INTERNAL);
    }

    sem_context *c = init_sem_context(f->definition, t);
    c->current_function_entry = f;
    analyze_function(c, f->definition);

    // we don't free entries because they belong to the sym_table
    free(c->locals);
    free(c);
}

void sem_analyze(node *program, sym_table *table)
{
    if (program->type != node_program)
        sem_error(ERR_INTERNAL, program);

    main_counter = 0;

    // first pass to locate all the functions
    for (int i = 0; i < program->as.program.size; i++)
    {
        node *current = program->as.program.items[i];

        bool is_function = is_function_node(current);
        if (!is_function)
            sem_error(ERR_INTERNAL, current);
        declare_function(table, current);
    }

    // no main found
    if (main_counter == 0)
        sem_error(ERR_UNDEFINED, program);

    for (int i = 0; i < program->as.program.size; i++)
    {
        node *current = program->as.program.items[i];

        bool is_function = is_function_node(current);
        if (!is_function)
            sem_error(ERR_INTERNAL, current);

        char *function_name = current->as.function.name;

        sym_function *function_entry = sym_lookup_function(table, function_name);

        if (function_entry == NULL)
            sem_error(ERR_INTERNAL, program);

        if (!function_entry->has_explicit_return_type && function_entry->i_state != unvisited)
            continue;

        analyze_function_entry(table, function_entry);
    }
}