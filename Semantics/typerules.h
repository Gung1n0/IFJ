/*
 * implementation of a compiler for the language IFJ26
 * typerules.h
 * xzombot00 Tomáš Zomborský
 */

#ifndef TYPERULES_H
#define TYPERULES_H

#include "../Common/enums.h"
#include "symtable.h"
#include <stdbool.h>

typedef struct {
  bool ok;
  data_types result_type;
  data_types operand_type;
} op_result;

/// @brief Calls either comparison_operation or binary_numeric_operation
/// depending on the operator type; special logic required with equal /
/// not_equal, because they also support string operations
/// @param op Operator
/// @param type1 Operand type 1
/// @param type2 Operand type 2
/// @return Returns different values depending on the operation
op_result binary_operation(operator op, data_types type1, data_types type2);

/// @brief Computes data type of a unary operation (negate)
/// @param op Operator
/// @param operand Operand
/// @return Returns different results depending on the operand
op_result unary_operation(operator op, data_types operand);

/// @brief Checks type value can be assigned to type target (e.g. double = int)
/// @param target Target type
/// @param value Source type
/// @return Returns true if it can, else false
bool is_assignable(data_types target, data_types value);

/// @brief Checks if target is double and source is int
/// @param target Target type
/// @param value Source type
/// @return Returns true if the statement is true, else false
bool needs_int_to_double(data_types target, data_types value);

/// @brief Checks if a parameter type is compatible with argument type
/// @param parameter Parameter
/// @param argument Argument type
/// @return Returns true if the statement is true, else false
bool is_argument_compatible(sym_parameter parameter, data_types argument);

/// @brief Checks if a return type is compatible with the return type of a
/// function
/// @param declared Return type of a function
/// @param value Type we are checking
/// @return Returns true if the statement is true, else false
bool is_return_compatible(data_types declared, data_types value);

/// @brief Checks if a data type is a valid varaible type
/// @param type Type to be checked
/// @return Returns false if the data type == boolean (for now) / unknown / any,
/// else true
bool is_valid_variable_type(data_types type);

#endif