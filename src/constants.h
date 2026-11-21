#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "enums.h"
static const actions table[17][17] = {
    //  +           -           *           /           !           ( ) $ i < <=
    //  >           >=          ==          !=       &&    ||
    {reduce, reduce, shift, shift, shift, shift, reduce, reduce, shift, reduce,
     reduce, reduce, reduce, reduce, reduce, reduce, reduce}, // +
    {reduce, reduce, shift, shift, shift, shift, reduce, reduce, shift, reduce,
     reduce, reduce, reduce, reduce, reduce, reduce, reduce}, // -
    {reduce, reduce, reduce, reduce, shift, shift, reduce, reduce, shift,
     reduce, reduce, reduce, reduce, reduce, reduce, reduce, reduce}, // *
    {reduce, reduce, reduce, reduce, shift, shift, reduce, reduce, shift,
     reduce, reduce, reduce, reduce, reduce, reduce, reduce, reduce}, // /
    {reduce, reduce, reduce, reduce, shift, shift, reduce, reduce, shift,
     reduce, reduce, reduce, reduce, reduce, reduce, reduce, reduce}, // !
    {shift, shift, shift, shift, shift, shift, prec_equal, error, shift, shift,
     shift, shift, shift, shift, shift, shift, shift}, // (
    {reduce, reduce, reduce, reduce, error, error, reduce, error, reduce,
     reduce, reduce, reduce, reduce, reduce, reduce, reduce, reduce}, // )
    {shift, shift, shift, shift, shift, shift, error, prec_equal, shift, shift,
     shift, shift, shift, shift, shift, shift, shift}, // $
    {reduce, reduce, reduce, reduce, error, error, reduce, error, reduce,
     reduce, reduce, reduce, reduce, reduce, reduce, reduce, reduce}, // i
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, error,
     error, error, error, reduce, reduce, reduce, reduce}, // <
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, error,
     error, error, error, reduce, reduce, reduce, reduce}, // <=
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, error,
     error, error, error, reduce, reduce, reduce, reduce}, // >
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, error,
     error, error, error, reduce, reduce, reduce, reduce}, // >=
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, shift,
     shift, shift, shift, error, error, reduce, reduce}, // ==
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, shift,
     shift, shift, shift, error, error, reduce, reduce}, // !=
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, shift,
     shift, shift, shift, shift, shift, reduce, reduce}, // &&
    {shift, shift, shift, shift, shift, shift, reduce, reduce, shift, shift,
     shift, shift, shift, shift, shift, shift, shift} // ||
};
#endif // right asoc, if equal: shift, left opposite; ak mešia priorita, reduce