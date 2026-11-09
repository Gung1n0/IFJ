#ifndef ENUMS_H
#define ENUMS_H

typedef enum
{
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_BOOLEAN,
    TYPE_ANY,
    TYPE_UNKNOWN
} data_types;

typedef enum
{
    unvisited,
    in_progress,
    done
} inference_state;

typedef enum
{
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
    node_call,
} node_type;

typedef enum
{
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_EQUAL,
    OP_NEGATE,
    // OP_POWER,
    OP_LESSER,
    OP_LESSER_OR_EQUAL,
    OP_GREATER,
    OP_GREATER_OR_EQUAL,
    OP_NOT_EQUAL,
    OP_NONE
} operator;

typedef enum
{
    ERR_NONE = 0,
    ERR_LEXICAL = 1,
    ERR_SYNTAX = 2,
    ERR_UNDEFINED = 3,
    ERR_REDEFINITION = 4,
    ERR_ARGUMENTS = 5,
    ERR_TYPES = 6,
    ERR_IMMUTABLE = 7,
    ERR_OTHER_SEMANTIC = 10,
    ERR_INTERNAL = 99
} error_code;
#endif