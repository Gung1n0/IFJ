#include "semantics.h"
#include "symtable.h"
#include "typerules.h"

#define INITIAL_LOCALS_ARRAY_SIZE 8

static int main_counter = 0;

typedef struct
{
    sym_table *table;
    sym_function *current_function_entry;

    sym_variable **locals;

    int locals_size;
    int locals_capacity;
} sem_context;

char *strduplicate(const char *s)
{
    char *p = malloc(strlen(s) + 1);
    if (p != NULL)
    {
        strcpy(p, s);
    }
    return p;
}

static data_types analyze_expressions(sem_context *c, node *n);
static data_types analyze_statement(sem_context *c, node *n);
static void analyze_block(sem_context *c, node *n);

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
        c->locals_capacity *= 2;
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
        // TODO: later
        return TYPE_UNKNOWN; // temporary
    default:
        sem_error(ERR_INTERNAL, n);
    }
    return TYPE_UNKNOWN;
}

static data_types analyze_statement(sem_context *c, node *n)
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
        return TYPE_UNIT;
    }
    case node_while:
    {
        data_types condition = analyze_expressions(c, n->as.while_n.condition);
        if (condition != TYPE_BOOLEAN)
            sem_error(ERR_TYPES, n->as.while_n.condition);
        analyze_block(c, n->as.while_n.body);
        return TYPE_UNIT;
    }
    case node_if:
    {
        data_types condition = analyze_expressions(c, n->as.if_n.condition);
        if (condition != TYPE_BOOLEAN)
            sem_error(ERR_TYPES, n->as.if_n.condition);
        analyze_block(c, n->as.if_n.then);
        if (n->as.if_n.else_n != NULL)
            analyze_block(c, n->as.if_n.else_n);
        return TYPE_UNIT; // TODO
    }
    case node_return:
        // TODO
        return TYPE_UNIT;
    case node_expression:
        return analyze_expressions(c, n->as.expression.expression);
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
        return TYPE_UNIT;
    }
    default:
        sem_error(ERR_INTERNAL, n);
    }
    return TYPE_UNIT;
}

static void analyze_block(sem_context *c, node *n)
{
    if (!sym_enter_scope(c->table))
        sem_error(ERR_INTERNAL, n);

    for (int statement_id = 0; statement_id < n->as.block.size; statement_id++)
    {
        analyze_statement(c, n->as.block.items[statement_id]);
    }

    if (!sym_exit_scope(c->table))
        sem_error(ERR_INTERNAL, n);
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

        // TODO: can't check for return types until pass 2
        // has_explicit_return_type -> return_type == unit
        if (function->has_explicit_return_type && function->return_type != TYPE_UNIT)
            sem_error(ERR_TYPES, f);

        // main can't have parameters
        if (function->parameter_count != 0)
            sem_error(ERR_ARGUMENTS, f);

        main_counter++;
    }

    error_code code = sym_declare_function(t, function);
    if (code != ERR_NONE)
        sem_error(code, f);
}

static void analyze_function(sem_context *c, node *n)
{
    if (!sym_enter_scope(c->table))
        sem_error(ERR_INTERNAL, n);

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
    analyze_block(c, n->as.function.body);

    if (!sym_exit_scope(c->table))
        sem_error(ERR_INTERNAL, n);
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

        sem_context *c = init_sem_context(current, table);
        c->current_function_entry = sym_lookup_function(table, function_name);

        analyze_function(c, current);
    }
}