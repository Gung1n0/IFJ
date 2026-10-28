// Tomáš Badinský (xbadint00)
//

#ifndef CODEGEN_H
#define CODEGEN_H

typedef enum{
    type_int,
    type_double,
    type_string,
    type_bool,
    type_nil,
    type_array,
    type_var,
}data_types;

typedef struct{
    union{
        int i_value;
        double d_value;
        char *s_value;
        char *var_name;
    } value;
    data_types data_type;
}operands;

void prologue_gen();

void createframe_gen();

void add_gen(operands res, operands op1, operands op2);

void write_gen(operands op1);

#endif