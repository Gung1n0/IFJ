#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "enums.h"
const actions table[15][15] = {
    {reduce, reduce, shift, shift, shift, shift, reduce, reduce, shift, reduce,
     reduce, reduce, reduce, reduce, reduce}, // +
    {reduce, reduce, shift, shift, shift, shift, reduce, reduce, shift, reduce,
     reduce, reduce, reduce, reduce, reduce}, // -
    {reduce, reduce, reduce, reduce, shift, shift, reduce, reduce, shift,
     reduce, reduce, reduce, reduce, reduce, reduce}, // *
    {reduce, reduce, reduce, reduce, shift, shift, reduce, reduce, shift,
     reduce, reduce, reduce, reduce, reduce, reduce}, // /
    {reduce, reduce, reduce, reduce, shift, shift, reduce, reduce, shift,
     reduce, reduce, reduce, reduce, reduce, reduce}, // !
    {shift, shift, shift, shift, shift, shift, prec_equal, error, shift, shift,
     shift, shift, shift, shift, shift}, // (
    {reduce, reduce, reduce, reduce, error, error, reduce, error, reduce,
     reduce, reduce, reduce, reduce, reduce, reduce}, // )
    {shift, shift, shift, shift, shift, shift, error, prec_equal, shift, shift,
     shift, shift, shift, shift, shift}, // $
    {reduce, reduce, reduce, reduce, error, error, reduce, error, reduce,
     reduce, reduce, reduce, reduce, reduce, reduce}, // i
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, error,
     error, error, error, reduce, reduce}, // <
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, error,
     error, error, error, reduce, reduce}, // <=
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, error,
     error, error, error, reduce, reduce}, // >
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, error,
     error, error, error, reduce, reduce}, // >=
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, shift,
     shift, shift, shift, error, error}, // ==
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, shift,
     shift, shift, shift, error, error} // !=
};
#endif