#include "ast.h"

actions create_table()
{ //                              +     -      *        /      !        (       )     $      i       <    <=     >    >=     ==     !=
    actions table[15][15] ={{reduce, reduce, shift,  shift,  shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // +
                            {reduce, reduce, shift,  shift,  shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // -
                            {reduce, reduce, reduce, reduce, shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // *
                            {reduce, reduce, reduce, reduce, shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // /
                            {reduce, reduce, reduce, reduce, shift,  shift, reduce, reduce, shift, reduce,reduce,reduce,reduce,reduce,reduce}, // !
                            {shift , shift , shift,  shift,  shift,  reduce,error,  error , shift, shift ,shift ,shift ,shift ,reduce,reduce}, // (
                            {reduce, reduce, reduce, reduce, error , error ,reduce, error , reduce,reduce,reduce,reduce,reduce,reduce,reduce}, // )
                            {shift,  shift,  shift,  shift,  shift,  shift, error,  prec_equal , shift, reduce,reduce,reduce,reduce,shift ,shift }, // $
                            {reduce, reduce, reduce, reduce, error,  error, reduce,  reduce, error, shift ,shift ,shift ,shift, reduce,reduce}, // i
                            {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce ,shift, error, error, error, error, reduce,reduce}, // <
                            {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, error, error, error, error, reduce,reduce}, // <=
                            {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, error, error, error, error, reduce,reduce}, // >
                            {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, error, error, error, error, reduce,reduce}, // >=
                            {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, shift, shift, shift, shift, error ,error }, // ==
                            {shift,  shift,  shift,  shift,  shift,  shift, reduce, reduce, shift, shift, shift, shift, shift, error ,error }  // !=
                            }; //needs a little fixing 
    return table;
}