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

#endif