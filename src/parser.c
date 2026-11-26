#include "ast.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

node *parse_for(token_array *arr);
node *parse_val_var(token_array *arr, Token *curr);
node *parse_return(token_array *arr);
node *parse_asign(token_array *arr);
node *parse_block(token_array *arr, bool indent);
node *parse_statement(token_array *arr);
node *parse_while(token_array *arr);
node *parse_if(token_array *arr);
node *parse_call(token_array *arr);
node *parse_expr(token_array *arr);
node *parse_if_expr(token_array *arr);
node *parse_interp(token_array *arr);
node *parse_member(token_array *arr, node *prev);
void block_append(node *bl, node *n2);
static node *parse_body(token_array *arr);

/// @brief duplicatres string
/// @param s, Source string
/// @return Pointer to the newly allocated string
char *strduplicate(const char *s) {
  char *p = malloc(strlen(s) + 1);
  if (p != NULL) {
    strcpy(p, s);
  }
  return p;
}

/// @brief Checks if a token is a keyword
/// @param t, Token to inspect
/// @return True if the token is a recognized keyword, else false
bool is_keyword(Token *t) {
  return t->type == TOKEN_KW_DEF || t->type == TOKEN_KW_DO ||
         t->type == TOKEN_KW_ELSE || t->type == TOKEN_KW_IF ||
         t->type == TOKEN_KW_INT || t->type == TOKEN_KW_IMPORT ||
         t->type == TOKEN_KW_UNIT || t->type == TOKEN_KW_VAL ||
         t->type == TOKEN_KW_VAR || t->type == TOKEN_KW_RETURN ||
         t->type == TOKEN_KW_WHILE || t->type == TOKEN_KW_THEN ||
         t->type == TOKEN_KW_DOUBLE || t->type == TOKEN_KW_STRING ||
         t->type == TOKEN_KW_EXTENSION || t->type == TOKEN_KW_BOOLEAN ||
         t->type == TOKEN_KW_ARROW || t->type == TOKEN_KW_FOR ||
         t->type == TOKEN_KW_UNTIL || t->type == TOKEN_KW_TO ||
         t->type == TOKEN_KW_BY;
}

/// @brief Returns readable token name
/// @param type, Token type
/// @return Static string name of token type
const char *token_name(TokenType type) {
  // AI GENERATED: prompt: generate the rest of the cases acording to enum
  // (provided first 2 lines of case and token_type enum)
  static const char *const names[] = {
      [TOKEN_IDENTIFIER] = "identifier",
      [TOKEN_INT_LITERAL] = "integer literal",
      [TOKEN_DOUBLE_LITERAL] = "double literal",
      [TOKEN_STRING_LITERAL] = "string literal",
      [TOKEN_KW_DEF] = "'def'",
      [TOKEN_KW_DO] = "'do'",
      [TOKEN_KW_ELSE] = "'else'",
      [TOKEN_KW_IF] = "'if'",
      [TOKEN_KW_INT] = "'Int'",
      [TOKEN_OP_PLUS] = "'+'",
      [TOKEN_OP_MINUS] = "'-'",
      [TOKEN_OP_MUL] = "'*'",
      [TOKEN_OP_DIV] = "'/'",
      [TOKEN_OP_ASSIGN] = "'='",
      [TOKEN_OP_EQ] = "'=='",
      [TOKEN_OP_NEQ] = "'!='",
      [TOKEN_OP_LESS] = "'<'",
      [TOKEN_OP_MORE] = "'>'",
      [TOKEN_OP_LESS_EQ] = "'<='",
      [TOKEN_OP_MORE_EQ] = "'>='",
      [TOKEN_OP_UNDERSC] = "'_'",
      [TOKEN_LPAR] = "'('",
      [TOKEN_RPAR] = "')'",
      [TOKEN_COLON] = "':'",
      [TOKEN_COMMA] = "','",
      [TOKEN_DOT] = "'.'",
      [TOKEN_AT] = "'@'",
      [TOKEN_SEMICOLON] = "';'",
      [TOKEN_EOL] = "end of line",
      [TOKEN_EOF] = "end of file",
      [TOKEN_IND] = "indent",
      [TOKEN_DED] = "dedent",
      [TOKEN_KW_IMPORT] = "'import'",
      [TOKEN_KW_UNIT] = "'Unit'",
      [TOKEN_KW_VAL] = "'val'",
      [TOKEN_KW_VAR] = "'var'",
      [TOKEN_KW_RETURN] = "'return'",
      [TOKEN_KW_WHILE] = "'while'",
      [TOKEN_KW_THEN] = "'then'",
      [TOKEN_KW_DOUBLE] = "'Double'",
      [TOKEN_KW_STRING] = "'String'",
      [TOKEN_KW_BOOLEAN] = "'Boolean'",
      [TOKEN_TRUE] = "'true'",
      [TOKEN_FALSE] = "'false'",
      [TOKEN_OP_AND] = "'&&'",
      [TOKEN_OP_OR] = "'||'",
      [TOKEN_OP_NOT] = "'!'",
      [TOKEN_KW_EXTENSION] = "'extension'",
      [TOKEN_KW_ARROW] = "'<-'",
      [TOKEN_KW_BY] = "'by'",
      [TOKEN_KW_FOR] = "'for'",
      [TOKEN_KW_UNTIL] = "'until'",
      [TOKEN_KW_TO] = "'to'",
      [TOKEN_INTERP_START] = "start of interpolated string",
      [TOKEN_INTERP_TEXT] = "interpolated text",
      [TOKEN_INTERP_ID] = "interpolated identifier",
      [TOKEN_INTERP_EXPR_START] = "'${'",
      [TOKEN_INTERP_EXPR_END] = "'}'",
      [TOKEN_INTERP_END] = "end of interpolated string",
  };
  size_t count = sizeof(names) / sizeof(names[0]);
  if ((size_t)type < count && names[type] != NULL)
    return names[type];
  return "unknown token";
}

/// @brief go to next token
/// @param arr, Token array to skip
void skip_token(token_array *arr) {
  if (arr != NULL && arr->pos < arr->size - 1)
    arr->pos++;
}

/// @brief Consumes the current token if matches type and data match else it
/// prints syntax error
/// @param arr, array of tokens
/// @param type, expected token type
/// @param data, expected token data
/// @return Pointer to the consumed token
Token *get_token_from_array(token_array *arr, TokenType type, char *data) {
  Token *curr = &arr->token_data[arr->pos];

  bool data_matches = true;
  if (data != NULL) {
    if (curr->data == NULL || strcmp(curr->data, data) != 0) {
      data_matches = false;
    }
  }

  if (curr->type == type && data_matches) {
    skip_token(arr);
    return curr;
  }

  const char *expected = token_name(type);
  if (data != NULL)
    expected = data;

  char error_msg[256];
  snprintf(error_msg, sizeof(error_msg), "Syntax error, expected %s", expected);
  print_error(error_msg, ERR_SYNTAX, arr->token_data[arr->pos].row,
              arr->token_data[arr->pos].column);
  return NULL; // Not needed but gcc throws warning without this
}

/// @brief Returns a token with offset without skipping
/// @param arr, array ofd tokens
/// @param offset, how far ahead to look
/// @return peeked token, or NULL if out of bounds
Token *peek_token_array(token_array *arr, int offset) {
  if (arr->size > 0 && arr->pos + offset < arr->size) {
    return &arr->token_data[offset + arr->pos];
  }
  print_error("syntax error", ERR_SYNTAX, arr->token_data[arr->pos].row,
              arr->token_data[arr->pos].column);
  return NULL; // Not needed but gcc throws warning without this
}

/// @brief skips to the next token and returns it
/// @param arr, array of tokens
/// @return Pointer to the next token
Token *next_token(token_array *arr) {
  skip_token(arr);
  return peek_token_array(arr, 0);
}

/// @brief Parses the file prologue import ifj26.*
/// @param arr, array of tokens
void parse_prologue(token_array *arr) {
  if (arr->size <= 0)
    print_error("Syntax error, empty file", ERR_SYNTAX, 0, 0);
  Token *curr = peek_token_array(arr, 0);

  while (curr->type == TOKEN_EOL) {
    curr = next_token(arr);
  }

  get_token_from_array(arr, TOKEN_KW_IMPORT, NULL);
  get_token_from_array(arr, TOKEN_IDENTIFIER, "ifj26");
  get_token_from_array(arr, TOKEN_DOT, NULL);
  get_token_from_array(arr, TOKEN_OP_MUL, NULL);

  if (peek_token_array(arr, 0)->type == TOKEN_EOL)
    get_token_from_array(arr, TOKEN_EOL, NULL);
  else
    get_token_from_array(arr, TOKEN_EOF, NULL);
}

/// @brief initializes AST node with zeros
/// @param type, AST node
/// @param row, line number
/// @param column, column number
/// @return Pointer to AST node
node *create_node(node_type type, int row, int column) {
  node *temp = calloc(1, sizeof(node));
  if (temp == NULL)
    print_error("Malloc failed", ERR_INTERNAL, 0, 0);

  temp->type = type;
  temp->row = row;
  temp->column = column;

  return temp;
}

/// @brief Recursively frees an AST node and all of its nodes data
/// @param n, AST node to free
void free_node(node *n) {
  if (n == NULL)
    return;
  switch (n->type) {
  case node_program:
    if (n->as.program.items) {
      for (int i = 0; i < n->as.program.size; i++) {
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
    for (int i = 0; i < n->as.block.size; i++) {
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
    if (n->as.lit.type == TYPE_STRING)
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
    if (n->as.call.items) {
      for (int i = 0; i < n->as.call.size; i++) {
        free_node(n->as.call.items[i]);
      }
      free(n->as.call.items);
    }
    free(n->as.call.list_sizes);
    break;
  case node_member:
    free(n->as.member.name);
    free_node(n->as.member.obj);
    if (n->as.member.items) {
      for (int i = 0; i < n->as.member.size; i++) {
        free_node(n->as.member.items[i]);
      }
      free(n->as.member.items);
    }
    free(n->as.member.list_sizes);
    break;
  case node_extension:
    for (int i = 0; i < n->as.extension.size; i++)
      free_node(n->as.extension.methods[i]);
    free(n->as.extension.methods);
    free(n->as.extension.reciever);
    break;

  case node_for:
    free(n->as.for_n.var_name);
    free_node(n->as.for_n.from);
    free_node(n->as.for_n.to);
    free_node(n->as.for_n.step);
    for (int i = 0; i < n->as.for_n.guard_count; i++)
      free_node(n->as.for_n.guards[i]);
    free(n->as.for_n.guards);
    free_node(n->as.for_n.body);
    break;

  case node_interp:
    for (int i = 0; i < n->as.interp.size; i++)
      free_node(n->as.interp.parts[i]);
    free(n->as.interp.parts);
    break;
  }
  free(n);
}

/// @brief Maps a type keyword token to AST data type
/// @param curr, token to convert
/// @return mapped data_types
data_types parse_type(Token *curr) {
  switch (curr->type) {
  case TOKEN_KW_INT:
    return TYPE_INT;
  case TOKEN_KW_UNIT:
    return TYPE_UNIT;
  case TOKEN_KW_DOUBLE:
    return TYPE_DOUBLE;
  case TOKEN_KW_STRING:
    return TYPE_STRING;
  case TOKEN_KW_BOOLEAN:
    return TYPE_BOOLEAN;
  default:
    return TYPE_UNKNOWN;
  }
}

/// @brief maps an operator token to AST operator
/// @param curr, operator to convert
/// @return operator enum
operator parse_operator(Token * curr) {
  switch (curr->type) {
  case TOKEN_OP_PLUS:
    return OP_ADD;
  case TOKEN_OP_MINUS:
    return OP_SUBTRACT;
  case TOKEN_OP_MUL:
    return OP_MULTIPLY;
  case TOKEN_OP_DIV:
    return OP_DIVIDE;
  case TOKEN_OP_EQ:
    return OP_EQUAL;
  case TOKEN_OP_NEQ:
    return OP_NOT_EQUAL;
  case TOKEN_OP_LESS:
    return OP_LESSER;
  case TOKEN_OP_MORE:
    return OP_GREATER;
  case TOKEN_OP_LESS_EQ:
    return OP_LESSER_OR_EQUAL;
  case TOKEN_OP_MORE_EQ:
    return OP_GREATER_OR_EQUAL;
  case TOKEN_OP_NOT:
    return OP_NOT;
  case TOKEN_OP_AND:
    return OP_AND;
  case TOKEN_OP_OR:
    return OP_OR;
  default:
    return OP_NONE;
  }
}

/// @brief initializes stack
/// @param cap, capacity of the stack
/// @return Pointer to stack
stack *stack_init(int cap) {
  stack *temp = malloc(sizeof(stack));
  if (temp == NULL)
    print_error("malloc failed", ERR_INTERNAL, 0, 0);

  if (cap <= 0)
    cap = 1;
  temp->items = calloc(1, sizeof(stack_item *) * cap);
  if (temp->items == NULL)
    print_error("malloc failed", ERR_INTERNAL, 0, 0);
  temp->cap = cap;
  temp->size = 0;

  return temp;
}

/// @brief initializes a stack item with zeros
/// @param t, type of stack item
/// @return pointer to stack item
stack_item *create_stack_item(item_type t) {
  stack_item *i = calloc(1, sizeof(stack_item));
  if (i == NULL)
    print_error("Malloc failed", ERR_INTERNAL, 0, 0);

  i->type = t;
  return i;
}

/// @brief free a stack item its AST node
/// @param si, stack item to free
void free_stack_item(stack_item *si) {
  if (si->type == node_item && si->ast.n != NULL)
    free_node(si->ast.n);

  free(si);
}

/// @brief frees all items in a stack
/// @param s, stack to free
void free_stack(stack *s) {
  for (int i = 0; i < s->size; i++) {
    free_stack_item(s->items[i]);
  }
  free(s->items);
  free(s);
}

/// @brief returns the top item from a stack
/// @param s, stack to pop
/// @return stack item on top
stack_item *stack_pop(stack *s) {
  stack_item *si = NULL;
  if (s->size > 0) {
    si = s->items[s->size - 1];
    s->size--;
  }
  return si;
}

/// @brief pushes a stack item into top of the stack
/// @param si, stack item to push
/// @param s, stack to push to
void stack_add(stack_item *si, stack *s) {
  if (s->cap == s->size) {
    stack_item **temp = realloc(s->items, s->cap * 2 * sizeof(stack_item *));
    if (temp == NULL)
      print_error("malloc failed", ERR_INTERNAL, 0, 0);
    s->items = temp;
    s->cap = s->cap * 2;
  }
  s->items[s->size] = si;
  s->size++;
}

/// @brief checks if a token can begin an operand in an expression
/// @param t, token to analyze
/// @return boolean value if is operand the start
bool is_operand_start(Token *t) {
  return t->type == TOKEN_IDENTIFIER || t->type == TOKEN_INT_LITERAL ||
         t->type == TOKEN_DOUBLE_LITERAL || t->type == TOKEN_STRING_LITERAL ||
         t->type == TOKEN_TRUE || t->type == TOKEN_FALSE ||
         t->type == TOKEN_INTERP_START || t->type == TOKEN_KW_IF ||
         t->type == TOKEN_LPAR;
}

/// @brief maps token to a precedence-table column index
/// @param arr, array of tokens
/// @param depth, nesting depth
/// @param prev_index, previous precedence-table column index
/// @return column index in the precedence-table
int token_to_index(token_array *arr, int depth, int prev_index) {
  Token *curr = peek_token_array(arr, 0);

  Token *next = NULL;
  if (arr->pos + 1 < arr->size)
    next = peek_token_array(arr, 1);

  if (curr->type == TOKEN_IDENTIFIER || curr->type == TOKEN_INT_LITERAL ||
      curr->type == TOKEN_DOUBLE_LITERAL ||
      curr->type == TOKEN_STRING_LITERAL || curr->type == TOKEN_TRUE ||
      curr->type == TOKEN_FALSE || curr->type == TOKEN_INTERP_START ||
      curr->type == TOKEN_KW_IF ||
      (curr->type == TOKEN_LPAR && next != NULL && next->type == TOKEN_RPAR))
    return 8;

  if ((prev_index == 8 || prev_index == 6) && arr->pos > 0 &&
      arr->token_data[arr->pos - 1].type == TOKEN_DED && is_operand_start(curr))
    return 7;

  if (curr->type == TOKEN_OP_MINUS && (prev_index == 8 || prev_index == 6))
    return 1;
  else if (curr->type == TOKEN_OP_MINUS)
    return 4;

  if (curr->type == TOKEN_RPAR && depth <= 0)
    return 7;
  else if (curr->type == TOKEN_RPAR)
    return 6;

  switch (curr->type) {
  case TOKEN_OP_PLUS:
    return 0;
  case TOKEN_OP_MUL:
    return 2;
  case TOKEN_OP_DIV:
    return 3;
  case TOKEN_OP_NOT:
    if (prev_index == 8 || prev_index == 6)
      print_error("Syntax error, unexpected", ERR_SYNTAX, curr->row,
                  curr->column);
    return 4;
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
  case TOKEN_OP_AND:
    return 15;
  case TOKEN_OP_OR:
    return 16;
  default:
    return 7;
  }
}

/// @brief Parses a string of interpolation expression into an AST node
/// @param arr, array of tokens
/// @return pointer node_interp AST node
node *parse_interp(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);
  node *in = create_node(node_interp, curr->row, curr->column);

  get_token_from_array(arr, TOKEN_INTERP_START, NULL);
  curr = peek_token_array(arr, 0);

  while (curr->type != TOKEN_INTERP_END) {
    node *part = NULL;

    if (curr->type == TOKEN_INTERP_TEXT) {
      part = create_node(node_lit, curr->row, curr->column);
      part->as.lit.type = TYPE_STRING;
      part->as.lit.s = strduplicate(curr->data ? curr->data : "");
      skip_token(arr);
    } else if (curr->type == TOKEN_INTERP_ID && curr->data != NULL) {
      part = create_node(node_id, curr->row, curr->column);
      part->as.ident.name = strduplicate(curr->data);
      skip_token(arr);
    } else if (curr->type == TOKEN_INTERP_EXPR_START) {
      skip_token(arr);
      part = parse_expr(arr);
      get_token_from_array(arr, TOKEN_INTERP_EXPR_END, NULL);
    } else
      print_error("Syntax error, unexpected token in interpolated string",
                  ERR_SYNTAX, curr->row, curr->column);

    node **tmp =
        realloc(in->as.interp.parts, sizeof(node *) * (in->as.interp.size + 1));
    if (tmp == NULL)
      print_error("Malloc failed", ERR_INTERNAL, 0, 0);
    in->as.interp.parts = tmp;
    in->as.interp.parts[in->as.interp.size++] = part;
    curr = peek_token_array(arr, 0);
  }

  get_token_from_array(arr, TOKEN_INTERP_END, NULL);
  return in;
}

/// @brief Parses member and member calls (a.floor().round(). ...)
/// @param arr, array of tokens
/// @param prev, previous (usually identifier) AST node
/// @return AST node of member chain
node *parse_member(token_array *arr, node *prev) {
  Token *curr = peek_token_array(arr, 0);
  while (curr->type == TOKEN_DOT) {

    node *mem = create_node(node_member, curr->row, curr->column);

    curr = next_token(arr);

    if (curr->type == TOKEN_IDENTIFIER && curr->data != NULL) {
      mem->as.member.name = strduplicate(curr->data);
      mem->as.member.obj = prev;
    } else
      print_error("expected identifier", ERR_SYNTAX, curr->row, curr->column);

    curr = next_token(arr);

    while (curr->type == TOKEN_LPAR) {
      int start_size = mem->as.member.size;
      curr = next_token(arr);

      while (curr->type != TOKEN_RPAR) {
        if (mem->as.member.size == mem->as.member.cap) {
          int new_cap = mem->as.member.cap == 0 ? 1 : mem->as.member.cap * 2;
          node **temp = realloc(mem->as.member.items, sizeof(node *) * new_cap);
          if (temp == NULL)
            print_error("Malloc failed", ERR_INTERNAL, 0, 0);
          mem->as.member.items = temp;
          mem->as.member.cap = new_cap;
        }
        mem->as.member.items[mem->as.member.size++] = parse_expr(arr);
        curr = peek_token_array(arr, 0);

        if (curr->type == TOKEN_COMMA) {
          int crow = curr->row, ccol = curr->column;
          curr = next_token(arr);

          while (curr->type == TOKEN_EOL) {
            curr = next_token(arr);
          }
          if (curr->type == TOKEN_RPAR && curr->row == crow)
            print_error("Syntax error, unexpected ','", ERR_SYNTAX, crow, ccol);
        } else if (curr->type != TOKEN_RPAR)
          print_error("Syntax error, expected ',' or ')'", ERR_SYNTAX,
                      curr->row, curr->column);
      }
      get_token_from_array(arr, TOKEN_RPAR, NULL);

      int *ls = realloc(mem->as.member.list_sizes,
                        sizeof(int) * (mem->as.member.lists_count + 1));
      if (ls == NULL)
        print_error("Malloc failed", ERR_INTERNAL, 0, 0);
      mem->as.member.list_sizes = ls;
      mem->as.member.list_sizes[mem->as.member.lists_count++] =
          mem->as.member.size - start_size;

      curr = peek_token_array(arr, 0);
    }

    prev = mem;
  }
  return prev;
}

/// @brief parses term: literal, identifier, unit value, or interpolated string
/// @param arr, array of tokens
/// @return pointer to term AST node
node *parse_term(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);

  if (curr->type == TOKEN_LPAR &&
      peek_token_array(arr, 1)->type == TOKEN_RPAR) {
    node *lit = create_node(node_lit, curr->row, curr->column);
    lit->as.lit.type = TYPE_UNIT;
    skip_token(arr);
    skip_token(arr);
    return lit;
  }

  if ((curr->type == TOKEN_IDENTIFIER || curr->type == TOKEN_INT_LITERAL ||
       curr->type == TOKEN_DOUBLE_LITERAL ||
       curr->type == TOKEN_STRING_LITERAL) &&
      curr->data == NULL)
    print_error("Syntax error, expected expression", ERR_SYNTAX, curr->row,
                curr->column);

  if (curr->type == TOKEN_IDENTIFIER) {
    node *id = create_node(node_id, curr->row, curr->column);
    id->as.ident.name = strduplicate(curr->data);
    curr = next_token(arr);
    if (curr->type == TOKEN_DOT)
      id = parse_member(arr, id);
    return id;
  } else if (curr->type == TOKEN_INT_LITERAL) {
    node *lit = create_node(node_lit, curr->row, curr->column);
    lit->as.lit.type = TYPE_INT;
    lit->as.lit.i = atoi(curr->data);

    curr = next_token(arr);
    if (curr->type == TOKEN_DOT)
      lit = parse_member(arr, lit);

    return lit;
  } else if (TOKEN_DOUBLE_LITERAL == curr->type) {
    node *lit = create_node(node_lit, curr->row, curr->column);
    lit->as.lit.d = atof(curr->data);
    lit->as.lit.type = TYPE_DOUBLE;
    curr = next_token(arr);
    if (curr->type == TOKEN_DOT)
      lit = parse_member(arr, lit);

    return lit;
  } else if (TOKEN_STRING_LITERAL == curr->type) {
    node *lit = create_node(node_lit, curr->row, curr->column);
    lit->as.lit.s = strduplicate(curr->data);
    lit->as.lit.type = TYPE_STRING;

    curr = next_token(arr);
    if (curr->type == TOKEN_DOT)
      lit = parse_member(arr, lit);

    return lit;
  } else if (TOKEN_TRUE == curr->type || TOKEN_FALSE == curr->type) {
    node *lit = create_node(node_lit, curr->row, curr->column);
    lit->as.lit.type = TYPE_BOOLEAN;
    lit->as.lit.b = (TOKEN_TRUE == curr->type);
    skip_token(arr);
    return lit;
  } else if (curr->type == TOKEN_INTERP_START) {
    node *in = parse_interp(arr);

    if (peek_token_array(arr, 0)->type == TOKEN_DOT)
      in = parse_member(arr, in);

    return in;
  } else
    print_error("Syntax error, expected expression", ERR_SYNTAX, curr->row,
                curr->column);
  return NULL;
}

/// @brief returns the precedence-table index of top terminal on the stack
/// @param st, stack of expressions
/// @return terminal index, or 7 if none exists
int top_terminal(stack *st) {
  if (st == NULL || st->size <= 0)
    return 7;

  int i = st->size - 1;
  while (i >= 0 && st->items[i]->type != terminal)
    i--;

  if (i < 0)
    return 7;

  return st->items[i]->ast.term;
}

/// @brief uses shift, reduce, parenthesis for reduction step
/// @param st, expression stack
/// @param curr, current token
void reduce_stack(stack *st, Token *curr) {
  node *nn = NULL;

  if (st->size >= 2 && st->items[st->size - 1]->type == node_item &&
      st->items[st->size - 2]->type == terminal &&
      st->items[st->size - 2]->ast.term == 4) {
    stack_item *s1 = stack_pop(st);
    stack_item *neg = stack_pop(st);

    nn = create_node(node_unar, curr->row, curr->column);
    nn->as.unar.op = neg->oper;
    nn->as.unar.operand = s1->ast.n;

    free(s1);
    free(neg);
  } else if (st->size >= 3 && st->items[st->size - 1]->type == node_item &&
             st->items[st->size - 2]->type == terminal &&
             st->items[st->size - 3]->type == node_item) {
    if (st->items[st->size - 2]->ast.term > 3 &&
        st->items[st->size - 2]->ast.term < 9) {
      free_stack(st);
      print_error("Syntax error, unexpected operator", ERR_SYNTAX, curr->row,
                  curr->column);
    }

    stack_item *sir = stack_pop(st);
    stack_item *op = stack_pop(st);
    stack_item *sil = stack_pop(st);

    nn = create_node(node_binar, curr->row, curr->column);
    nn->as.binar.right = sir->ast.n;
    free(sir);

    nn->as.binar.op = op->oper;
    free(op);

    nn->as.binar.left = sil->ast.n;
    free(sil);
  } else if (st->size >= 3 && st->items[st->size - 1]->type == terminal &&
             st->items[st->size - 2]->type == node_item &&
             st->items[st->size - 3]->type == terminal) {
    if (!(st->items[st->size - 1]->ast.term == 6 &&
          st->items[st->size - 3]->ast.term == 5)) {
      free_stack(st);
      print_error("Syntax error, unexpected term", ERR_SYNTAX, curr->row,
                  curr->column);
    }

    stack_item *sir = stack_pop(st);
    stack_item *s1 = stack_pop(st);
    stack_item *sil = stack_pop(st);

    nn = s1->ast.n;

    free(sir);
    free(sil);
    free(s1);
  } else {
    free_stack(st);
    print_error("Syntax error, expected expression", ERR_SYNTAX, curr->row,
                curr->column);
  }

  stack_item *si = create_stack_item(node_item);
  si->ast.n = nn;
  stack_add(si, st);
}

/// @brief parses indented block or single inline expression
/// @param arr, array of tokens
/// @return pointer to AST node node_block
node *parse_inline_body(token_array *arr) {
  if (peek_token_array(arr, 0)->type == TOKEN_EOL) {
    skip_token(arr);
    get_token_from_array(arr, TOKEN_IND, NULL);
    return parse_block(arr, true);
  }
  Token *t = peek_token_array(arr, 0);
  node *blk = create_node(node_block, t->row, t->column);
  node *e = create_node(node_expression, t->row, t->column);
  e->as.expression.expression = parse_expr(arr);
  block_append(blk, e);
  return blk;
}

/// @brief Parses an if-then-else expression
/// @param arr, array of tokens at 'if'
/// @return pointer to AST node node_if
node *parse_if_expr(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);
  node *n = create_node(node_if, curr->row, curr->column);

  get_token_from_array(arr, TOKEN_KW_IF, NULL);
  n->as.if_n.condition = parse_expr(arr);
  get_token_from_array(arr, TOKEN_KW_THEN, NULL);

  n->as.if_n.then = parse_inline_body(arr);

  while (peek_token_array(arr, 0)->type == TOKEN_EOL &&
         peek_token_array(arr, 1)->type == TOKEN_KW_ELSE)
    skip_token(arr);

  get_token_from_array(arr, TOKEN_KW_ELSE, NULL);

  if (peek_token_array(arr, 0)->type == TOKEN_KW_IF) {
    Token *e = peek_token_array(arr, 0);
    node *blk = create_node(node_block, e->row, e->column);
    block_append(blk, parse_if_expr(arr));
    n->as.if_n.else_n = blk;
  } else
    n->as.if_n.else_n = parse_inline_body(arr);
  return n;
}

/// @brief parses operand expression: calls, if, terms
/// @param arr, array of tokens
/// @return pointer to AST node
node *parse_operand(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);
  if (curr->type == TOKEN_IDENTIFIER &&
      peek_token_array(arr, 1)->type == TOKEN_LPAR)
    return parse_call(arr);
  if (curr->type == TOKEN_KW_IF)
    return parse_if_expr(arr);
  return parse_term(arr);
}

/// @brief parses a full expression using shift and reduce
/// @param arr, array of tokens
/// @return pointer to AST node expression
node *parse_expr(token_array *arr) {
  stack *st = stack_init(2);

  stack_item *si = create_stack_item(terminal);
  si->ast.term = 7;
  stack_add(si, st);

  int depth = 0;
  int prev_index = 7;
  bool done = false;

  while (!done) {
    int row = top_terminal(st);

    Token *curr = peek_token_array(arr, 0);
    int column = token_to_index(arr, depth, prev_index);

    actions ac = table[row][column];

    switch (ac) {
    case shift:
      if (column == 8) {
        if (prev_index == 8 || prev_index == 6) {
          free_stack(st);
          print_error("Syntax error, unexpected operand", ERR_SYNTAX, curr->row,
                      curr->column);
        }

        si = create_stack_item(node_item);
        si->ast.n = parse_operand(arr);

        stack_add(si, st);
      } else {
        si = create_stack_item(terminal);
        if (column == 5)
          depth++;
        else if (column == 6)
          depth--;
        si->ast.term = column;
        curr = peek_token_array(arr, 0);
        if (column == 4 && curr->type == TOKEN_OP_MINUS) {
          si->oper = OP_NEGATE;
        } else {
          si->oper = parse_operator(curr);
        }
        stack_add(si, st);
        skip_token(arr);
      }
      prev_index = column;
      break;

    case reduce:
      reduce_stack(st, curr);
      break;

    case prec_equal:
      if (row == 5 && column == 6) {
        depth--;
        skip_token(arr);
        prev_index = 6;
        si = create_stack_item(terminal);
        si->ast.term = 6;
        stack_add(si, st);
        reduce_stack(st, curr);
      } else if (row == 7 && column == 7) {
        done = true;
      }
      break;

    case error:
    default: {
      int err_row = curr->row;
      int err_col = curr->column;
      free_stack(st);
      print_error("syntax error: unexpected token", ERR_SYNTAX, err_row,
                  err_col);
    } break;
    }
  }

  Token *curr = peek_token_array(arr, 0);

  if (st->size != 2 || st->items[1]->type != node_item) {
    free_stack(st);
    int err_row = 0;
    int err_col = 0;
    if (curr) {
      err_row = curr->row;
      err_col = curr->column;
    }
    print_error("Syntax error, incorrect expression ending", ERR_SYNTAX,
                err_row, err_col);
  }

  stack_item *top = stack_pop(st);
  node *n = top->ast.n;
  free(top);
  free_stack(st);

  if (peek_token_array(arr, 0)->type == TOKEN_DOT)
    n = parse_member(arr, n);

  return n;
}

/// @brief parses a function call
/// @param arr, array of tokens
/// @return pointer to AST node node_call
node *parse_call(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);
  node *call = create_node(node_call, curr->row, curr->column);

  if (curr->data == NULL)
    print_error("Syntax error, expected identifier", ERR_SYNTAX, curr->row,
                curr->column);

  call->as.call.name = strduplicate(curr->data);
  skip_token(arr);

  do {
    int size_before = call->as.call.size;

    get_token_from_array(arr, TOKEN_LPAR, NULL);
    curr = peek_token_array(arr, 0);

    if (curr->type == TOKEN_COMMA)
      print_error("Syntax error, unexpected ','", ERR_SYNTAX, curr->row,
                  curr->column);

    while (curr->type != TOKEN_RPAR) {
      node **temp = realloc(call->as.call.items,
                            sizeof(node *) * (call->as.call.size + 1));
      if (temp == NULL)
        print_error("Malloc failed", ERR_INTERNAL, 0, 0);
      call->as.call.items = temp;
      call->as.call.items[call->as.call.size++] = parse_expr(arr);
      curr = peek_token_array(arr, 0);

      if (curr->type == TOKEN_COMMA) {
        int crow = curr->row, ccol = curr->column;
        curr = next_token(arr);
        while (curr->type == TOKEN_EOL) {
          curr = next_token(arr);
        }
        if (curr->type == TOKEN_RPAR && curr->row == crow)
          print_error("Syntax error, unexpected ','", ERR_SYNTAX, crow, ccol);
      } else if (curr->type != TOKEN_RPAR)
        print_error("Syntax error, expected ',' or ')'", ERR_SYNTAX, curr->row,
                    curr->column);
    }
    get_token_from_array(arr, TOKEN_RPAR, NULL);

    int *ls = realloc(call->as.call.list_sizes,
                      sizeof(int) * (call->as.call.lists_count + 1));
    if (ls == NULL)
      print_error("Malloc failed", ERR_INTERNAL, 0, 0);
    call->as.call.list_sizes = ls;
    call->as.call.list_sizes[call->as.call.lists_count++] =
        call->as.call.size - size_before;

    curr = peek_token_array(arr, 0);
  } while (curr->type == TOKEN_LPAR);

  return call;
}

/// @brief parses return and optional return value
/// @param arr, tokens of array
/// @return pointer to AST node node_return
node *parse_return(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);
  node *ret = create_node(node_return, curr->row, curr->column);
  curr = next_token(arr);
  if (curr->type != TOKEN_EOL && curr->type != TOKEN_DED &&
      curr->type != TOKEN_SEMICOLON && curr->type != TOKEN_EOF)
    ret->as.ret.value = parse_expr(arr);
  return ret;
}

/// @brief parses 'val' and 'var' declarations with optional type and
/// initializer
/// @param arr, array of tokens
/// @param curr, current token pointing to 'val' or 'var'
/// @return pointer to AST node node_val_var
node *parse_val_var(token_array *arr, Token *curr) {
  node *vl = create_node(node_val_var, curr->row, curr->column);
  vl->as.val_var.changable = (curr->type != TOKEN_KW_VAL);

  curr = next_token(arr);
  if (curr->data == NULL || curr->type != TOKEN_IDENTIFIER)
    print_error("Syntax error, expected identifier", ERR_SYNTAX, curr->row,
                curr->column);

  vl->as.val_var.name = strduplicate(curr->data);

  curr = next_token(arr);

  if (curr->type == TOKEN_COLON) {
    get_token_from_array(arr, TOKEN_COLON, NULL);
    curr = &arr->token_data[arr->pos];

    if (parse_type(curr) == TYPE_UNKNOWN)
      print_error("Syntax error, expected type", ERR_SYNTAX, curr->row,
                  curr->column);

    vl->as.val_var.type = parse_type(curr);
    curr = next_token(arr);
  } else
    vl->as.val_var.type = TYPE_UNKNOWN;

  if (curr->type == TOKEN_OP_ASSIGN) {
    skip_token(arr);
    vl->as.val_var.init = parse_expr(arr);
    return vl;
  }
  print_error("Syntax error, expected '='", ERR_SYNTAX, curr->row,
              curr->column);
  return NULL;
}

/// @brief parses an assign (x = 5)
/// @param arr, array of tokens at id
/// @return pointer to AST node node_asign
node *parse_asign(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);
  node *as = create_node(node_asign, curr->row, curr->column);

  if (curr->data == NULL)
    print_error("Syntax error, expected identifier", ERR_SYNTAX, curr->row,
                curr->column);

  as->as.asign.name = strduplicate(curr->data);
  skip_token(arr);
  get_token_from_array(arr, TOKEN_OP_ASSIGN, NULL);

  as->as.asign.init = parse_expr(arr);

  return as;
}

/// @brief appends guard condition AST node to a for-loop node
/// @param f, AST node for node_for
/// @param g, guard expresion AST node to append
void for_add_guard(node *f, node *g) {
  if (f->as.for_n.guard_count == f->as.for_n.guard_cap) {
    int nc = f->as.for_n.guard_cap == 0 ? 1 : f->as.for_n.guard_cap * 2;
    node **tmp = realloc(f->as.for_n.guards, sizeof(node *) * nc);
    if (tmp == NULL)
      print_error("Malloc failed", ERR_INTERNAL, 0, 0);
    f->as.for_n.guards = tmp;
    f->as.for_n.guard_cap = nc;
  }
  f->as.for_n.guards[f->as.for_n.guard_count++] = g;
}

/// @brief parses loop variable, range bounds, optional step of a for-loop
/// @param arr, array of tokens
/// @param f, AST node node_for
void for_generator(token_array *arr, node *f) {
  Token *curr = peek_token_array(arr, 0);
  if (curr->type != TOKEN_IDENTIFIER)
    print_error("Syntax error, expected identifier", ERR_SYNTAX, curr->row,
                curr->column);
  f->as.for_n.var_name = strduplicate(curr->data);
  skip_token(arr);
  get_token_from_array(arr, TOKEN_KW_ARROW, NULL);
  f->as.for_n.from = parse_expr(arr);

  curr = peek_token_array(arr, 0);
  if (curr->type == TOKEN_KW_TO)
    f->as.for_n.inclusive = true;
  else if (curr->type == TOKEN_KW_UNTIL)
    f->as.for_n.inclusive = false;
  else
    print_error("Syntax error, expected 'to' or 'until'", ERR_SYNTAX, curr->row,
                curr->column);
  skip_token(arr);
  f->as.for_n.to = parse_expr(arr);

  if (peek_token_array(arr, 0)->type == TOKEN_KW_BY) {
    skip_token(arr);
    f->as.for_n.step = parse_expr(arr);
  }
}

/// @brief parses zero or more 'if' guards
/// @param arr, array of tokens
/// @param f, AST node node_for
void for_guards(token_array *arr, node *f) {
  while (peek_token_array(arr, 0)->type == TOKEN_KW_IF) {
    skip_token(arr);
    for_add_guard(f, parse_expr(arr));
    while (peek_token_array(arr, 0)->type == TOKEN_EOL)
      skip_token(arr);
  }
}

/// @brief parses a whole for loop
/// @param arr, array of tokens
/// @return pointer to AST node node_for
node *parse_for(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);
  node *f = create_node(node_for, curr->row, curr->column);
  get_token_from_array(arr, TOKEN_KW_FOR, NULL);

  if (peek_token_array(arr, 0)->type == TOKEN_EOL) {
    skip_token(arr);
    get_token_from_array(arr, TOKEN_IND, NULL);
    for_generator(arr, f);
    while (peek_token_array(arr, 0)->type == TOKEN_EOL)
      skip_token(arr);
    for_guards(arr, f);
    get_token_from_array(arr, TOKEN_DED, NULL);
  } else {
    for_generator(arr, f);
    for_guards(arr, f);
  }

  get_token_from_array(arr, TOKEN_KW_DO, NULL);
  f->as.for_n.body = parse_body(arr);
  return f;
}

/// @brief appends statement or expression AST node to block node
/// @param bl, AST node node_block
/// @param n2, AST node to append
void block_append(node *bl, node *n2) {
  node **tmp =
      realloc(bl->as.block.items, sizeof(node *) * (bl->as.block.size + 1));
  if (tmp == NULL)
    print_error("Malloc failed", ERR_INTERNAL, 0, 0);
  bl->as.block.items = tmp;
  bl->as.block.items[bl->as.block.size++] = n2;
}

/// @brief parses a single statement
/// @param arr, array of tokens
/// @return pointer to AST node statement
node *parse_statement(token_array *arr) {
  Token *curr = peek_token_array(arr, 0);
  switch (curr->type) {
  case TOKEN_KW_VAL:
  case TOKEN_KW_VAR:
    return parse_val_var(arr, curr);
  case TOKEN_KW_RETURN:
    return parse_return(arr);
  case TOKEN_KW_WHILE:
    return parse_while(arr);
  case TOKEN_KW_IF:
    return parse_if(arr);
  case TOKEN_KW_FOR:
    return parse_for(arr);
  default:
    break;
  }
  if (curr->type == TOKEN_IDENTIFIER &&
      peek_token_array(arr, 1)->type == TOKEN_OP_ASSIGN)
    return parse_asign(arr);

  if (is_keyword(curr) || curr->type == TOKEN_EOF || curr->type == TOKEN_EOL ||
      curr->type == TOKEN_DED || curr->type == TOKEN_IND)
    print_error("Syntax error, unexpected token", ERR_SYNTAX, curr->row,
                curr->column);

  node *e = create_node(node_expression, curr->row, curr->column);
  e->as.expression.expression = parse_expr(arr);
  return e;
}

/// @brief parses a sequence of statements in an indented or single-line block
/// into AST node node_block
/// @param arr, array of tokens
/// @param indent, boolean value if idend was present
/// @return pointer to AST node node_block
node *parse_block(token_array *arr, bool indent) {
  Token *curr = peek_token_array(arr, 0);
  node *bl = create_node(node_block, curr->row, curr->column);

  while (curr->type == TOKEN_EOL) {
    curr = next_token(arr);
  }
  if (indent && curr->type == TOKEN_DED)
    print_error("Syntax error, empty block", ERR_SYNTAX, curr->row,
                curr->column);

  while (true) {
    node *nn = parse_statement(arr);
    block_append(bl, nn);
    curr = peek_token_array(arr, 0);

    bool more = false;
    bool compound =
        nn->type == node_while || nn->type == node_if || nn->type == node_for;

    bool after_ded =
        arr->pos > 0 && arr->token_data[arr->pos - 1].type == TOKEN_DED;

    if (!compound) {
      if (curr->type == TOKEN_SEMICOLON) {
        more = true;
        skip_token(arr);
      } else if (curr->type == TOKEN_EOL) {
        skip_token(arr);
      } else if (curr->type != TOKEN_DED && curr->type != TOKEN_EOF &&
                 curr->type != TOKEN_KW_ELSE && curr->type != TOKEN_RPAR &&
                 curr->type != TOKEN_COMMA && !after_ded) {
        print_error("Syntax error, expected new line", ERR_SYNTAX, curr->row,
                    curr->column);
      }
    }

    curr = peek_token_array(arr, 0);
    while (curr->type == TOKEN_EOL) {
      curr = next_token(arr);
    }

    if (!indent && !more)
      break;
    if (indent && curr->type == TOKEN_DED) {
      skip_token(arr);
      break;
    }
    if (curr->type == TOKEN_EOF)
      print_error("Syntax error, expected dedent", ERR_SYNTAX, curr->row,
                  curr->column);
  }
  return bl;
}

/// @brief parses an indented block or an inline block body acording to indent
/// (eol)
/// @param arr, array of tokens
/// @return pointer to the created node_block AST node
node *parse_body(token_array *arr) {
  if (peek_token_array(arr, 0)->type == TOKEN_EOL) {
    skip_token(arr);
    get_token_from_array(arr, TOKEN_IND, NULL);
    return parse_block(arr, true);
  }
  return parse_block(arr, false);
}

/// @brief Parses while loop and body
/// @param arr, array of tokens
/// @return pointer to AST node node_while
node *parse_while(token_array *arr) {
  get_token_from_array(arr, TOKEN_KW_WHILE, NULL);

  Token *curr = peek_token_array(arr, 0);
  node *wh = create_node(node_while, curr->row, curr->column);

  wh->as.while_n.condition = parse_expr(arr);

  get_token_from_array(arr, TOKEN_KW_DO, NULL);
  wh->as.while_n.body = parse_body(arr);

  return wh;
}

/// @brief parses if, then, else statement and chained else if statements
/// @param arr, array of tokens
/// @return pointer to AST node node_if
node *parse_if(token_array *arr) {
  get_token_from_array(arr, TOKEN_KW_IF, NULL);

  node *if_else = create_node(node_if, peek_token_array(arr, 0)->row,
                              peek_token_array(arr, 0)->column);

  if_else->as.if_n.condition = parse_expr(arr);

  get_token_from_array(arr, TOKEN_KW_THEN, NULL);

  if_else->as.if_n.then = parse_body(arr);

  while (peek_token_array(arr, 0)->type == TOKEN_EOL &&
         peek_token_array(arr, 1)->type == TOKEN_KW_ELSE)
    skip_token(arr);

  if (peek_token_array(arr, 0)->type == TOKEN_KW_ELSE) {
    get_token_from_array(arr, TOKEN_KW_ELSE, NULL);

    if (peek_token_array(arr, 0)->type == TOKEN_KW_IF) {
      Token *t = peek_token_array(arr, 0);
      node *blk = create_node(node_block, t->row, t->column);
      block_append(blk, parse_if(arr));
      if_else->as.if_n.else_n = blk;
    } else
      if_else->as.if_n.else_n = parse_body(arr);
  } else
    if_else->as.if_n.else_n = NULL;

  return if_else;
}

/// @brief Parses paramaters of a funtion separated by ','
/// @param arr, array of tokens
/// @param n, AST node to add parsed parameters
void parse_params(token_array *arr, node *n) {
  Token *curr = peek_token_array(arr, 0);

  while (curr->type != TOKEN_RPAR) {
    if (curr->data != NULL) {
      if (curr->type == TOKEN_IDENTIFIER) {
        int row1 = curr->row;
        int collumn1 = curr->column;

        char *tempstr = strduplicate(curr->data);
        curr = next_token(arr);
        if (curr->type != TOKEN_COLON)
          print_error("Syntax error, expected ':'", ERR_SYNTAX,
                      arr->token_data[arr->pos].row,
                      arr->token_data[arr->pos].column);

        curr = next_token(arr);

        if (parse_type(curr) == TYPE_UNKNOWN)
          print_error("Syntax error, expected type", ERR_SYNTAX,
                      arr->token_data[arr->pos].row,
                      arr->token_data[arr->pos].column);

        param *temp = realloc(n->as.function.params,
                              sizeof(param) * (n->as.function.param_count + 1));
        if (temp == NULL)
          print_error("Malloc failed", ERR_INTERNAL, 0, 0);
        n->as.function.params = temp;
        n->as.function.param_count++;

        n->as.function.params[n->as.function.param_count - 1].name = tempstr;
        n->as.function.params[n->as.function.param_count - 1].types =
            parse_type(curr);
        n->as.function.params[n->as.function.param_count - 1].row = row1;
        n->as.function.params[n->as.function.param_count - 1].column = collumn1;
        n->as.function.params[n->as.function.param_count - 1].list =
            n->as.function.lists_count;

        curr = next_token(arr);

        if (curr->type == TOKEN_COMMA) {
          int comma_row = curr->row;
          int comma_col = curr->column;

          curr = next_token(arr);

          while (curr->type == TOKEN_EOL) {
            curr = next_token(arr);
          }

          if (curr->type == TOKEN_RPAR && curr->row == comma_row)
            print_error("Syntax error, unexpected ','", ERR_SYNTAX, comma_row,
                        comma_col);
        } else if (curr->type != TOKEN_RPAR)
          print_error("Syntax error, expected ',' or ')'", ERR_SYNTAX,
                      curr->row, curr->column);
      } else
        print_error("Syntax error, expected identifier", ERR_SYNTAX,
                    arr->token_data[arr->pos].row,
                    arr->token_data[arr->pos].column);
    } else
      print_error("Syntax error, expected identifier", ERR_SYNTAX,
                  arr->token_data[arr->pos].row,
                  arr->token_data[arr->pos].column);
  }
}

/// @brief parses a function header (name, parameter lists, optional return
/// type, body)
/// @param arr, array of tokens
/// @param n, AST node to add parsed parameters of header
void parse_function_header(token_array *arr, node *n) {
  data_types type;
  bool explicit = false;
  bool was_eol = false;

  Token *curr = peek_token_array(arr, 0);
  if (curr->type != TOKEN_IDENTIFIER || curr->data == NULL)
    print_error("Syntax error, expected identifier", ERR_SYNTAX,
                arr->token_data[arr->pos].row,
                arr->token_data[arr->pos].column);

  n->as.function.name = strduplicate(curr->data);
  n->as.function.lists_count = 0;
  curr = next_token(arr);

  while (curr->type == TOKEN_LPAR) {
    skip_token(arr);
    if (peek_token_array(arr, 0)->type != TOKEN_RPAR)
      parse_params(arr, n);
    get_token_from_array(arr, TOKEN_RPAR, NULL);
    n->as.function.lists_count++;
    curr = peek_token_array(arr, 0);
  }

  curr = peek_token_array(arr, 0);
  if (curr->type == TOKEN_COLON) {
    get_token_from_array(arr, TOKEN_COLON, NULL);

    curr = peek_token_array(arr, 0);

    if (parse_type(curr) == TYPE_UNKNOWN)
      print_error("Syntax error, expected type", ERR_SYNTAX,
                  arr->token_data[arr->pos].row,
                  arr->token_data[arr->pos].column);

    type = parse_type(curr);
    explicit = true;
    skip_token(arr);
  } else
    type = TYPE_UNKNOWN;

  n->as.function.explicit = explicit;
  n->as.function.return_type = type;

  get_token_from_array(arr, TOKEN_OP_ASSIGN, NULL);

  curr = peek_token_array(arr, 0);
  if (curr->type == TOKEN_EOL) {
    skip_token(arr);
    was_eol = true;
  }

  if (was_eol) {
    get_token_from_array(arr, TOKEN_IND, NULL);
    n->as.function.body = parse_block(arr, true);
  } else
    n->as.function.body = parse_block(arr, false);
}

/// @brief parses a 'def' into AST node function
/// @param arr, array of tokens
/// @param curr, current token at 'def'
/// @return pointer to AST node node_function
node *parse_def(token_array *arr, Token *curr) {
  node *n = create_node(node_function, curr->row, curr->column);

  if (curr->type == TOKEN_KW_DEF) {
    skip_token(arr);
    parse_function_header(arr, n);
  } else
    print_error("Syntax error", ERR_SYNTAX, arr->token_data[arr->pos].row,
                arr->token_data[arr->pos].column);
  n->as.function.is_main = false;
  return n;
}

/// @brief parses '@main'
/// @param arr, array of tokens at '@main'
/// @param curr, curret token
/// @return pointer to AST node node_function
node *parse_main(token_array *arr, Token *curr) {
  get_token_from_array(arr, TOKEN_AT, NULL);
  get_token_from_array(arr, TOKEN_IDENTIFIER, "main");
  curr = peek_token_array(arr, 0);
  if (curr->type == TOKEN_EOL) {
    curr = next_token(arr);
  }

  if (curr->type == TOKEN_KW_DEF) {
    node *n = parse_def(arr, curr);
    n->as.function.is_main = true;
    return n;
  } else
    print_error("Syntax error", ERR_SYNTAX, arr->token_data[arr->pos].row,
                arr->token_data[arr->pos].column);
  return NULL;
}

/// @brief parses an 'extension' block
/// @param arr, array of tokens
/// @return pointer to AST node node_extension
node *parse_extension(token_array *arr) { // EXTKW extension
  Token *curr = peek_token_array(arr, 0);

  node *ext = create_node(node_extension, curr->row, curr->column);

  get_token_from_array(arr, TOKEN_KW_EXTENSION, NULL);
  get_token_from_array(arr, TOKEN_LPAR, NULL);

  curr = peek_token_array(arr, 0);
  if (curr->type != TOKEN_IDENTIFIER)
    print_error("Syntax error, expected identifier", ERR_SYNTAX, curr->row,
                curr->column);

  ext->as.extension.reciever = strduplicate(curr->data);
  skip_token(arr);
  get_token_from_array(arr, TOKEN_COLON, NULL);

  curr = peek_token_array(arr, 0);

  if (parse_type(curr) == TYPE_UNKNOWN)
    print_error("Syntax error, expected data type", ERR_SYNTAX, curr->row,
                curr->column);

  ext->as.extension.type = parse_type(curr);
  skip_token(arr);

  get_token_from_array(arr, TOKEN_RPAR, NULL);
  get_token_from_array(arr, TOKEN_EOL, NULL);
  get_token_from_array(arr, TOKEN_IND, NULL);

  curr = peek_token_array(arr, 0);

  if (curr->type == TOKEN_DED)
    print_error("Syntax error, empty block", ERR_SYNTAX, curr->row,
                curr->column);

  while (curr->type != TOKEN_DED) {
    if (curr->type == TOKEN_EOL)
      skip_token(arr);
    else if (curr->type == TOKEN_KW_DEF) {
      node **temp = realloc(ext->as.extension.methods,
                            sizeof(node *) * (ext->as.extension.size + 1));
      if (temp == NULL)
        print_error("Malloc failed", ERR_INTERNAL, 0, 0);

      ext->as.extension.methods = temp;
      ext->as.extension.methods[ext->as.extension.size] = parse_def(arr, curr);
      ext->as.extension.size++;
    } else if (curr->type == TOKEN_EOF) {
      print_error("Syntax error, expected dedent", ERR_SYNTAX, curr->row,
                  curr->column);
    } else
      print_error("Syntax error, expected def", ERR_SYNTAX, curr->row,
                  curr->column);
    curr = peek_token_array(arr, 0);
  }
  skip_token(arr);
  return ext;
}

/// @brief parses the whole program (prologue, functions and bodys of funtions)
/// @param arr, array of tokens
/// @return pointer AST node node_program
node *parse_program(token_array *arr) {
  parse_prologue(arr);

  Token *curr = peek_token_array(arr, 0);
  node *np = create_node(node_program, 0, 0);
  while (curr->type != TOKEN_EOF) {
    node *f = NULL;
    if (curr->type == TOKEN_KW_DEF) {
      f = parse_def(arr, curr);
    } else if (curr->type == TOKEN_AT) {
      f = parse_main(arr, curr);
    } else if (curr->type == TOKEN_EOL)
      skip_token(arr);
    else if (curr->type == TOKEN_KW_EXTENSION)
      f = parse_extension(arr);
    else
      print_error("Syntax error", ERR_SYNTAX, arr->token_data[arr->pos].row,
                  arr->token_data[arr->pos].column);

    curr = &arr->token_data[arr->pos];
    if (f == NULL)
      continue;

    if (np->as.program.size == np->as.program.cap) {
      int nc = np->as.program.cap <= 0 ? 1 : np->as.program.cap * 2;
      node **temp = realloc(np->as.program.items, sizeof(node *) * nc);
      if (temp == NULL)
        print_error("Malloc failed", ERR_INTERNAL, 0, 0);
      np->as.program.items = temp;
      np->as.program.cap = nc;
    }
    np->as.program.items[np->as.program.size++] = f;
  }
  return np;
}