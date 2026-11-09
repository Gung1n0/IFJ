#ifndef TYPERULES_H
#define TYPERULES_H

#include <stdbool.h>
#include "../Common/enums.h"
#include "symtable.h"

typedef struct
{
    bool ok;
    data_types result_type;
    data_types operand_type;
} op_result;

op_result binary_operation(operator op, data_types type1, data_types type2);
op_result unary_operation(operator op, data_types operand);

bool is_assignable(data_types target, data_types value);
bool needs_int_to_double(data_types target, data_types value);
bool is_argument_compatible(sym_parameter parameter, data_types argument);
bool is_return_compatible(data_types declared, data_types value);
bool is_valid_variable_type(data_types type);

#endif