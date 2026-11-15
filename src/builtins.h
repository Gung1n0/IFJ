/*
 * implementation of a compiler for the language IFJ26
 * bulitins.h
 * defines constants for builtin functions (such as print())
 * xzombot00 Tomáš Zomborský
 */

#ifndef BUILTINS_H
#define BUILTINS_H

#define BUILTIN_PARAM(param_name, param_type)                                  \
  {.name = (param_name), .data_type = (param_type)}

// print
#define PRINT_NAME "print"
#define PRINT_PARAM_COUNT 1
#define PRINT_PARAMS ((const sym_parameter[]){BUILTIN_PARAM("value", TYPE_ANY)})
#define PRINT_RETURN TYPE_UNIT

// readLine
#define READLINE_NAME "readLine"
#define READLINE_PARAM_COUNT 0
#define READLINE_PARAMS NULL
#define READLINE_RETURN TYPE_STRING

// readInt
#define READINT_NAME "readInt"
#define READINT_PARAM_COUNT 0
#define READINT_PARAMS NULL
#define READINT_RETURN TYPE_INT

// readDouble
#define READDOUBLE_NAME "readDouble"
#define READDOUBLE_PARAM_COUNT 0
#define READDOUBLE_PARAMS NULL
#define READDOUBLE_RETURN TYPE_DOUBLE

// toStr
#define TOSTR_NAME "toStr"
#define TOSTR_PARAM_COUNT 1
#define TOSTR_PARAMS ((const sym_parameter[]){BUILTIN_PARAM("value", TYPE_ANY)})
#define TOSTR_RETURN TYPE_STRING

// toInt
#define TOINT_NAME "toInt"
#define TOINT_PARAM_COUNT 1
#define TOINT_PARAMS                                                           \
  ((const sym_parameter[]){BUILTIN_PARAM("value", TYPE_DOUBLE)})
#define TOINT_RETURN TYPE_INT

// strLength
#define STRLENGTH_NAME "strLength"
#define STRLENGTH_PARAM_COUNT 1
#define STRLENGTH_PARAMS                                                       \
  ((const sym_parameter[]){BUILTIN_PARAM("value", TYPE_STRING)})
#define STRLENGTH_RETURN TYPE_INT

// subStr
#define SUBSTR_NAME "subStr"
#define SUBSTR_PARAM_COUNT 3
#define SUBSTR_PARAMS                                                          \
  ((const sym_parameter[]){BUILTIN_PARAM("value", TYPE_STRING),                \
                           BUILTIN_PARAM("start", TYPE_INT),                   \
                           BUILTIN_PARAM("end", TYPE_INT)})
#define SUBSTR_RETURN TYPE_STRING

// ord
#define ORD_NAME "ord"
#define ORD_PARAM_COUNT 2
#define ORD_PARAMS                                                             \
  ((const sym_parameter[]){BUILTIN_PARAM("value", TYPE_STRING),                \
                           BUILTIN_PARAM("index", TYPE_INT)})
#define ORD_RETURN TYPE_INT

// chr
#define CHR_NAME "chr"
#define CHR_PARAM_COUNT 1
#define CHR_PARAMS ((const sym_parameter[]){BUILTIN_PARAM("code", TYPE_INT)})
#define CHR_RETURN TYPE_STRING

#endif
