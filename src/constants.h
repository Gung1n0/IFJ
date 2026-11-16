#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "Enums.h"

typedef enum{
 LEXICAL_ERROR = 1,
 SYNTAX_ERROR = 2,
 UNDEFINED = 3,
UNAUTHORIZED = 4,
INVALID_ARGUMENT = 5,
INVALID_TYPE = 6,
IMMUTABLE_WRITE = 7,
SEMANTIC_ERROR = 10,
INTERNAL_ERROR = 99
} error_code;

//                                  +     -      *        /      !        (       )      $      i       <      <=      >    >=      ==     !=
const actions table[15][15] = {{reduce, reduce, shift,  shift,  shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // +
                               {reduce, reduce, shift,  shift,  shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // -
                               {reduce, reduce, reduce, reduce, shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // *
                               {reduce, reduce, reduce, reduce, shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // /
                               {reduce, reduce, reduce, reduce, shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // !
                               {shift,  shift,  shift,  shift,  shift,  shift, prec_equal, error,  shift, shift, shift, shift, shift, shift, shift},  // (
                               {reduce, reduce, reduce, reduce, error , error ,reduce, error , reduce,reduce,reduce,reduce,reduce,reduce,reduce}, // )
                               {shift,  shift,  shift,  shift,  shift,  shift, error,  prec_equal, shift, shift, shift, shift, shift, shift, shift},  // $
                               {reduce, reduce, reduce, reduce, error,  error, reduce, error , reduce,reduce,reduce,reduce,reduce,reduce,reduce}, // i
                               {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce ,shift, error, error, error, error, reduce,reduce}, // <
                               {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, error, error, error, error, reduce,reduce}, // <=
                               {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, error, error, error, error, reduce,reduce}, // >
                               {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, error, error, error, error, reduce,reduce}, // >=
                               {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, shift, shift, shift, shift, error ,error }, // ==
                               {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, shift, shift, shift, shift, error ,error }  // !=
                         };
#endif