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

bool is_keyword(token *t)
{

    return t->type >= TOKEN_KW_DEF && t->type <= TOKEN_KW_STRING;
}

const char *token_name(TokenType type)
{
    switch (type)
    {
    case TOKEN_IDENTIFIER:
        return "identifier";
    case TOKEN_INT_LITERAL:
        return "integer literal";
    case TOKEN_DOUBLE_LITERAL:
        return "double literal";
    case TOKEN_STRING_LITERAL:
        return "string literal";
    case TOKEN_KW_DEF:
        return "'def'";
    case TOKEN_KW_DO:
        return "'do'";
    case TOKEN_KW_ELSE:
        return "'else'";
    case TOKEN_KW_IF:
        return "'if'";
    case TOKEN_KW_INT:
        return "'Int'";
    case TOKEN_KW_VAL:
        return "'val'";
    case TOKEN_KW_VAR:
        return "'var'";
    case TOKEN_KW_RETURN:
        return "'return'";
    case TOKEN_KW_IMPORT:
        return "'import'";
    case TOKEN_KW_UNIT:
        return "'Unit'";
    case TOKEN_KW_DOUBLE:
        return "'Double'";
    case TOKEN_KW_STRING:
        return "'String'";
    case TOKEN_OP_PLUS:
        return "'+'";
    case TOKEN_OP_MINUS:
        return "'-'";
    case TOKEN_OP_MUL:
        return "'*'";
    case TOKEN_OP_DIV:
        return "'/'";
    case TOKEN_OP_ASSIGN:
        return "'='";
    case TOKEN_OP_EQ:
        return "'=='";
    case TOKEN_OP_NEQ:
        return "'!='";
    case TOKEN_OP_LESS:
        return "'<'";
    case TOKEN_OP_MORE:
        return "'>'";
    /*case TOKEN_OP_LEQ: return "'<='";
    case TOKEN_OP_GEQ: return "'>='";*/
    case TOKEN_OP_UNDERSC:
        return "'_'";
    case TOKEN_LPAR:
        return "'('";
    case TOKEN_RPAR:
        return "')'";
    case TOKEN_COLON:
        return "':'";
    case TOKEN_COMMA:
        return "','";
    case TOKEN_DOT:
        return "'.'";
    case TOKEN_AT:
        return "'@'";
    case TOKEN_SEMICOLON:
        return "';'";
    case TOKEN_EOL:
        return "end of line";
    case TOKEN_EOF:
        return "end of file";
    case TOKEN_IND:
        return "indent";
    case TOKEN_DED:
        return "dedent";
    }
    return "token";
}

// temporary solution for testing without lexer
token get_token(TokenType type, char *data)
{
    token Token;
    Token.type = type;
    Token.data = data;
    return Token;
}

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
token *get_token_from_array(token_array *arr, TokenType type, char *data)
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
    snprintf(error_msg, sizeof(error_msg), "Syntax error, expected %s", data != NULL ? data : token_name(type));
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

void parse_prologue(token_array *arr)
{
    if (arr->size > 0)
    {
        token *curr = peek_token_array(arr, 0);
        // go trough tokens until i get prologue, or syntax error

        while (curr->type != TOKEN_KW_IMPORT)
        {
            if (arr->pos == arr->size - 1)
                print_error("expected prologue", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

            if (curr->type != TOKEN_EOL)
            {
                if (is_keyword(curr))
                    print_error("expected 'import'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
                else
                    print_error("expected prologue", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
            }
            else
            {
                skip_token(arr);
                curr = &arr->token_data[arr->pos];
            }
        }

        skip_token(arr);

        get_token_from_array(arr, TOKEN_IDENTIFIER, "ifj26");

        get_token_from_array(arr, TOKEN_DOT, NULL);

        get_token_from_array(arr, TOKEN_OP_MUL, NULL);

        get_token_from_array(arr, TOKEN_EOL, NULL);
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
    switch (curr->type)
    {
    case TOKEN_KW_INT:
        return type_int;
    case TOKEN_KW_UNIT:
        return type_unit;
    case TOKEN_KW_DOUBLE:
        return type_double;
    case TOKEN_KW_STRING:
        return type_string;
    default:
        return type_none;
    }
}

operator parse_operator(token *curr)
{
    switch (curr->type)
    {
    case TOKEN_OP_PLUS:
        return add;
    case TOKEN_OP_MINUS:
        return subtract;
    case TOKEN_OP_MUL:
        return multiply;
    case TOKEN_OP_DIV:
        return divide;
    case TOKEN_OP_EQ:
        return equal;
    case TOKEN_OP_NEQ:
        return not_equal;
    case TOKEN_OP_LESS:
        return lesser;
    case TOKEN_OP_MORE:
        return grater;
    /*case TOKEN_OP_LEQ:
        return lesser_or_equal;
    case TOKEN_OP_GEQ:
        return greater_or_equal;*/
    default:
        return none;
    }
}

node *parse_unar(token *curr, token_array *arr)
{
    // todo
    return NULL;
}

node *parse_binar(token *curr, token_array *arr)
{
    // todo
    return NULL;
}

node *parse_expr(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);

    if (curr->type == TOKEN_IDENTIFIER)
    {
        node *id = create_node(node_id, curr->row, curr->column);
        id->idk.ident.name = strduplicate(curr->data);
        skip_token(arr);
        return id;
    }
    else if (curr->type == TOKEN_INT_LITERAL)
    {
        node *lit = create_node(node_lit, curr->row, curr->column);
        lit->idk.lit.type = type_int;
        lit->idk.lit.i = curr->data ? atoi(curr->data) : 0;
        skip_token(arr);
        return lit;
    }
    else
    {
        print_error("Syntax error, expected expression", SYNTAX_ERROR, curr->row, curr->column);
    }
    return NULL;
}

node *parse_return(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);

    node *ret = create_node(node_return, curr->row, curr->column);

    skip_token(arr);
    curr = peek_token_array(arr, 0);

    if (curr->type == TOKEN_EOL || curr->type == TOKEN_DED || curr->type == TOKEN_SEMICOLON)
    {
        ret->idk.ret.value = NULL;
        return ret;
    }
    ret->idk.ret.value = parse_expr(arr);
    return ret;
}

node *parse_val_var(token_array *arr, token *curr)
{
    node *vl = create_node(node_val_var, curr->row, curr->column);
    if (curr->type != TOKEN_KW_VAL)
        vl->idk.val_var.changable = true;
    else
        vl->idk.val_var.changable = false;

    skip_token(arr);
    curr = peek_token_array(arr, 0);
    if (curr->data == NULL || curr->type != TOKEN_IDENTIFIER)
        print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);

    vl->idk.val_var.name = strduplicate(curr->data);

    skip_token(arr);
    curr = peek_token_array(arr, 0);

    if (curr->type == TOKEN_COLON)
    {
        get_token_from_array(arr, TOKEN_COLON, NULL);
        curr = &arr->token_data[arr->pos];

        if (parse_type(curr) == type_none)
            print_error("Syntax error, expected type", SYNTAX_ERROR, curr->row, curr->column);

        vl->idk.val_var.type = parse_type(curr);

        skip_token(arr);
        curr = peek_token_array(arr, 0);
    }
    else
        vl->idk.val_var.type = type_none;

    if (curr->type == TOKEN_OP_ASSIGN)
    {
        skip_token(arr);
        vl->idk.val_var.init = parse_expr(arr);
        return vl;
    }
    else
        print_error("Syntax error, expected '='", SYNTAX_ERROR, curr->row, curr->column);

    return NULL;
}

node *parse_function_body(token_array *arr)
{
    node *bl = create_node(node_block, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);
    token *curr = peek_token_array(arr, 0);
    bool indent = false;

    if (curr->type == TOKEN_IND)
    {
        indent = true;
        skip_token(arr);
        curr = peek_token_array(arr, 0);
    }

    if (curr->type == TOKEN_DED && indent)
        print_error("Syntax error, empty block", SYNTAX_ERROR, curr->row, curr->column);

    if (!indent)
    {
        node **temp = realloc(bl->idk.block.items, sizeof(node *) * (bl->idk.block.size + 1));
            if (temp == NULL)
                print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
            bl->idk.block.items = temp;
            bl->idk.block.size++;

            bl->idk.block.items[bl->idk.block.size - 1] = parse_expr(arr);
            curr = peek_token_array(arr, 0);
            
            return bl;
    }

    while ((curr->type != TOKEN_DED && indent) || (!indent && curr->type != TOKEN_EOL))
    {
        if (curr->type == TOKEN_KW_VAL || curr->type == TOKEN_KW_VAR)
        {
            node **temp = realloc(bl->idk.block.items, sizeof(node *) * (bl->idk.block.size + 1));
            if (temp == NULL)
                print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
            bl->idk.block.items = temp;
            bl->idk.block.size++;

            bl->idk.block.items[bl->idk.block.size - 1] = parse_val_var(arr, curr);
            curr = peek_token_array(arr, 0);
        }
        else if (curr->type == TOKEN_KW_RETURN)
        {
            node **temp = realloc(bl->idk.block.items, sizeof(node *) * (bl->idk.block.size + 1));
            if (temp == NULL)
                print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
            bl->idk.block.items = temp;
            bl->idk.block.size++;

            bl->idk.block.items[bl->idk.block.size - 1] = parse_return(arr);
            curr = peek_token_array(arr, 0);
        }
        else if (is_keyword(curr))
        {
            print_error("Syntax error, unexpected keyword", SYNTAX_ERROR, curr->row, curr->column);
        }
        else if (curr->type == TOKEN_EOF)
        {
            print_error("Syntax error, expected dedent", SYNTAX_ERROR, curr->row, curr->column);
        }
        else
        {
            print_error("Syntax error", SYNTAX_ERROR, curr->row, curr->column);
        }

        if (curr->type == TOKEN_DED)
            break;

        if (curr->type == TOKEN_SEMICOLON)
        {
            skip_token(arr);
            curr = peek_token_array(arr, 0);
            if (curr->type == TOKEN_EOL)
            {
                skip_token(arr);
                curr = peek_token_array(arr, 0);
            }
        }
        else
        {
            skip_token(arr);
            curr = peek_token_array(arr, 0);
        }
    }
    skip_token(arr);
    return bl;
}

bool check_bracket(token *curr)
{
    return curr->type != TOKEN_RPAR;
}

void parse_params(token_array *arr, node *n)
{
    token *curr = peek_token_array(arr, 0);

    while (check_bracket(curr))
    {
        if (curr->data != NULL)
        {
            if (curr->type == TOKEN_IDENTIFIER)
            {
                int row1 = curr->row;
                int collumn1 = curr->column;

                char *tempstr = strduplicate(curr->data);
                skip_token(arr);
                curr = peek_token_array(arr, 0);

                if (curr->type != TOKEN_COLON)
                    print_error("Syntax error, expected ':'", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

                skip_token(arr);
                curr = peek_token_array(arr, 0);

                if (parse_type(curr) == type_none)
                    print_error("Syntax error, expected type", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

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

                if (curr->type == TOKEN_COMMA)
                {
                    int comma_row = curr->row;
                    int comma_col = curr->column;
                    bool had_eol = false;

                    skip_token(arr);
                    curr = peek_token_array(arr, 0);
                    while (curr->type == TOKEN_EOL)
                    {
                        had_eol = true;
                        skip_token(arr);
                        curr = peek_token_array(arr, 0);
                    }

                    if (curr->type == TOKEN_RPAR && !had_eol)
                        print_error("Syntax error, unexpected ','", SYNTAX_ERROR, comma_row, comma_col);
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
    if (curr->type != TOKEN_IDENTIFIER || curr->data == NULL)
        print_error("Syntax error, expected identifier", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

    n->idk.function.name = strduplicate(curr->data);
    skip_token(arr);
    curr = &arr->token_data[arr->pos];

    get_token_from_array(arr, TOKEN_LPAR, NULL);

    curr = peek_token_array(arr, 0);
    if (curr->type == TOKEN_RPAR)
    {
        get_token_from_array(arr, TOKEN_RPAR, NULL);
    }
    else
    {
        parse_params(arr, n);
        get_token_from_array(arr, TOKEN_RPAR, NULL);
    }

    curr = peek_token_array(arr, 0);
    if (curr->type == TOKEN_COLON)
    {

        get_token_from_array(arr, TOKEN_COLON, NULL);

        curr = peek_token_array(arr, 0);

        if (parse_type(curr) == type_none)
            print_error("Syntax error, expected type", SYNTAX_ERROR, arr->token_data[arr->pos].row, arr->token_data[arr->pos].column);

        type = parse_type(curr);
        explicit = true;
        skip_token(arr);
    }
    else
        type = type_none;

    n->idk.function.explicit = explicit;
    n->idk.function.return_type = type;

    get_token_from_array(arr, TOKEN_OP_ASSIGN, NULL);

    curr = peek_token_array(arr, 0);
    if (curr->type == TOKEN_EOL && curr->data == NULL)
    {
        skip_token(arr);
        curr = peek_token_array(arr, 0);
    }

    n->idk.function.body = parse_function_body(arr);
    return;
}

// parses def function
node *parse_def(token_array *arr, token *curr)
{
    node *n = create_node(node_function, curr->row, curr->column);

    if (curr->type == TOKEN_KW_DEF)
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
    if (curr->type == TOKEN_AT)
    {
        skip_token(arr);
        curr = &arr->token_data[arr->pos];
        if (curr->type == TOKEN_IDENTIFIER && curr->data != NULL)
        {
            if (strcmp(curr->data, "main") == 0)
            {
                skip_token(arr);

                curr = peek_token_array(arr, 0);
                if (curr->type == TOKEN_EOL && curr->data == NULL)
                {
                    skip_token(arr);
                    curr = peek_token_array(arr, 0);
                }

                if (curr->type == TOKEN_KW_DEF)
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
    while (curr->type != TOKEN_EOF)
    {
        node *f = NULL;
        if (curr->type == TOKEN_KW_DEF)
        {
            f = parse_def(arr, curr);
        }
        else if (curr->type == TOKEN_AT)
        {
            f = parse_main(arr, curr);
        }
        else if (curr->type == TOKEN_EOL)
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