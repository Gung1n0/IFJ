/*
 * implementation of a compiler for the language IFJ26
 * enums.h
 * collection of all shared enums
 * xzombot00 Tomáš Zomborský
 * xsturaf00 Filip Šturák
 */

#ifndef ENUMS_H
#define ENUMS_H

typedef enum {
  TYPE_INT,
  TYPE_DOUBLE,
  TYPE_STRING,
  TYPE_UNIT,
  TYPE_BOOLEAN,
  TYPE_ANY,
  TYPE_UNKNOWN
} data_types;

typedef enum { unvisited, in_progress, done } inference_state;

typedef enum {
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
  node_member,
  node_extension
} node_type;

typedef enum {
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
  OP_NONE,
  OP_NOT, // BOOLTHEN extention
  OP_AND, // BOOLTHEN extention
  OP_OR   // BOOLTHEN extention
} operator;

typedef enum {
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
typedef enum {
  TOKEN_IDENTIFIER,     // napr. 'moj_vysledok'
  TOKEN_INT_LITERAL,    // napr. 42
  TOKEN_DOUBLE_LITERAL, // napr. 3.14
  TOKEN_STRING_LITERAL, // napr. "ahoj"

  TOKEN_KW_DEF,
  TOKEN_KW_DO,
  TOKEN_KW_ELSE,
  TOKEN_KW_IF,
  TOKEN_KW_INT,
  TOKEN_KW_DOUBLE,
  TOKEN_KW_STRING,
  TOKEN_KW_IMPORT,
  TOKEN_KW_UNIT,
  TOKEN_KW_VAL,
  TOKEN_KW_VAR,
  TOKEN_KW_RETURN,
  TOKEN_KW_WHILE,
  TOKEN_KW_THEN,
  TOKEN_KW_EXTENSION, // EXTKW extention
  TOKEN_KW_BOOLEAN,   // BOOLTHEN extention
  TOKEN_KW_FOR,       // CONTROLL extention
  TOKEN_KW_TO,        // CONTROLL extention
  TOKEN_KW_UNTIL,     // CONTROLL extention
  TOKEN_KW_BY,        // CONTROLL extention
  TOKEN_KW_ARROW,     // CONTROLL extention

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

  TOKEN_TRUE,   // BOOLTHEN extention
  TOKEN_FALSE,  // BOOLTHEN extention
  TOKEN_OP_AND, // BOOLTHEN extention
  TOKEN_OP_OR,  // BOOLTHEN extention
  TOKEN_OP_NOT, // BOOLTHEN extention

  TOKEN_EOL,
  TOKEN_EOF,
  TOKEN_IND,
  TOKEN_DED
} TokenType;
typedef enum { LPAR, RPAR, none_b } bracket_type;

typedef enum { terminal, node_item } item_type;

typedef enum { L_TO_R, R_TO_L } asocitivity;

typedef enum {
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
} symbols;
typedef enum { shift, reduce, prec_equal, error } actions;

#endif