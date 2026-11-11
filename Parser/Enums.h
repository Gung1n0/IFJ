#ifndef ENUMS_H
#define ENUMS_H
typedef enum
{
    TOKEN_IDENTIFIER,     // napr. 'moj_vysledok'
    TOKEN_INT_LITERAL,    // napr. 42
    TOKEN_DOUBLE_LITERAL, // napr. 3.14
    TOKEN_STRING_LITERAL, // napr. "ahoj"

    TOKEN_KW_DEF,
    TOKEN_KW_DO,
    TOKEN_KW_ELSE,
    TOKEN_KW_IF,
    TOKEN_KW_INT,
    TOKEN_KW_MAIN, // new
    TOKEN_KW_IMPORT,
    TOKEN_KW_UNIT,
    TOKEN_KW_VAL,
    TOKEN_KW_VAR,
    TOKEN_KW_RETURN,
    TOKEN_KW_WHILE,
    TOKEN_KW_THEN,

    TOKEN_OP_PLUS,    // +
    TOKEN_OP_MINUS,   // -
    TOKEN_OP_MUL,     // *
    TOKEN_OP_DIV,     // /
    TOKEN_OP_ASSIGN,  // =
    TOKEN_OP_EQ,      // ==
    TOKEN_OP_NEQ,     // !=
    TOKEN_OP_LESS,    // <
    TOKEN_OP_MORE,    // >
    TOKEN_OP_LESS_EQ, // <=
    TOKEN_OP_MORE_EQ, // >=
    TOKEN_OP_UNDERSC, // _

    TOKEN_LPAR,  // (
    TOKEN_RPAR,  // )
    TOKEN_COLON, // :
    TOKEN_COMMA, // ,
    TOKEN_DOT,   // .
    TOKEN_AT,    // @
    TOKEN_SEMICOLON,

    TOKEN_EOL,
    TOKEN_EOF,
    TOKEN_IND,
    TOKEN_DED
} TokenType;

typedef enum
{
    type_int,
    type_double,
    type_string,
    type_unit,
    type_none
} data_types;

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
    add,
    subtract,
    multiply,
    divide,
    equal,
    negation,
    // power,
    lesser,
    lesser_or_equal,
    grater,
    greater_or_equal,
    not_equal,
    none
} operator;

typedef enum
{
    LPAR,
    RPAR,
    none_b
} bracket_type;

typedef enum
{
    bracket,
    op,
    n
}item_type;

typedef enum
{
    L_TO_R,
    R_TO_L
}asocitivity;


typedef enum{
    prec_plus, 
    prec_minus, 
    prec_multiply, 
    prec_divide, 
    prec_negate, 
    prec_lpar, 
    prec_rpar, 
    prec_end, 
    prec_i, 
    prec_less, 
    prec_less_equal, 
    prec_greater, 
    prec_greater_equal, 
    prec_is_eqal, 
    prec_is_not_equal
}symbols;
typedef enum{
    shift, reduce, prec_equal, error
}actions;
#endif