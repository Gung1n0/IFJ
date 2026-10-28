// Tomáš Badinský (xbadint00)
#include <stdio.h>
#include "codegen.h"

// print based on data type of operand
void print_operand(operands op) {
    if(op.data_type == type_var){
        printf("%s\n", op.value.var_name);
    }
    else if(op.data_type == type_int){
        printf("int@%d\n", op.value.i_value);
    }
    else if(op.data_type == type_double){
        printf("float@%a\n", op.value.d_value);
    }
    else if(op.data_type == type_string){
        printf("string@");
        print_literal(op.value.s_value);
        printf("\n");
    }
    else if(op.data_type == type_bool){
        printf("bool@%s\n", op.value.s_value);
    }
    else if(op.data_type == type_nil){
        printf("nil@nil\n");
    }
    else{
        printf("Unsupported data type\n");
    }
}

// print string literal with escape sequences based on IFJcode26 specification
void print_literal(char* str){
    for(int i = 0; str[i]!= '\0'; i++){

        if(str[i] == '#'){
            printf("\\035");
        }
        else if(str[i] == '\\'){
            printf("\\092");
        }
        else if(str[i] >= 0 && str[i] <= 32){
            printf("\\%03d", str[i]);
        }
        else{
            printf("%c", str[i]);
        }
    }

}


void builtins_gen(void){
    
}

void code_gen(){
    printf(".IFJcode26\n");

    builtins_gen();

}