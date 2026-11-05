#ifndef ENUMS_H
#define ENUMS_H

typedef enum
{
    type_int,
    type_double,
    type_string,
    type_unit,
    type_boolean,
    type_any,
    type_unknown
} data_types;

typedef enum
{
    unvisited,
    in_progress,
    done
} inference_state;

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