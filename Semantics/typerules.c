#include "typerules.h"
#include "../Common/enums.h"

static bool are_data_types_numeric(data_types type1, data_types type2)
{
    bool is_type1_numeric = (type1 == TYPE_INT || type1 == TYPE_DOUBLE);
    bool is_type2_numeric = (type2 == TYPE_INT || type2 == TYPE_DOUBLE);
    return is_type1_numeric && is_type2_numeric;
}

static op_result binary_numeric_operation(data_types type1, data_types type2)
{
    if (type1 == TYPE_INT && type2 == TYPE_INT) // both integers
        return (op_result){true, TYPE_INT, TYPE_INT};

    // every other case
    return (op_result){true, TYPE_DOUBLE, TYPE_DOUBLE};
}

static op_result comparison_operation(data_types type1, data_types type2)
{
    if (type1 == TYPE_STRING && type2 == TYPE_STRING) // string op string
        return (op_result){true, TYPE_BOOLEAN, TYPE_STRING};
    if (type1 == TYPE_INT && type2 == TYPE_INT) // int op int
        return (op_result){true, TYPE_BOOLEAN, TYPE_INT};
    if (are_data_types_numeric(type1, type2)) // (int/double) op (int/double)
        return (op_result){true, TYPE_BOOLEAN, TYPE_DOUBLE};
    return (op_result){false, TYPE_UNKNOWN, TYPE_UNKNOWN};
}

op_result binary_operation(operator op, data_types type1, data_types type2)
{
    switch (op)
    {
    case OP_ADD:
        if (type1 == TYPE_STRING && type2 == TYPE_STRING)
            return (op_result){true, TYPE_STRING, TYPE_STRING};
        if (are_data_types_numeric(type1, type2))
            return binary_numeric_operation(type1, type2);
        return (op_result){false, TYPE_UNKNOWN, TYPE_UNKNOWN};
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
        if (are_data_types_numeric(type1, type2))
            return binary_numeric_operation(type1, type2);
        return (op_result){false, TYPE_UNKNOWN, TYPE_UNKNOWN};
    case OP_LESSER:
    case OP_LESSER_OR_EQUAL:
    case OP_GREATER:
    case OP_GREATER_OR_EQUAL:
    case OP_EQUAL:
    case OP_NOT_EQUAL:
        return comparison_operation(type1, type2);
    default:
        return (op_result){false, TYPE_UNKNOWN, TYPE_UNKNOWN};
    }
}

op_result unary_operation(operator op, data_types operand)
{
    if (op == OP_NEGATE)
    {
        if (operand == TYPE_INT || operand == TYPE_DOUBLE)
            return (op_result){true, operand, operand};
        return (op_result){false, TYPE_UNKNOWN, TYPE_UNKNOWN};
    }
    return (op_result){false, TYPE_UNKNOWN, TYPE_UNKNOWN};
}

bool is_assignable(data_types target, data_types value)
{
    if (target == TYPE_UNKNOWN || value == TYPE_UNKNOWN)
        return false;

    if (target == value)
        return true;

    return needs_int_to_double(target, value);
}
bool needs_int_to_double(data_types target, data_types value)
{
    if (target == TYPE_DOUBLE && value == TYPE_INT)
        return true;
    return false;
}
bool is_argument_compatible(sym_parameter parameter, data_types argument)
{
    if (parameter.data_type == TYPE_ANY)
        return true;
    return is_assignable(parameter.data_type, argument);
}
bool is_return_compatible(data_types declared, data_types value)
{
    if (declared == TYPE_UNIT)
        return true;
    return is_assignable(declared, value);
}
bool is_valid_variable_type(data_types type)
{
    if (type == TYPE_UNKNOWN || type == TYPE_ANY)
        return false;
    return true;
}
