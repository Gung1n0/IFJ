#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

node *parse_while(token_array *arr);
node *parse_if(token_array *arr);

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
    return t->type == TOKEN_KW_DEF ||
           t->type == TOKEN_KW_DO ||
           t->type == TOKEN_KW_ELSE ||
           t->type == TOKEN_KW_IF ||
           t->type == TOKEN_KW_INT ||
           t->type == TOKEN_KW_IMPORT ||
           t->type == TOKEN_KW_UNIT ||
           t->type == TOKEN_KW_VAL ||
           t->type == TOKEN_KW_VAR ||
           t->type == TOKEN_KW_RETURN ||
           t->type == TOKEN_KW_WHILE ||
           t->type == TOKEN_KW_THEN ||
           t->type == TOKEN_KW_DOUBLE ||
           t->type == TOKEN_KW_STRING;
}

const char *token_name(TokenType type)
{
    // AI GENERATED: prompt: generate the rest of the cases acording to enum (provided first 2 lines of case and token_type enum)
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
    case TOKEN_OP_LESS_EQ:
        return "'<='";
    case TOKEN_OP_MORE_EQ:
        return "'>='";
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
    case TOKEN_KW_IMPORT:
        return "'import'";
    case TOKEN_KW_UNIT:
        return "'Unit'";
    case TOKEN_KW_VAL:
        return "'val'";
    case TOKEN_KW_VAR:
        return "'var'";
    case TOKEN_KW_RETURN:
        return "'return'";
    case TOKEN_KW_WHILE:
        return "'while'";
    case TOKEN_KW_THEN:
        return "'then'";
    case TOKEN_KW_DOUBLE:
        return "'double'";
    case TOKEN_KW_STRING:
        return "'string'";
    }
    return "token";
}

// destroy token array
void destruct_token_array(token_array *t)
{
    free(t->token_data);
    t->token_data = NULL;
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
        if (n->as.program.items)
        {
            for (int i = 0; i < n->as.program.size; i++)
            {
                free_node(n->as.program.items[i]);
            }
            free(n->as.program.items);
        }
        break;
    case node_function:
        free(n->as.function.name);
        free_node(n->as.function.body);
        for (int i = 0; i < n->as.function.param_count; i++)
            free(n->as.function.params[i].name);
        free(n->as.function.params);
        break;
    case node_block:
        for (int i = 0; i < n->as.block.size; i++)
        {
            free_node(n->as.block.items[i]);
        }
        free(n->as.block.items);
        break;
    case node_val_var:
        free(n->as.val_var.name);
        free_node(n->as.val_var.init);
        break;
    case node_asign:
        free(n->as.asign.name);
        free_node(n->as.asign.init);
        break;

    case node_expression:
        free_node(n->as.expression.expression);
        break;

    case node_return:
        free_node(n->as.ret.value);
        break;

    case node_if:
        free_node(n->as.if_n.condition);
        free_node(n->as.if_n.then);
        free_node(n->as.if_n.else_n);
        break;

    case node_while:
        free_node(n->as.while_n.condition);
        free_node(n->as.while_n.body);
        break;

    case node_lit:
        if (n->as.lit.type == type_string)
            free(n->as.lit.s);
        break;
    case node_id:
        free(n->as.ident.name);
        break;

    case node_unar:
        free_node(n->as.unar.operand);
        break;

    case node_binar:
        free_node(n->as.binar.left);
        free_node(n->as.binar.right);
        break;

    case node_call:
        free(n->as.call.name);
        if (n->as.call.items)
        {
            for (int i = 0; i < n->as.call.size; i++)
            {
                free_node(n->as.call.items[i]);
            }
            free(n->as.call.items);
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
        return greater;
    case TOKEN_OP_LESS_EQ:
        return lesser_or_equal;
    case TOKEN_OP_MORE_EQ:
        return greater_or_equal;
    default:
        return none;
    }
}

stack *stack_init(int cap)
{
    stack *temp = malloc(sizeof(stack));
    if (temp == NULL)
        print_error("malloc failed", INTERNAL_ERROR, 0, 0);

    if (cap <= 0)
        cap = 1;
    temp->items = calloc(1, sizeof(stack_item *) * cap);
    if (temp->items == NULL)
        print_error("malloc failed", INTERNAL_ERROR, 0, 0);
    temp->cap = cap;
    temp->size = 0;

    return temp;
}

stack_item *create_stack_item(item_type t)
{
    stack_item *i = calloc(1, sizeof(stack_item));
    if (i == NULL)
        print_error("Malloc failed", INTERNAL_ERROR, 0, 0);

    i->type = t;
    return i;
}
void free_stack_item(stack_item *si)
{
    if (si->type == node_item && si->ast.n != NULL)
        free_node(si->ast.n);

    free(si);
}

void free_stack(stack *s)
{
    for (int i = 0; i < s->size; i++)
    {
        free_stack_item(s->items[i]);
    }
    free(s->items);
    free(s);
}

bool is_empty_stack(stack *s)
{
    return s->size <= 0;
}

stack_item *stack_pop(stack *s)
{
    stack_item *si = NULL;
    if (!is_empty_stack(s))
    {
        si = s->items[s->size - 1];
        s->size--;
    }
    return si;
}

void stack_add(stack_item *si, stack *s)
{
    if (s->cap == s->size)
    {
        stack_item **temp = realloc(s->items, s->cap * 2 * sizeof(stack_item *));
        if (temp == NULL)
            print_error("malloc failed", INTERNAL_ERROR, 0, 0);
        s->items = temp;
        s->cap = s->cap * 2;
    }
    s->items[s->size] = si;
    s->size++;
}

int token_to_index(token_array *arr, int depth, int prev_index)
{
    token *curr = peek_token_array(arr, 0);

    token *next = NULL;
    if (arr->pos + 1 < arr->size)
        next = peek_token_array(arr, 1);

    if (curr->type == TOKEN_IDENTIFIER && next && next->type == TOKEN_LPAR)
        print_error("didnt expect function here", SYNTAX_ERROR, curr->row, curr->column);

    if (curr->type == TOKEN_IDENTIFIER || curr->type == TOKEN_INT_LITERAL || curr->type == TOKEN_DOUBLE_LITERAL || curr->type == TOKEN_STRING_LITERAL)
        return 8;

    if (curr->type == TOKEN_OP_MINUS && (prev_index == 8 || prev_index == 6))
        return 1;
    else if (curr->type == TOKEN_OP_MINUS)
        return 4;

    if (curr->type == TOKEN_RPAR && depth <= 0)
        return 7;
    else if (curr->type == TOKEN_RPAR)
        return 6;

    switch (curr->type)
    {
    case TOKEN_OP_PLUS:
        return 0;
    case TOKEN_OP_MUL:
        return 2;
    case TOKEN_OP_DIV:
        return 3;
    case TOKEN_LPAR:
        return 5;
    case TOKEN_OP_LESS:
        return 9;
    case TOKEN_OP_LESS_EQ:
        return 10;
    case TOKEN_OP_MORE:
        return 11;
    case TOKEN_OP_MORE_EQ:
        return 12;
    case TOKEN_OP_EQ:
        return 13;
    case TOKEN_OP_NEQ:
        return 14;
    default:
        return 7;
    }
}

node *parse_term(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);
    if (curr->data == NULL)
        print_error("Syntax error, expected expression", SYNTAX_ERROR, curr->row, curr->column);

    if (curr->type == TOKEN_IDENTIFIER)
    {
        node *id = create_node(node_id, curr->row, curr->column);
        id->as.ident.name = strduplicate(curr->data);
        skip_token(arr);
        return id;
    }
    else if (curr->type == TOKEN_INT_LITERAL)
    {
        node *lit = create_node(node_lit, curr->row, curr->column);
        lit->as.lit.type = type_int;
        lit->as.lit.i = curr->data ? atoi(curr->data) : 0;
        skip_token(arr);
        return lit;
    }
    else if (TOKEN_DOUBLE_LITERAL == curr->type)
    {
        node *lit = create_node(node_lit, curr->row, curr->column);
        lit->as.lit.d = atof(curr->data);
        lit->as.lit.type = type_double;
        skip_token(arr);
        return lit;
    }
    else if (TOKEN_STRING_LITERAL == curr->type)
    {
        node *lit = create_node(node_lit, curr->row, curr->column);
        lit->as.lit.s = strduplicate(curr->data);
        lit->as.lit.type = type_string;
        skip_token(arr);
        return lit;
    }
    else
        print_error("Syntax error, expected expression", SYNTAX_ERROR, curr->row, curr->column);
    return NULL;
}

int top_terminal(stack *st)
{
    if (st == NULL || st->size <= 0)
        return 7;

    int i = st->size - 1;
    while (i >= 0 && st->items[i]->type != terminal)
        i--;

    if (i < 0)
        return 7;

    return st->items[i]->ast.term;
}

operator idx_to_op(int idx)
{
    switch (idx)
    {
    case 0:
        return add;
    case 1:
        return subtract;
    case 2:
        return multiply;
    case 3:
        return divide;
    case 4:
        return negation;
    case 9:
        return lesser;
    case 10:
        return lesser_or_equal;
    case 11:
        return greater;
    case 12:
        return greater_or_equal;
    case 13:
        return equal;
    case 14:
        return not_equal;
    case 5:
    case 6:
    case 7:
    case 8:
    default:
        return none;
    }
}

void reduce_stack(stack *st, token *curr)
{
    node *nn = NULL;

    if (st->size >= 2 && st->items[st->size - 1]->type == node_item && st->items[st->size - 2]->type == terminal && st->items[st->size - 2]->ast.term == 4)
    {
        stack_item *s1 = stack_pop(st);
        stack_item *neg = stack_pop(st);

        nn = create_node(node_unar, curr->row, curr->column);
        nn->as.unar.op = idx_to_op(neg->ast.term);
        nn->as.unar.operand = s1->ast.n;

        free(s1);
        free(neg);
    }
    else if (st->size >= 3 && st->items[st->size - 1]->type == node_item && st->items[st->size - 2]->type == terminal && st->items[st->size - 3]->type == node_item)
    {
        if (st->items[st->size - 2]->ast.term > 3 && st->items[st->size - 2]->ast.term < 9)
        {
            free_stack(st);
            print_error("Syntax error, unexpected operator", SYNTAX_ERROR, curr->row, curr->column);
        }

        stack_item *sir = stack_pop(st);
        stack_item *op = stack_pop(st);
        stack_item *sil = stack_pop(st);

        nn = create_node(node_binar, curr->row, curr->column);
        nn->as.binar.right = sir->ast.n;
        free(sir);

        nn->as.binar.op = idx_to_op(op->ast.term);
        free(op);

        nn->as.binar.left = sil->ast.n;
        free(sil);
    }
    else if (st->size >= 3 && st->items[st->size - 1]->type == terminal && st->items[st->size - 2]->type == node_item && st->items[st->size - 3]->type == terminal)
    {
        if (!(st->items[st->size - 1]->ast.term == 6 && st->items[st->size - 3]->ast.term == 5))
        {
            free_stack(st);
            print_error("Syntax error, unexpected term", SYNTAX_ERROR, curr->row, curr->column);
        }

        stack_item *sir = stack_pop(st);
        stack_item *s1 = stack_pop(st);
        stack_item *sil = stack_pop(st);

        nn = s1->ast.n;

        free(sir);
        free(sil);
        free(s1);
    }
    else
    {
        free_stack(st);
        print_error("Syntax error, expected expression", SYNTAX_ERROR, curr->row, curr->column);
    }

    stack_item *si = create_stack_item(node_item);
    si->ast.n = nn;
    stack_add(si, st);
}

node *parse_expr(token_array *arr)
{
    stack *st = stack_init(2);

    stack_item *si = create_stack_item(terminal);
    si->ast.term = 7;
    stack_add(si, st);

    int depth = 0;
    int prev_index = 7;
    bool done = false;

    while (!done)
    {
        int row = top_terminal(st);

        token *curr = peek_token_array(arr, 0);
        int column = token_to_index(arr, depth, prev_index);

        actions ac = table[row][column];

        switch (ac)
        {
        case shift:
            if (column == 8)
            {
                if (prev_index == 8 || prev_index == 6)
                {
                    free_stack(st);
                    print_error("Syntax error, unexpected operand", SYNTAX_ERROR, curr->row, curr->column);
                }

                si = create_stack_item(node_item);
                si->ast.n = parse_term(arr);
                stack_add(si, st);
            }
            else
            {
                si = create_stack_item(terminal);
                if (column == 5)
                    depth++;
                else if (column == 6)
                    depth--;
                si->ast.term = column;
                stack_add(si, st);
                skip_token(arr);
            }
            prev_index = column;
            break;

        case reduce:
            reduce_stack(st, curr);
            break;

        case prec_equal:
            if (row == 5 && column == 6)
            {
                depth--;
                skip_token(arr);
                prev_index = 6;
                si = create_stack_item(terminal);
                si->ast.term = 6;
                stack_add(si, st);
                reduce_stack(st, curr);
            }
            else if (row == 7 && column == 7)
            {
                done = true;
            }
            break;

        case error:
        default:
        {
            int err_row = 0;
            if (curr)
                err_row = curr->row;

            int err_col = 0;
            if (curr)
                err_col = curr->column;

            free_stack(st);
            print_error("syntax error: unexpected token", SYNTAX_ERROR, err_row, err_col);
        }
        break;
        }
    }

    token *curr = peek_token_array(arr, 0);

    if (st->size != 2 || st->items[1]->type != node_item)
    {
        free_stack(st);
        int err_row = 0;
        int err_col = 0;
        if (curr)
        {
            err_row = curr->row;
            err_col = curr->column;
        }
        print_error("Syntax error, incorrect expression ending", SYNTAX_ERROR, err_row, err_col);
    }

    stack_item *top = stack_pop(st);
    node *n = top->ast.n;
    free(top);
    free_stack(st);
    return n;
}

node *parse_call(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);
    node *call = create_node(node_call, curr->row, curr->column);

    if (curr->data == NULL)
        print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);

    call->as.call.name = strduplicate(curr->data);
    skip_token(arr);
    get_token_from_array(arr, TOKEN_LPAR, NULL);

    curr = peek_token_array(arr, 0);
    if (curr->type == TOKEN_COMMA)
        print_error("Syntax error, unexpected ','", SYNTAX_ERROR, curr->row, curr->column);

    call->as.call.size = 0;

    int last_comma = 0;
    while (TOKEN_RPAR != curr->type)
    {
        if (curr->type == TOKEN_IDENTIFIER || curr->type == TOKEN_INT_LITERAL || curr->type == TOKEN_DOUBLE_LITERAL || curr->type == TOKEN_STRING_LITERAL)
        {
            if (curr->data == NULL)
                print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);

            node **temp = realloc(call->as.call.items, sizeof(node *) * (call->as.call.size + 1));
            if (temp == NULL)
                print_error("Internal error, Malloc failed", INTERNAL_ERROR, 0, 0);
            node *argument = parse_expr(arr);
            call->as.call.items = temp;
            call->as.call.size++;
            call->as.call.items[call->as.call.size - 1] = argument;
            curr = peek_token_array(arr, 0);
            if (curr->type != TOKEN_COMMA && curr->type != TOKEN_RPAR)
                print_error("Syntax error, expected identifier or literal", SYNTAX_ERROR, curr->row, curr->column);
        }
        else if (curr->type == TOKEN_COMMA)
        {
            if (peek_token_array(arr, 1)->type == TOKEN_RPAR)
                last_comma = arr->pos;

            if (peek_token_array(arr, 1)->type == TOKEN_COMMA)
                print_error("Syntax error, unexpected ','", SYNTAX_ERROR, peek_token_array(arr, 1)->row, peek_token_array(arr, 1)->column);

            skip_token(arr);
            curr = peek_token_array(arr, 0);
            continue;
        }
        else
        {
            print_error("Syntax error, expected identifier or liberal", SYNTAX_ERROR, curr->row, curr->column); // big hehe
        }
    }
    get_token_from_array(arr, TOKEN_RPAR, NULL);

    if (last_comma != 0)
    {
        if (arr->token_data[last_comma].row == arr->token_data[arr->pos - 1].row)
            print_error("Syntax error, unexpected ','", SYNTAX_ERROR, arr->token_data[arr->pos - 1].row, arr->token_data[arr->pos - 1].column);
    }
    return call;
}

node *parse_return(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);

    node *ret = create_node(node_return, curr->row, curr->column);

    skip_token(arr);
    curr = peek_token_array(arr, 0);

    if (curr->type == TOKEN_EOL || curr->type == TOKEN_DED || curr->type == TOKEN_SEMICOLON)
    {
        ret->as.ret.value = NULL;
        return ret;
    }
    else if (curr->type == TOKEN_IDENTIFIER && peek_token_array(arr, 1)->type == TOKEN_LPAR)
    {
        ret->as.ret.value = parse_call(arr);
    }
    else
        ret->as.ret.value = parse_expr(arr);
    return ret;
}

node *parse_val_var(token_array *arr, token *curr)
{
    node *vl = create_node(node_val_var, curr->row, curr->column);
    if (curr->type != TOKEN_KW_VAL)
        vl->as.val_var.changable = true;
    else
        vl->as.val_var.changable = false;

    skip_token(arr);
    curr = peek_token_array(arr, 0);
    if (curr->data == NULL || curr->type != TOKEN_IDENTIFIER)
        print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);

    vl->as.val_var.name = strduplicate(curr->data);

    skip_token(arr);
    curr = peek_token_array(arr, 0);

    if (curr->type == TOKEN_COLON)
    {
        get_token_from_array(arr, TOKEN_COLON, NULL);
        curr = &arr->token_data[arr->pos];

        if (parse_type(curr) == type_none)
            print_error("Syntax error, expected type", SYNTAX_ERROR, curr->row, curr->column);

        vl->as.val_var.type = parse_type(curr);

        skip_token(arr);
        curr = peek_token_array(arr, 0);
    }
    else
        vl->as.val_var.type = type_none;

    if (curr->type == TOKEN_OP_ASSIGN)
    {
        skip_token(arr);
        curr = peek_token_array(arr, 0);
        if (curr->type == TOKEN_IDENTIFIER && peek_token_array(arr, 1)->type == TOKEN_LPAR)
            vl->as.val_var.init = parse_call(arr);
        else
            vl->as.val_var.init = parse_expr(arr);

        return vl;
    }
    else
        print_error("Syntax error, expected '='", SYNTAX_ERROR, curr->row, curr->column);

    return NULL;
}

node *parse_asign(token_array *arr)
{
    token *curr = peek_token_array(arr, 0);
    node *as = create_node(node_asign, curr->row, curr->column);

    if (curr->data == NULL)
        print_error("Syntax error, expected identifier", SYNTAX_ERROR, curr->row, curr->column);

    as->as.asign.name = strduplicate(curr->data);
    skip_token(arr);
    get_token_from_array(arr, TOKEN_OP_ASSIGN, NULL);
    curr = peek_token_array(arr, 0);

    if (curr->type == TOKEN_IDENTIFIER && peek_token_array(arr, 1)->type == TOKEN_LPAR)
        as->as.asign.init = parse_call(arr);
    else
        as->as.asign.init = parse_expr(arr);

    return as;
}

node *parse_block(token_array *arr, bool indent)
{
    token *curr = peek_token_array(arr, 0);
    node *bl = create_node(node_block, curr->row, curr->column);
    node *nn = NULL;
    do
    {
        if (curr->type == TOKEN_KW_VAL || curr->type == TOKEN_KW_VAR)
        {
            nn = parse_val_var(arr, curr);
        }
        else if (curr->type == TOKEN_KW_RETURN)
        {
            nn = parse_return(arr);
        }
        else if (is_keyword(curr))
        {
            print_error("Syntax error, unexpected keyword", SYNTAX_ERROR, curr->row, curr->column);
        }
        else if (curr->type == TOKEN_EOF)
        {
            print_error("Syntax error, expected dedent", SYNTAX_ERROR, curr->row, curr->column);
        }
        else if (curr->type == TOKEN_IDENTIFIER)
        {
            token *ahead = peek_token_array(arr, 1);
            if (ahead->type == TOKEN_OP_ASSIGN)
            {
                nn = parse_asign(arr);
            }
            else if (ahead->type == TOKEN_LPAR)
            {
                nn = create_node(node_expression, curr->row, curr->column);
                nn->as.expression.expression = parse_call(arr);
            }
            else
            {
                nn = create_node(node_expression, curr->row, curr->column);
                nn->as.expression.expression = parse_expr(arr);
            }
        }
        else if (curr->type == TOKEN_INT_LITERAL || curr->type == TOKEN_DOUBLE_LITERAL || curr->type == TOKEN_STRING_LITERAL)
        {
            nn = create_node(node_expression, curr->row, curr->column);
            nn->as.expression.expression = parse_expr(arr);
        }
        else if (curr->type == TOKEN_KW_WHILE)
            nn = parse_while(arr);
        else if (curr->type == TOKEN_KW_IF)
            nn = parse_if(arr);
        else
        {
            print_error("Syntax error", SYNTAX_ERROR, curr->row, curr->column);
        }

        curr = peek_token_array(arr, 0);

        if (nn == NULL)
            print_error("Syntax error, expected expression", SYNTAX_ERROR, curr->row, curr->column);

        if (nn->type == node_while || nn->type == node_if)
        {
            node **temp = realloc(bl->as.block.items, sizeof(node *) * (bl->as.block.size + 1));
            if (temp == NULL)
                print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
            bl->as.block.items = temp;
            bl->as.block.items[bl->as.block.size++] = nn;
            curr = peek_token_array(arr, 0);
            continue;
        }

        if (curr->type == TOKEN_DED)
        {
            node **temp = realloc(bl->as.block.items, sizeof(node *) * (bl->as.block.size + 1));
            if (temp == NULL)
                print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
            bl->as.block.items = temp;
            bl->as.block.size++;

            bl->as.block.items[bl->as.block.size - 1] = nn;
            skip_token(arr);
            return bl;
        }

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
            if (curr->type != TOKEN_EOL)
                print_error("Syntax error, expected new line", SYNTAX_ERROR, curr->row, curr->column);
            skip_token(arr);
            curr = peek_token_array(arr, 0);
        }

        node **temp = realloc(bl->as.block.items, sizeof(node *) * (bl->as.block.size + 1));
        if (temp == NULL)
            print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
        bl->as.block.items = temp;
        bl->as.block.size++;

        bl->as.block.items[bl->as.block.size - 1] = nn;
        curr = peek_token_array(arr, 0);
    } while (indent && curr->type != TOKEN_DED);
    if (indent)
        skip_token(arr);
    return bl;
}

node *parse_function_body(token_array *arr)
{
    node *bl = NULL;
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

    bl = parse_block(arr, indent);
    return bl;
}

node *parse_while(token_array *arr)
{
    get_token_from_array(arr, TOKEN_KW_WHILE, NULL);

    token *curr = peek_token_array(arr, 0);
    node *wh = create_node(node_while, curr->row, curr->column);

    wh->as.while_n.condition = parse_expr(arr);

    get_token_from_array(arr, TOKEN_KW_DO, NULL);
    get_token_from_array(arr, TOKEN_EOL, NULL);
    get_token_from_array(arr, TOKEN_IND, NULL);

    wh->as.while_n.body = parse_block(arr, true);

    return wh;
}
// dont know if after else has to be EOL, ill say yes
node *parse_if(token_array *arr)
{
    get_token_from_array(arr, TOKEN_KW_IF, NULL);

    node *if_else = create_node(node_if, peek_token_array(arr, 0)->row, peek_token_array(arr, 0)->column);

    if_else->as.if_n.condition = parse_expr(arr);

    get_token_from_array(arr, TOKEN_KW_THEN, NULL);

    get_token_from_array(arr, TOKEN_EOL, NULL);

    get_token_from_array(arr, TOKEN_IND, NULL);

    if_else->as.if_n.then = parse_block(arr, true);

    get_token_from_array(arr, TOKEN_KW_ELSE, NULL);

    get_token_from_array(arr, TOKEN_EOL, NULL);

    get_token_from_array(arr, TOKEN_IND, NULL);

    if_else->as.if_n.else_n = parse_block(arr, true);

    return if_else;
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

                param *temp = realloc(n->as.function.params, sizeof(param) * (n->as.function.param_count + 1));
                if (temp == NULL)
                    print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
                n->as.function.params = temp;
                n->as.function.param_count++;

                n->as.function.params[n->as.function.param_count - 1].name = tempstr;
                n->as.function.params[n->as.function.param_count - 1].types = parse_type(curr);
                n->as.function.params[n->as.function.param_count - 1].row = row1;
                n->as.function.params[n->as.function.param_count - 1].column = collumn1;

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

    n->as.function.name = strduplicate(curr->data);
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

    n->as.function.explicit = explicit;
    n->as.function.return_type = type;

    get_token_from_array(arr, TOKEN_OP_ASSIGN, NULL);

    curr = peek_token_array(arr, 0);
    if (curr->type == TOKEN_EOL && curr->data == NULL)
    {
        skip_token(arr);
        curr = peek_token_array(arr, 0);
    }

    n->as.function.body = parse_function_body(arr);
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
    n->as.function.is_main = false;
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
                    n->as.function.is_main = true;
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
node *parse_program(token_array *arr)
{
    parse_prologue(arr);

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

        if (np->as.program.cap <= 0)
        {
            node **temp = malloc(sizeof(node *) * 1);
            if (temp == NULL)
                print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
            np->as.program.items = temp;
            np->as.program.cap = 1;
            np->as.program.size = 0;
            np->as.program.items[np->as.program.size] = f;
            np->as.program.size++;
        }
        else{
            node **temp = realloc(np->as.program.items, sizeof(node *) * (np->as.program.cap *2));
            if (temp == NULL)
                print_error("Malloc failed", INTERNAL_ERROR, 0, 0);
            np->as.program.items = temp;
            np->as.program.cap = np->as.program.cap*2;
            np->as.program.items[np->as.program.size - 1] = f;
            np->as.program.size++;
        }
    }
    return np;

    
}
int main(int argc, char const *argv[])
{
    printf("%i%s\n temp", argc, argv[0]);
    return 0;
}