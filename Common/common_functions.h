/*
 * implementation of a compiler for the language IFJ26
 * common_functions.h
 * collection of shared common functions
 * xzombot00 Tomáš Zomborský
 * xsturaf00 Filip Šturák
 */

#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include "../Parser/ast.h"
#include "enums.h"
#include <stdlib.h>
#include <string.h>

// TODO: switch error messages with actual messages
#define ERR_ARGUMENTS_MESSAGE "argument error"
#define ERR_IMMUTABLE_MESSAGE "immutable error"
#define ERR_INTERNAL_MESSAGE "internal error"
#define ERR_LEXICAL_MESSAGE "lexical error"
#define ERR_NONE_MESSAGE "no error"
#define ERR_OTHER_SEMANTIC_MESSAGE "other semantic error"
#define ERR_REDEFINITION_MESSAGE "redifinition error"
#define ERR_SYNTAX_MESSAGE "syntax error"
#define ERR_TYPES_MESSAGE "types error"
#define ERR_UNDEFINED_MESSAGE "undefined error"
#define EMPTY_ERROR_MESSAGE ""

/// @brief Local implementation of strdup; allocates a new string and copies the
/// contents to it
/// @param s String to be duplicated
/// @return Returns duplicated string, if allocation was successful, else NULL
static inline char *strduplicate(const char *s) {
  char *p = (char *)malloc(strlen(s) + 1);
  if (p != NULL) {
    strcpy(p, s);
  } else
    return NULL;
  return p;
}

/// @brief Converts error code into a string message
/// @param err Error code, to be converted
/// @return String message
static inline const char *error_to_message(error_code err) {
  switch (err) {
  case ERR_ARGUMENTS:
    return ERR_ARGUMENTS_MESSAGE;
  case ERR_IMMUTABLE:
    return ERR_IMMUTABLE_MESSAGE;
  case ERR_INTERNAL:
    return ERR_INTERNAL_MESSAGE;
  case ERR_LEXICAL:
    return ERR_LEXICAL_MESSAGE;
  case ERR_NONE:
    return ERR_NONE_MESSAGE;
  case ERR_OTHER_SEMANTIC:
    return ERR_OTHER_SEMANTIC_MESSAGE;
  case ERR_REDEFINITION:
    return ERR_REDEFINITION_MESSAGE;
  case ERR_SYNTAX:
    return ERR_SYNTAX_MESSAGE;
  case ERR_TYPES:
    return ERR_TYPES_MESSAGE;
  case ERR_UNDEFINED:
    return ERR_UNDEFINED_MESSAGE;
  default:
    return EMPTY_ERROR_MESSAGE;
  }
}

/// @brief Prints error message, points to the fault node and exits with a
/// error_code
/// @param err Error code
/// @param n Node to report on
static inline void sem_error(error_code err, node *n) {
  const char *message = error_to_message(err);
  fprintf(stderr, "%s at row %i column %i\n", message, n->row, n->column);
  exit(err);
}

#endif