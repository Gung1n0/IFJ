// Tomáš Badinský (xbadint00)
#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"

#include "ast.h"

// print string literal with escape sequences based on IFJcode26 specification
void print_literal(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {

        if (str[i] == '#')
        {
            printf("\\035");
        }
        else if (str[i] == '\\')
        {
            printf("\\092");
        }
        else if (str[i] >= 0 && str[i] <= 32)
        {
            printf("\\%03d", str[i]);
        }
        else
        {
            printf("%c", str[i]);
        }
    }
}

// print based on data type of operand
/*
void print_operand(operands op) {
    if(op.data_type == type_var){
        printf(" %s", op.value.var_name);
    }
    else if(op.data_type == type_int){
        printf(" int@%d", op.value.i_value);
    }
    else if(op.data_type == type_double){
        printf(" float@%a", op.value.d_value);
    }
    else if(op.data_type == type_string){
        printf(" string@");
        print_literal(op.value.s_value);
    }
    else if(op.data_type == type_bool){
        printf(" bool@%s", op.value.s_value);
    }
    else if(op.data_type == type_nil){
        printf(" nil@nil");
    }
    else{
        printf("Unsupported data type\n");
    }
}
*/

typedef struct
{
    node *program;

} gen_context;

// internal error handler
void internal_err(node *where, char *msg)
{
    if (where != NULL)
    {
        error_exit(INTERNAL_ERROR, "%s, %d %d", msg, where->row, where->column);
    }
    error_exit(INTERNAL_ERROR, "%s", msg);
}

// find main function in node
node *find_main(node *program)
{
    for (int i = 0; i < program->as.program.size; i++)
    {
        node *function = program->as.program.items[i];
        if (function != NULL && function->type == node_function && function->as.function.is_main)
        {
            return function;
        }
    }
    return NULL;
}

/*
void builtins_gen(void){

}
*/

// generating code from AST
void code_generate(struct node *program)
{

    node *main_function = find_main(program);
    if (main_function == NULL)
    {
        internal_err(program, "@main not found");
    }

    // IFJ header
    printf(".IFJcode26\n");

    // builtins_gen();
}