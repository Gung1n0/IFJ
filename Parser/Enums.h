/*typedef enum
{
    opening,
    closing
} token_bracket_variant;

typedef enum
{
    plus,
    minus,
    divide,
    multiply
} token_arithmetic_operator;

typedef enum
{
    greater,       //>
    lesser,        // <
    greater_equal, // >=
    lesser_equal,  // <=
    equal          // =
} token_relation_operator;*/
//i dont think its needed yet im gonna keep it, justin case 

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

#endif