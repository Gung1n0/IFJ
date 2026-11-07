#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

// duplicate string
char *strduplicate(const char *s)
{
    char *p = malloc(strlen(s) + 1);
    if (p != NULL)
    {
        strcpy(p, s);
    }
    return p;
}

// temporary solution for testing without lexer
token get_token(token_type type, char *data)
{
    token Token;
    Token.type = type;
    Token.data = data;
    return Token;
}
//, posun, kontrola a spotrebovanie očakávaného tokenu.

// initialize token array
token_array construct_token_array(int size)
{
    token_array t;
    t.token_data = malloc(size * sizeof(token));
    if (t.token_data != NULL)
    {
        t.size = size;
    }
    else
    {
        t.size = 0;
    }
    return t;
}

// destroy token array
void destruct_token_array(token_array *t)
{
    free(t->token_data);
    t->token_data = NULL;
}

// initialize token array
void init_token_array(token_array *arr)
{
    arr->size = 0;
    arr->token_data = NULL;
    arr->pos = 0;
}

// enlarge array of tokens by one
void enlarge_token_array_one(token_array *arr)
{
    if (arr == NULL)
        return;

    token *tmp = realloc(arr->token_data, (arr->size + 1) * sizeof(token));
    if (tmp == NULL)
        print_error("realloc failed", INTERNAL_ERROR, 0, 0);
    arr->token_data = tmp;
    arr->size++;
    return;
}

// destroy token
void destruct_token(token *t)
{
    free(t->data);
    t->data = NULL;
}

// go to next token
void skip_token(token_array *arr)
{
    if (arr != NULL && arr->pos < arr->size - 1)
        arr->pos++;
}

// checks and gets current token from array
token *get_token_from_array(token_array *arr, token_type type, char *data)
{
    token *curr = &arr->token_data[arr->pos];

    bool data_matches = true;
    if (data != NULL)
    {
        if (curr->data == NULL || strcmp(curr->data, data) != 0)
        {
            data_matches = false;
        }
    }

    if (curr->type == type && data_matches)
    {
        skip_token(arr);
        return curr;
    }

    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "Syntax error, expected %s", arr->token_data[arr->pos].data);
    print_error(error_msg, SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
    return NULL; // Not needed but gcc throws warning without this
}

// peek at first token
token *peek_token_array(token_array *arr, int offset)
{
    if (arr->size > 0 && arr->pos + offset < arr->size)
    {
        return &arr->token_data[offset + arr->pos];
    }
    print_error("syntax error", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
    return NULL; // Not needed but gcc throws warning without this
}

// works only when all files end with EOF, needs to be checked with ado
void parse_prologue(token_array *arr)
{
    if (arr->size > 0)
    {
        token *curr = peek_token_array(arr, 0);
        // go trough tokens until i get prologue, or syntax error

        while (curr->type != token_keyword)
        {
            if (arr->pos == arr->size - 1)
                print_error("expected prologue", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

            if (curr->type != token_newline)
                print_error("expected prologue", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
            else
            {
                skip_token(arr);
                curr = &arr->token_data[arr->pos];
            }
        }

        if (curr->data == NULL || strcmp(curr->data, "import") != 0)
            print_error("expected 'import'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

        skip_token(arr);

        get_token_from_array(arr, token_identifier, "ifj26");

        get_token_from_array(arr, token_specialchar, ".");

        get_token_from_array(arr, token_specialchar, "*");

        get_token_from_array(arr, token_newline, NULL);
        return;
    }
    print_error("expected prologue", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
}

node *create_node(node_type type, int row, int column)
{
    node *temp = calloc(1, sizeof(node));
    if (temp == NULL)
        print_error("Malloc failed", INTERNAL_ERROR, 0, 0);

    temp->type = type;
    temp->row = row;
    temp->column = column;

    return temp;
}

void free_node(node *n)
{
    if (n == NULL)
        return;
    switch (n->type)
    {
    case node_program:
        if (n->idk.program.items)
        {
            for (int i = 0; i < n->idk.program.size; i++)
            {
                free_node(n->idk.program.items[i]);
            }
            free(n->idk.program.items);
        }
        break;
    case node_function:
        free(n->idk.function.name);
        free_node(n->idk.function.body);
        for (int i = 0; i < n->idk.function.param_count; i++)
            free(n->idk.function.params[i].name);
        free(n->idk.function.params);
        break;
    case node_block:
        for (int i = 0; i < n->idk.block.size; i++)
        {
            free_node(n->idk.block.items[i]);
        }
        free(n->idk.block.items);
        break;
    case node_val_var:
        free(n->idk.val_var.name);
        free_node(n->idk.val_var.init);
        break;
    case node_asign:
        free(n->idk.asign.name);
        free_node(n->idk.asign.init);
        break;

    case node_expression:
        free_node(n->idk.expression.expression);
        break;

    case node_return:
        free_node(n->idk.ret.value);
        break;

    case node_if:
        free_node(n->idk.if_n.condition);
        free_node(n->idk.if_n.then);
        free_node(n->idk.if_n.else_n);
        break;

    case node_while:
        free_node(n->idk.while_n.condition);
        free_node(n->idk.while_n.body);
        break;

    case node_lit:
        if (n->idk.lit.type == type_string)
            free(n->idk.lit.s);
        break;
    case node_id:
        free(n->idk.ident.name);
        break;

    case node_unar:
        free_node(n->idk.unar.operand);
        break;

    case node_binar:
        free_node(n->idk.binar.left);
        free_node(n->idk.binar.right);
        break;

    case node_call:
        free(n->idk.call.name);
        if (n->idk.call.items)
        {
            for (int i = 0; i < n->idk.call.size; i++)
            {
                free_node(n->idk.call.items[i]);
            }
            free(n->idk.call.items);
        }
        break;
    }
    free(n);
}

data_types parse_type(token *curr)
{
    if (curr->data == NULL)
        return type_none;

    if (strcmp(curr->data, "Int") == 0)
        return type_int;
    else if (strcmp(curr->data, "Unit") == 0)
        return type_unit;
    else if (strcmp(curr->data, "Double") == 0)
        return type_double;
    else if (strcmp(curr->data, "String") == 0)
        return type_string;
    else
        return type_none;
}

node *parse_function_body(token_array *arr)
{
    node *bl = create_node(node_block, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
    token *curr = peek_token_array(arr, 0);

    if (curr->data != NULL || curr->type != token_indent)
        print_error("Syntax error, expected indent", SYNTAX_ERROR, curr->row, curr->column);

    while (curr->type != token_dedent)
    {
        data_types type = parse_type(curr);
        switch (type)
        {
        case token_keyword:
        {
            if (curr->data == NULL)
                print_error("Syntax error, expected keyword", SYNTAX_ERROR, curr->row, curr->column);
            // val or var
            if (strcmp(curr->data, "val") == 0 || strcmp(curr->data, "var") == 0)
            {
                node *vl = create_node(node_val_var, curr->row, curr->column);
                if (strcmp(curr->data, "val") == 0)
                    vl->idk.val_var.changable = true;
                else
                    vl->idk.val_var.changable = false;

                skip_token(arr);
                curr = peek_token_array(arr, 0);
                if (curr->data == NULL || parse_type(curr) != token_identifier)
                    print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);

                vl->idk.val_var.name = curr->data;
                skip_token(arr);
                curr = peek_token_array(arr, 0);

                if (parse_type(curr) == token_newline)
                {
                    vl->idk.val_var.init = NULL;
                    vl->idk.val_var.type = type_none;
                    skip_token(arr);
                    break;
                }

                if (curr->data == NULL)
                    print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);
                if (parse_type(curr) == token_relation_operator && strcmp(curr->data, "=") == 0)
                {
                    switch (parse_type(curr))
                    { // if var = unar a
                    case token_arithmetic_operator:
                    {
                        if (curr->data == NULL)
                            print_error("Syntax error, expected '-'", SYNTAX_ERROR, curr->row, curr->column);
                        node *un = create_node(node_unar, curr->row, curr->column);
                        un->idk.unar.op = subtract;

                        skip_token(arr);
                        curr = peek_token_array(arr, 0);

                        if (parse_type(curr) != token_identifier || curr->data == NULL)
                            print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);

                        node *id = create_node(node_id, curr->row, curr->column);
                        id->idk.ident.name = strduplicate(curr->data);
                        un->idk.unar.operand = id;
                        vl->idk.val_var.init = un;

                        node *temp = realloc(bl->idk.block.items, sizeof(node) * bl->idk.block.size + 1);
                        if (temp == NULL)
                            print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
                        bl->idk.block.items = temp;
                        bl->idk.block.size++;

                        bl->idk.block.items[bl->idk.block.size - 1] = vl;

                        skip_token(arr);
                        curr = peek_token_array(arr, 0);
                        break;
                    } // if var = a
                    case token_identifier:
                    {
                        if (parse_type(curr) != token_identifier || curr->data == NULL)
                            print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);
                        node *id = create_node(node_id, curr->row, curr->column);

                        id->idk.ident.name = strduplicate(curr->data);

                        skip_token(arr);
                        curr = peek_token_array(arr, 0);

                        // if var = a
                        if (parse_type(curr) == token_newline && curr->data == NULL)
                        {
                            node *temp = realloc(bl->idk.block.items, sizeof(node) * bl->idk.block.size + 1);
                            if (temp == NULL)
                                print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
                            bl->idk.block.items = temp;
                            bl->idk.block.size++;

                            bl->idk.block.items[bl->idk.block.size - 1] = id;

                            skip_token(arr);
                            curr = peek_token_array(arr, 0);
                            break;
                        }
                        while (parse_type(curr) != token_newline)
                        {
                            //todo if infinite binars
                        }
                            break;
                    }
                    default:
                        break;
                    }
                }
            }
        }
        }
    }
    return bl;
}

bool check_bracket(token *curr)
{
    if (curr->data != NULL)
    {
        if (!(curr->type == token_bracket && strcmp(curr->data, ")") == 0))
            return true;
        else
            return false;
    }
    return true;
}

void parse_params(token_array *arr, node *n)
{
    token *curr = peek_token_array(arr, 0);

    while (check_bracket(curr))
    {
        if (curr->data != NULL)
        {
            if (curr->type == token_identifier)
            {
                int row1 = curr->row;
                int collumn1 = curr->column;

                char *tempstr = strduplicate(curr->data);
                skip_token(arr);
                curr = peek_token_array(arr, 0);
                if (curr->data == NULL)
                    print_error("Syntax error, expected ':'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

                if (curr->type != token_specialchar || strcmp(curr->data, ":") != 0)
                    print_error("Syntax error, expected ':'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

                skip_token(arr);
                curr = peek_token_array(arr, 0);

                if (curr->data == NULL)
                    print_error("Syntax error, expected keyword", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

                if ((curr->type != token_keyword || parse_type(curr) == type_none))
                    print_error("Syntax error, expected keyword", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

                param *temp = realloc(n->idk.function.params, sizeof(param) * (n->idk.function.param_count + 1));
                if (temp == NULL)
                    print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
                n->idk.function.params = temp;
                n->idk.function.param_count++;

                n->idk.function.params[n->idk.function.param_count - 1].name = tempstr;
                n->idk.function.params[n->idk.function.param_count - 1].types = parse_type(curr);
                n->idk.function.params[n->idk.function.param_count - 1].row = row1;
                n->idk.function.params[n->idk.function.param_count - 1].column = collumn1;

                skip_token(arr);
                curr = peek_token_array(arr, 0);

                if (curr->data == NULL)
                    print_error("Syntax error, expected ')'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

                if (strcmp(curr->data, ",") == 0)
                {
                    if (peek_token_array(arr, 1)->data == NULL)
                        print_error("Syntax error, expected ')'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

                    if (strcmp(peek_token_array(arr, 1)->data, ")") == 0)
                        print_error("Syntax error, unexpected ','", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

                    skip_token(arr);
                    curr = peek_token_array(arr, 0);
                }
                else
                {
                    if (check_bracket(curr))
                        print_error("Syntax error, expected ',' or ')'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
                }
            }
            else
                print_error("Syntax error, expected identifier", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
        }
        else
            print_error("Syntax error, expected identifier", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
    }
}

// parses head of a function: (n: Int):Int =
void parse_function_header(token_array *arr, node *n)
{
    data_types type;
    bool explicit = false;

    token *curr = peek_token_array(arr, 0);
    if (curr->type != token_identifier || curr->data == NULL)
        print_error("Syntax error, expected identifier", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

    n->idk.function.name = strduplicate(curr->data);
    skip_token(arr);
    curr = &arr->token_data[arr->pos];

    get_token_from_array(arr, token_bracket, "(");

    curr = peek_token_array(arr, 0);
    if (curr->type == token_bracket)
    {
        get_token_from_array(arr, token_bracket, ")");
    }
    else
    {
        parse_params(arr, n);
        get_token_from_array(arr, token_bracket, ")");
    }

    curr = peek_token_array(arr, 0);
    if (curr->type == token_specialchar && curr->data != NULL && strcmp(curr->data, ":") == 0)
    {

        get_token_from_array(arr, token_specialchar, ":");

        curr = peek_token_array(arr, 0);

        if (curr->data == NULL)
            print_error("Syntax error, expected identifier", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

        if ((curr->type != token_keyword || parse_type(curr) == type_none))
            print_error("Syntax error, expected identifier", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

        type = parse_type(curr);
        explicit = true;
        skip_token(arr);
    }
    else
        type = type_none;

    n->idk.function.explicit = explicit;
    n->idk.function.return_type = type;

    get_token_from_array(arr, token_relation_operator, "=");

    curr = peek_token_array(arr, 0);
    if (curr->type == token_newline && curr->data == NULL)
    {
        skip_token(arr);
        curr = peek_token_array(arr, 0);
    }

    parse_function_body(arr);
    return;
}

// parses def function
node *parse_def(token_array *arr, token *curr)
{
    if (curr->data == NULL)
        print_error("Syntax error", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

    node *n = create_node(node_function, curr->row, curr->column);

    if (strcmp(curr->data, "def") == 0)
    {
        skip_token(arr);
        parse_function_header(arr, n);
    }
    else
        print_error("Syntax error", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
    n->idk.function.is_main = false;
    return n;
}

// parses @main def function
node *parse_main(token_array *arr, token *curr)
{
    if (strcmp(curr->data, "@") == 0)
    {
        skip_token(arr);
        curr = &arr->token_data[arr->pos];
        if (curr->type == token_identifier && curr->data != NULL)
        {
            if (strcmp(curr->data, "main") == 0)
            {
                skip_token(arr);

                curr = peek_token_array(arr, 0);
                if (curr->type == token_newline && curr->data == NULL)
                {
                    skip_token(arr);
                    curr = peek_token_array(arr, 0);
                }

                if (curr->type == token_keyword && curr->data != NULL)
                {
                    node *n = parse_def(arr, curr);
                    n->idk.function.is_main = true;
                    return n;
                }
                else
                    print_error("Syntax error", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
            }
            else
                print_error("Syntax error, expected 'main'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
        }
        else
            print_error("Syntax error, expected 'main'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
    }
    else
        print_error("Syntax error, expected '@'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
    return NULL;
}

//
node *parse_head_funtion(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);
    node *np = create_node(node_program, 0, 0);
    while (curr->type != token_eof)
    {
        node *f = NULL;
        if (curr->type == token_keyword && curr->data != NULL)
        {
            f = parse_def(arr, curr);
        }
        else if (curr->type == token_specialchar && curr->data != NULL)
        {
            f = parse_main(arr, curr);
        }
        else if (curr->type == token_newline)
            skip_token(arr);
        else
            print_error("Syntax error", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

        curr = &arr->token_data[arr->pos];
        if (f == NULL)
            continue;

        node **temp = realloc(np->idk.program.items, sizeof(node *) * (np->idk.program.size + 1));
        if (temp == NULL)
            print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
        np->idk.program.items = temp;
        np->idk.program.size++;
        np->idk.program.items[np->idk.program.size - 1] = f;
    }
    return np;
}

int main(int argc, char const *argv[])
{
    printf("%i %s dočasne aby gcc nehadzal warrning \n", argc, argv[0]);
    // parse_prologue(arr);
    // parse_function(arr);
    return 0;
}
