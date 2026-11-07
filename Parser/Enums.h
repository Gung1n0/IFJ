#ifndef ENUMS_H
#define ENUMS_H

typedef enum {
    token_identifier,
    token_keyword,
    token_arithmetic_operator,
    token_relation_operator,
    token_int,
    token_double,
    token_string,
    token_specialchar,
    token_bracket,
    token_semicolon,
    token_indent,
    token_dedent,
    token_newline,
    token_eof
} token_type;

typedef enum {
    type_int,
    type_double,
    type_string,
    type_unit,
    type_none
} data_types;

typedef enum{
    node_program,
    node_function,
    node_block,
    node_val_var,
    node_asign,
    node_expression,
    node_return,
    node_if,
    node_while,
    node_lit,
    node_id,
    node_unar,
    node_binar,
    node_call
} node_type;

typedef enum
{
    add,
    subtract,
    multiply,
    devide,
    equal,
    negation,
    power,
    lesser,
    lesser_or_equal,
    grater,
    greater_or_equal,
    not_equal,
    none
}operator;
#endif