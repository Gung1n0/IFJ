#include "token.h"
#include "enums.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  STATE_START_LINE,
  STATE_SPACE,
  STATE_EOL,
  STATE_IND,
  STATE_DED,
  STATE_READ,
  STATE_ID,
  STATE_INT,
  STATE_DOUBLE,
  STATE_STRING,
  STATE_MULTILINE_STRING,
  STATE_STRING_LOAD,
  STATE_CHAR,
  STATE_NUM,
  STATE_OP_ASSIGN,
  STATE_OP_NEQ,
  STATE_LESS,
  STATE_MORE,
  STATE_EOF,
  STATE_MAIN,
  STATE_IMPORT,
  STATE_UNIT,
  STATE_VAL,
  STATE_VAR,
  STATE_RETURN,
  STATE_WHILE,
  STATE_THEN,
  STATE_DOUBLE_LOAD_E,
  STATE_DOUBLE_LOAD,
  STATE_DOUBLE_LOAD_DOT,
  STATE_UNDERSC_LOAD,
  STATE_IDENTIFIER,
  STATE_SLASH,
  STATE_COMM,
  STATE_COMM_LVL,
  STATE_COMM_EOL,
  STATE_NUM_ZERO,
  STATE_E_SIGN,
  STATE_INTERP_LOAD,
} State;

typedef struct {
  int *data;
  int top;
  int capacity;
} IndentStack;

typedef struct {
  char *data;
  int len;
  int capacity;
} Buffer;

static int line_beg = 1;
static IndentStack stack = {NULL, 0, 0};
static int space_count = 0;
static int row = 1;
static int column = 1;
static int pen_dot = 0;
static int paren_level = 0;
static int tab_found = 0;
static Buffer buffer = {NULL, 0, 32};
static Token token;
static State state;
static int e_pen = 0;
static int comm_lvl = 0;
static int comm_eol = 0;
static int line_has_token = 0;
static int interp_multiline = 0;
static int in_interp_expr = 0;
static Token pending_tokens[16];
static int pending_head = 0;
static int pending_tail = 0;

void enqueue_token(TokenType type, const char *text) {
  Token t;
  t.type = type;
  t.row = row;
  t.column = column;
  if (text != NULL) {
    size_t len = strlen(text);
    t.data = malloc(len + 1);
    memcpy(t.data, text, len + 1);
  } else {
    t.data = NULL;
  }
  pending_tokens[pending_tail++] = t;
  line_has_token = 1;
}

void bufferCheck() {
  if (buffer.len + 1 >= buffer.capacity) {
    buffer.capacity *= 2;
    buffer.data = realloc(buffer.data, buffer.capacity * sizeof(char));
  }
}

void free_lexer_globals() {
  if (buffer.data != NULL) {
    free(buffer.data);
    buffer.data = NULL;
  }
  if (stack.data != NULL) {
    free(stack.data);
    stack.data = NULL;
  }
}

void fillToken(TokenType type) {
  if ((buffer.len > 0 || type == TOKEN_STRING_LITERAL) && buffer.data != NULL) {
    token.data = malloc(buffer.len + 1);
    for (int i = 0; i < buffer.len; i++) {
      token.data[i] = buffer.data[i];
    }
    token.data[buffer.len] = '\0';
  } else {
    token.data = NULL;
  }
  token.column = column;
  token.row = row;
  token.type = type;
  if (type != TOKEN_EOL && type != TOKEN_EOF && type != TOKEN_IND &&
      type != TOKEN_DED) {
    line_has_token = 1;
  }
}

Token get_token(FILE *source) {
  if (pending_head < pending_tail) {
    Token t = pending_tokens[pending_head++];
    if (pending_head == pending_tail) {
      pending_head = 0;
      pending_tail = 0;
    }
    return t;
  }
  buffer.len = 0;
  if (stack.data == NULL) {
    stack.capacity = 10;
    stack.data = malloc(stack.capacity * sizeof(int));
    stack.top = 0;
    stack.data[0] = 0;
  }

  if (state != STATE_SPACE && state != STATE_DED && state != STATE_EOF &&
      state != STATE_INTERP_LOAD) {
    if (line_beg) {
      state = STATE_START_LINE;
      space_count = 0;
      tab_found = 0;
    } else {
      state = STATE_READ;
    }
  }

  if (buffer.data == NULL) {
    buffer.data = malloc(buffer.capacity * sizeof(char));
  }

  if (pen_dot == 1) {
    pen_dot = 0;
    column++;
    fillToken(TOKEN_DOT);
    return token;
  }

  while (1) {
    int c;
    switch (state) {
    case (STATE_START_LINE):
      line_has_token = 0;
      while (1) {
        c = fgetc(source);
        column++;
        if (c == ' ')
          space_count++;
        else if (c == '\t')
          tab_found = 1;
        else
          break;
      }

      if (c == '\n') {
        space_count = 0;
        tab_found = 0;
        column = 1;
        row++;
        break;
      } else if (c == EOF) {
        state = STATE_EOF;
        break;
      } else {
        if (c == '/') {
          int next_c = fgetc(source);
          if (next_c == '/') {
            state = STATE_COMM;
            break;
          } else if (next_c == '*') {
            comm_lvl++;
            state = STATE_COMM_LVL;
            break;
          }
          ungetc(next_c, source);
        } 
        if (tab_found) {
          fprintf(
              stderr,
              "Lexikalna chyba: Tabulator v odsadeni na riadku %d, stlpec %d\n",
              row, column);
          free_lexer_globals();
          exit(1);
        }
        ungetc(c, source);
        column--;
        state = STATE_SPACE;
      }
      break;

    case (STATE_SPACE):
      if (space_count == stack.data[stack.top]) {
        line_beg = 0;
        state = STATE_READ;
      } else if (space_count > stack.data[stack.top])
        state = STATE_IND;
      else if (space_count < stack.data[stack.top])
        state = STATE_DED;
      break;

    case (STATE_IND):
      if (stack.top + 1 == stack.capacity) {
        stack.capacity *= 2;
        stack.data = realloc(stack.data, stack.capacity * sizeof(int));
      }
      stack.data[stack.top + 1] = space_count;
      stack.top++;
      space_count = 0;
      line_beg = 0;
      fillToken(TOKEN_IND);
      return token;
      break;

    case (STATE_DED):
      stack.top -= 1;
      state = STATE_SPACE;
      fillToken(TOKEN_DED);
      return token;
      break;

    case (STATE_EOL):
      line_beg = 1;
      space_count = 0;
      tab_found = 0;
      if (line_has_token) {
        line_has_token = 0;
        fillToken(TOKEN_EOL);
        column = 1;
        row++;
        return token;
      } else {
        column = 1;
        row++;
        state = STATE_START_LINE;
      }
      break;

    case (STATE_EOF):
      if (stack.top > 0) {
        stack.top -= 1;
        fillToken(TOKEN_DED);
        return token;
      }
      fillToken(TOKEN_EOF);
      return token;
      break;

    case (STATE_READ):
      c = fgetc(source);
      column++;

      if (c == ' ' || c == '\t' || c == '\r') {
        continue;
      } else if (c == '\n') {
        if (paren_level > 0) {
          column = 1;
          row++;
          continue;
        }
        state = STATE_EOL;
        break;
      } else if (c == EOF) {
        if (line_has_token) {
          line_has_token = 0;
          state = STATE_EOF;
          fillToken(TOKEN_EOL);
          return token;
        }
        state = STATE_EOF;
        break;
      } else if (c == '(') {
        paren_level++;
        fillToken(TOKEN_LPAR);
        return token;
      } else if (c == ')') {
        paren_level--;
        fillToken(TOKEN_RPAR);
        return token;
      } else if (c == ';') {
        fillToken(TOKEN_SEMICOLON);
        return token;
      } else if (c == '"') {
        c = fgetc(source);
        if (c == '"') {
          c = fgetc(source);
          if (c == '"') {
            column += 2;
            state = STATE_MULTILINE_STRING;
          } else {
            ungetc(c, source);
            column++;
            buffer.data[0] = '\0';
            state = STATE_STRING;
          }
        } else {
          ungetc(c, source);
          state = STATE_STRING_LOAD;
        }
      } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        ungetc(c, source);
        column--;
        state = STATE_CHAR;
      } else if (c >= '0' && c <= '9') {
        if (c == '0')
          state = STATE_NUM_ZERO;
        else {
          ungetc(c, source);
          column--;
          state = STATE_NUM;
        }
      } else if (c == '.') { // L8
        c = fgetc(source);
        if (c >= '0' && c <= '9') {
          bufferCheck();
          buffer.data[buffer.len++] = '0';
          buffer.data[buffer.len++] = '.';
          buffer.data[buffer.len++] = c;
          column++;
          state = STATE_DOUBLE_LOAD;
        } else {
          ungetc(c, source);
          fillToken(TOKEN_DOT);
          return token;
        }
      } else if (c == '+') {
        fillToken(TOKEN_OP_PLUS);
        return token;
      } else if (c == '-') {
        fillToken(TOKEN_OP_MINUS);
        return token;
      } else if (c == '*') {
        fillToken(TOKEN_OP_MUL);
        return token;
      } else if (c == '/')
        state = STATE_SLASH;
      else if (c == '=')
        state = STATE_OP_ASSIGN;
      else if (c == '!')
        state = STATE_OP_NEQ;
      else if (c == '<')
        state = STATE_LESS;
      else if (c == '>')
        state = STATE_MORE;
      else if (c == '_')
        state = STATE_UNDERSC_LOAD;
      else if (c == ':') {
        fillToken(TOKEN_COLON);
        return token;
      } else if (c == ',') {
        fillToken(TOKEN_COMMA);
        return token;
      } else if (c == '@') {
        fillToken(TOKEN_AT);
        return token;
      } else if (c == '}') {
        if (in_interp_expr) {
          in_interp_expr = 0;
          enqueue_token(TOKEN_OP_PLUS, NULL);
          state = STATE_INTERP_LOAD;
          fillToken(TOKEN_RPAR);
          return token;
        }
        fprintf(stderr,
                "Lexikalna chyba: Neplatny znak '}' na riadku %d, stlpec %d\n",
                row, column);
        free_lexer_globals();
        exit(1);
      } else if (c == '&') {
        int next_c = fgetc(source);
        if (next_c == '&') {
          column++;
          fillToken(TOKEN_OP_AND);
          return token;
        } else {
          ungetc(next_c, source);
          fprintf(stderr,
                  "Lexikalna chyba: Neplatny znak '&' na riadku %d, stlpec %d\n",
                  row, column);
          free_lexer_globals();
          exit(1);
        }
      } else if (c == '|') {
        int next_c = fgetc(source);
        if (next_c == '|') {
          column++;
          fillToken(TOKEN_OP_OR);
          return token;
        } else {
          ungetc(next_c, source);
          fprintf(stderr,
                  "Lexikalna chyba: Neplatny znak '|' na riadku %d, stlpec %d\n",
                  row, column);
          free_lexer_globals();
          exit(1);
        }
      } else {
        fprintf(stderr,
                "Lexikalna chyba: Neplatny znak na riadku %d, stlpec %d\n", row,
                column);
        free_lexer_globals();
        exit(1);
      }
      break;

    case (STATE_STRING_LOAD):
      c = fgetc(source);
      column++;
      bufferCheck();
      if (c == '\\') {
        int esc = fgetc(source);
        column++;
        if (esc == 'n')
          buffer.data[buffer.len++] = '\n';
        else if (esc == 't')
          buffer.data[buffer.len++] = '\t';
        else if (esc == 'r')
          buffer.data[buffer.len++] = '\r';
        else if (esc == '"')
          buffer.data[buffer.len++] = '"';
        else if (esc == '\\')
          buffer.data[buffer.len++] = '\\';
        else {
          fprintf(stderr,
                  "Lexikalna chyba: Zly escape na riadku %d, stlpec %d\n", row,
                  column);
          free_lexer_globals();
          exit(1);
        }
      } else if (c == '"') {
        buffer.data[buffer.len] = '\0';
        state = STATE_STRING;
      } else if (c == '\n' || c == EOF) {
        fprintf(stderr,
                "Lexikalna chyba: Neukonceny retazec na riadku %d, stlpec %d\n",
                row, column);
        free_lexer_globals();
        exit(1);
      } else {
        buffer.data[buffer.len++] = c;
      }
      break;

    case (STATE_MULTILINE_STRING):
      c = fgetc(source);
      bufferCheck();
      if (c == '\n') {
        row++;
        column = 1;
      } else {
        column++;
      }

      if (c == EOF) {
        fprintf(stderr,
                "Lexikalna chyba: Neukonceny viacriadkovy retazec na riadku "
                "%d, stlpec %d\n",
                row, column);
        free_lexer_globals();
        exit(1);
      } else if (c == '"') {
        c = fgetc(source);
        if (c == '"') {
          c = fgetc(source);
          if (c == '"') {
            column += 2;
            buffer.data[buffer.len] = '\0';
            state = STATE_STRING;
          } else {
            ungetc(c, source);
            column++;
            bufferCheck();
            buffer.data[buffer.len++] = '"';
            bufferCheck();
            buffer.data[buffer.len++] = '"';
          }
        } else {
          ungetc(c, source);
          buffer.data[buffer.len++] = '"';
        }
      } else {
        buffer.data[buffer.len++] = c;
      }
      break;

    case (STATE_STRING):
      fillToken(TOKEN_STRING_LITERAL);
      return token;
      break;

    case (STATE_CHAR):
      if (e_pen != 0) {
        c = e_pen;
        e_pen = 0;
      } else
        c = fgetc(source);
      column++;
      bufferCheck();
      if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') || c == '_') {
        buffer.data[buffer.len] = c;
        buffer.len++;
      } else {
        if (buffer.len == 1 && buffer.data[0] == 's' && c == '"') {
          c = fgetc(source);
          if (c == '"') {
            c = fgetc(source);
            if (c == '"') {
              column += 2;
              interp_multiline = 1;
            } else {
              ungetc(c, source);
              column++;
              buffer.len = 0;
              buffer.data[0] = '\0';
              fillToken(TOKEN_STRING_LITERAL);
              return token;
            }
          } else {
            ungetc(c, source);
            interp_multiline = 0;
          }
          buffer.len = 0;
          state = STATE_INTERP_LOAD;
          fillToken(TOKEN_LPAR);
          return token;
        }
        ungetc(c, source);
        column--;
        buffer.data[buffer.len] = '\0';
        if (strcmp(buffer.data, "def") == 0)
          fillToken(TOKEN_KW_DEF);
        else if (strcmp(buffer.data, "do") == 0)
          fillToken(TOKEN_KW_DO);
        else if (strcmp(buffer.data, "else") == 0)
          fillToken(TOKEN_KW_ELSE);
        else if (strcmp(buffer.data, "if") == 0)
          fillToken(TOKEN_KW_IF);
        else if (strcmp(buffer.data, "Int") == 0)
          fillToken(TOKEN_KW_INT);
        else if (strcmp(buffer.data, "String") == 0)
          fillToken(TOKEN_KW_STRING);
        else if (strcmp(buffer.data, "Double") == 0)
          fillToken(TOKEN_KW_DOUBLE);
        else if (strcmp(buffer.data, "import") == 0)
          fillToken(TOKEN_KW_IMPORT);
        else if (strcmp(buffer.data, "Unit") == 0)
          fillToken(TOKEN_KW_UNIT);
        else if (strcmp(buffer.data, "val") == 0)
          fillToken(TOKEN_KW_VAL);
        else if (strcmp(buffer.data, "var") == 0)
          fillToken(TOKEN_KW_VAR);
        else if (strcmp(buffer.data, "return") == 0)
          fillToken(TOKEN_KW_RETURN);
        else if (strcmp(buffer.data, "while") == 0)
          fillToken(TOKEN_KW_WHILE);
        else if (strcmp(buffer.data, "then") == 0)
          fillToken(TOKEN_KW_THEN);
        else if (strcmp(buffer.data, "extension") == 0)
          fillToken(TOKEN_KW_EXTENSION);
        else if (strcmp(buffer.data, "Boolean") == 0)
          fillToken(TOKEN_KW_BOOLEAN);
        else if (strcmp(buffer.data, "true") == 0)
          fillToken(TOKEN_TRUE);
        else if (strcmp(buffer.data, "false") == 0)
          fillToken(TOKEN_FALSE);
        else if (strcmp(buffer.data, "for") == 0)
          fillToken(TOKEN_KW_FOR);
        else if (strcmp(buffer.data, "to") == 0)
          fillToken(TOKEN_KW_TO);
        else if (strcmp(buffer.data, "until") == 0)
          fillToken(TOKEN_KW_UNTIL);
        else if (strcmp(buffer.data, "by") == 0)
          fillToken(TOKEN_KW_BY);
        else
          fillToken(TOKEN_IDENTIFIER);
        return token;
      }
      break;

    case STATE_NUM:
      c = fgetc(source);
      column++;
      bufferCheck();
      if (c >= '0' && c <= '9') {
        buffer.data[buffer.len] = c;
        buffer.len++;
      } else if (c == '.') {
        buffer.data[buffer.len] = c;
        buffer.len++;
        state = STATE_DOUBLE_LOAD_DOT;
      } else if (c == 'e' || c == 'E') {
        buffer.data[buffer.len] = c;
        buffer.len++;
        state = STATE_E_SIGN;
      } else if (c == '_') {
        c = fgetc(source);

        if (c >= '0' && c <= '9') {
          ungetc(c, source);
        } else {
          fprintf(
              stderr,
              "Lexikalna chyba: Zle umiestneny podciarkovnik na riadku %d\n",
              row);
          free_lexer_globals();
          exit(1);
        }
      } else {
        ungetc(c, source);
        column--;
        state = STATE_INT;
      }
      break;

    case STATE_INT:
      fillToken(TOKEN_INT_LITERAL);
      return token;
      break;

    case STATE_DOUBLE_LOAD_DOT:
      c = fgetc(source);
      column++;
      if (c >= '0' && c <= '9') {
        bufferCheck();
        buffer.data[buffer.len] = c;
        buffer.len++;
        state = STATE_DOUBLE_LOAD;
      } else {
        pen_dot = 1;
        ungetc(c, source);
        column--;
        buffer.len--;
        state = STATE_INT;
      }
      break;

    case STATE_DOUBLE_LOAD:
      bufferCheck();
      c = fgetc(source);
      column++;
      if (c == 'E' || c == 'e') {
        buffer.data[buffer.len] = c;
        buffer.len++;
        int e = c;
        c = fgetc(source);
        if ((c >= '0' && c <= '9') || c == '+' || c == '-')
          state = STATE_E_SIGN;
        else {
          column--;
          buffer.len--;
          e_pen = e;
        }
        ungetc(c, source);
      } else if (c >= '0' && c <= '9') {
        buffer.data[buffer.len] = c;
        buffer.len++;
      } else if (c == 'd' || c == 'D') {
        buffer.data[buffer.len] = c;
        buffer.len++;
        state = STATE_DOUBLE;
      } else if (c == '_') {
        c = fgetc(source);
        if (c >= '0' && c <= '9') {
          ungetc(c, source);
        } else {
          fprintf(
              stderr,
              "Lexikalna chyba: Zle umiestneny podciarkovnik na riadku %d\n",
              row);
          free_lexer_globals();
          exit(1);
        }
      } else {
        ungetc(c, source);
        column--;
        state = STATE_DOUBLE;
      }
      break;

    case STATE_DOUBLE_LOAD_E:
      bufferCheck();
      c = fgetc(source);
      column++;
      if (c >= '0' && c <= '9') {
        buffer.data[buffer.len] = c;
        buffer.len++;
      } else if (c == 'd' || c == 'D') {
        buffer.data[buffer.len] = c;
        buffer.len++;
        state = STATE_DOUBLE;
      } else if (c == '_') {
        c = fgetc(source);
        if ((c >= '0' && c <= '9') && (buffer.data[buffer.len - 1] >= '0' &&
                                       buffer.data[buffer.len - 1] <= '9')) {
          ungetc(c, source);
        } else {
          fprintf(
              stderr,
              "Lexikalna chyba: Zle umiestneny podciarkovnik na riadku %d\n",
              row);
          free_lexer_globals();
          exit(1);
        }
      } else {
        ungetc(c, source);
        column--;
        state = STATE_DOUBLE;
      }
      break;

    case STATE_DOUBLE:
      fillToken(TOKEN_DOUBLE_LITERAL);
      return token;
      break;

    case STATE_E_SIGN:
      c = fgetc(source);
      column++;
      bufferCheck();

      if (c == '+' || c == '-') {

        buffer.data[buffer.len] = c;
        buffer.len++;
        int next_c = fgetc(source);
        ungetc(next_c, source);

        if (next_c < '0' || next_c > '9') {
          fprintf(stderr,
                  "Lexikalna chyba: Neplatny format exponentu na riadku %d, "
                  "stlpec %d\n",
                  row, column);
          free_lexer_globals();
          exit(1);
        }

        state = STATE_DOUBLE_LOAD_E;
      } else if (c >= '0' && c <= '9') {
        ungetc(c, source);
        column--;
        state = STATE_DOUBLE_LOAD_E;
      } else {
        fprintf(stderr,
                "Lexikalna chyba: Neplatny format exponentu na riadku %d, "
                "stlpec %d\n",
                row, column);
        free_lexer_globals();
        exit(1);
      }
      break;

    case STATE_OP_ASSIGN:
      c = fgetc(source);
      if (c == '=') {
        column++;
        fillToken(TOKEN_OP_EQ);
        return token;
      } else {
        ungetc(c, source);
        fillToken(TOKEN_OP_ASSIGN);
        return token;
      }
      break;

    case STATE_OP_NEQ:
      c = fgetc(source);
      if (c == '=') {
        column++;
        fillToken(TOKEN_OP_NEQ);
        return token;
      } else {
        ungetc(c, source);
        fillToken(TOKEN_OP_NOT);
        return token;
      }
      break;

    case STATE_LESS:
      c = fgetc(source);
      if (c == '=') {
        column++;
        fillToken(TOKEN_OP_LESS_EQ);
        return token;
      } else if (c == '-') {
        column++;
        fillToken(TOKEN_KW_ARROW);
        return token;
      } else {
        ungetc(c, source);
        fillToken(TOKEN_OP_LESS);
        return token;
      }
      break;

    case STATE_MORE:
      c = fgetc(source);
      if (c == '=') {
        column++;
        fillToken(TOKEN_OP_MORE_EQ);
        return token;
      } else {
        ungetc(c, source);
        fillToken(TOKEN_OP_MORE);
        return token;
      }
      break;

    case STATE_UNDERSC_LOAD:
      c = fgetc(source);
      ungetc(c, source);
      if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') || c == '_') {
        buffer.data[buffer.len] = '_';
        buffer.len++;
        state = STATE_IDENTIFIER;
      } else {
        fillToken(TOKEN_OP_UNDERSC);
        return token;
      }
      break;

    case STATE_IDENTIFIER:
      c = fgetc(source);
      if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') || c == '_') {
        buffer.data[buffer.len] = c;
        buffer.len++;
        column++;
      } else {
        ungetc(c, source);
        fillToken(TOKEN_IDENTIFIER);
        return token;
      }
      break;

    case STATE_SLASH:
      c = fgetc(source);
      if (c == '/') {
        column--;
        state = STATE_COMM;
      } else if (c == '*') {
        column--;
        comm_lvl++;
        state = STATE_COMM_LVL;
      } else {
        ungetc(c, source);
        fillToken(TOKEN_OP_DIV);
        return token;
      }
      break;

    case STATE_COMM:
      c = fgetc(source);
      if (c == '\n') {
        column++;
        state = STATE_EOL;
      } else if (c == EOF) {
        column++;
        if (line_has_token) {
          line_has_token = 0;
          state = STATE_EOF;
          fillToken(TOKEN_EOL);
          return token;
        }
        state = STATE_EOF;
      }
      break;

    case STATE_COMM_LVL:
      c = fgetc(source);
      column++;
      if (c == '*') {
        c = fgetc(source);
        if (c == '/') {
          comm_lvl--;
          column++;
        } else
          ungetc(c, source);

        if (comm_lvl == 0) {
          if (comm_eol != 0)
            state = STATE_COMM_EOL;
          else
            state = STATE_READ;
        }
      } else if (c == '/') {
        c = fgetc(source);
        if (c == '*') {
          comm_lvl++;
          column++;
        } else
          ungetc(c, source);
      } else if (c == EOF) {
        fprintf(stderr,
                "Lexikalna chyba: Neukonceny blokovy komentar (EOF na riadku "
                "%d, stlpec %d)\n",
                row, column);
        free_lexer_globals();
        exit(1);
      } else if (c == '\n') {
        comm_eol++;
        row++;
        column = 1;
      }
      break;

    case STATE_COMM_EOL:
      column += comm_eol;
      comm_eol = 0;
      c = fgetc(source);
      column++;
      if (c == '\n')
        state = STATE_EOL;
      else if (c != '\n' && c != ' ' && c != EOF) {
        fprintf(stderr,
                "Lexikalna chyba: Nepovoleny znak za viacriadkovym blokovym "
                "komentarom na riadku %d, stlpec %d\n",
                row, column);
        free_lexer_globals();
        exit(1);
      } else if (c == EOF) {
        state = STATE_EOF;
      }
      break;

    case STATE_NUM_ZERO:
      c = fgetc(source);
      if (c == '.' || c == 'e' || c == 'E') {
        ungetc(c, source);
        buffer.data[buffer.len] = '0';
        buffer.len++;
        state = STATE_NUM;
      } else if (c == '_' || (c >= '0' && c <= '9')) {
        ungetc(c, source);
        fprintf(stderr,
                "Lexikalna chyba: Nenulovy literal nesmie zacinat nulou na "
                "riadku %d, stlpec %d\n",
                row, column);
        free_lexer_globals();
        exit(1);
      } else {
        ungetc(c, source);
        buffer.data[buffer.len] = '0';
        buffer.len++;
        state = STATE_INT;
      }
      break;
    
    case (STATE_INTERP_LOAD):
      c = fgetc(source);
      bufferCheck();
      if (interp_multiline && c == '\n') {
        row++;
        column = 1;
      } else {
        column++;
      }

      if (c == '$') {
        int next_c = fgetc(source);
        column++;
        if (next_c == '$') {
          buffer.data[buffer.len++] = '$';
        } else if (next_c == '{') {
          buffer.data[buffer.len] = '\0';
          fillToken(TOKEN_STRING_LITERAL);
          enqueue_token(TOKEN_OP_PLUS, NULL);
          enqueue_token(TOKEN_IDENTIFIER, "toStr");
          enqueue_token(TOKEN_LPAR, NULL);
          in_interp_expr = 1;
          state = STATE_READ;
          return token;
        } else if ((next_c >= 'a' && next_c <= 'z') ||
                   (next_c >= 'A' && next_c <= 'Z') || next_c == '_') {
          char id_buf[256];
          int id_len = 0;
          id_buf[id_len++] = (char)next_c;
          while (1) {
            int ic = fgetc(source);
            if ((ic >= '0' && ic <= '9') || (ic >= 'a' && ic <= 'z') ||
                (ic >= 'A' && ic <= 'Z') || ic == '_') {
              column++;
              if (id_len < 255) {
                id_buf[id_len++] = (char)ic;
              }
            } else {
              ungetc(ic, source);
              break;
            }
          }
          id_buf[id_len] = '\0';
          if (id_len == 1 && id_buf[0] == '_') {
            fprintf(stderr,
                    "Lexikalna chyba: Neplatny identifikator v interpolacii na "
                    "riadku %d, stlpec %d\n",
                    row, column);
            free_lexer_globals();
            exit(1);
          }
          buffer.data[buffer.len] = '\0';
          fillToken(TOKEN_STRING_LITERAL);
          enqueue_token(TOKEN_OP_PLUS, NULL);
          enqueue_token(TOKEN_IDENTIFIER, "toStr");
          enqueue_token(TOKEN_LPAR, NULL);
          enqueue_token(TOKEN_IDENTIFIER, id_buf);
          enqueue_token(TOKEN_RPAR, NULL);
          enqueue_token(TOKEN_OP_PLUS, NULL);
          return token;
        } else {
          fprintf(stderr,
                  "Lexikalna chyba: Neplatna interpolacia na riadku %d, stlpec "
                  "%d\n",
                  row, column);
          free_lexer_globals();
          exit(1);
        }
      } else if (!interp_multiline && c == '\\') {
        int esc = fgetc(source);
        column++;
        if (esc == 'n')
          buffer.data[buffer.len++] = '\n';
        else if (esc == 't')
          buffer.data[buffer.len++] = '\t';
        else if (esc == 'r')
          buffer.data[buffer.len++] = '\r';
        else if (esc == '"')
          buffer.data[buffer.len++] = '"';
        else if (esc == '\\')
          buffer.data[buffer.len++] = '\\';
        else {
          fprintf(stderr,
                  "Lexikalna chyba: Zly escape na riadku %d, stlpec %d\n", row,
                  column);
          free_lexer_globals();
          exit(1);
        }
      } else if (c == '"') {
        if (!interp_multiline) {
          buffer.data[buffer.len] = '\0';
          fillToken(TOKEN_STRING_LITERAL);
          enqueue_token(TOKEN_RPAR, NULL);
          state = STATE_READ;
          return token;
        } else {
          c = fgetc(source);
          if (c == '"') {
            c = fgetc(source);
            if (c == '"') {
              column += 2;
              buffer.data[buffer.len] = '\0';
              fillToken(TOKEN_STRING_LITERAL);
              enqueue_token(TOKEN_RPAR, NULL);
              state = STATE_READ;
              return token;
            } else {
              ungetc(c, source);
              column++;
              bufferCheck();
              buffer.data[buffer.len++] = '"';
              bufferCheck();
              buffer.data[buffer.len++] = '"';
            }
          } else {
            ungetc(c, source);
            buffer.data[buffer.len++] = '"';
          }
        }
      } else if ((!interp_multiline && c == '\n') || c == EOF) {
        fprintf(stderr,
                "Lexikalna chyba: Neukonceny interpolovany retazec na riadku "
                "%d, stlpec %d\n",
                row, column);
        free_lexer_globals();
        exit(1);
      } else {
        buffer.data[buffer.len++] = c;
      }
      break;

    default:
      fprintf(stderr,
              "Lexikalna chyba: Neplatny stav lexera na riadku %d, "
              "stlpec %d\n",
              row, column);
      free_lexer_globals();
      exit(1);
      break;
    }
  }
}

token_array get_all_tokens(FILE *source) {
  token_array arr;
  arr.size = 64;
  arr.pos = 0;
  arr.token_data = malloc(arr.size * sizeof(Token));

  while (1) {
    Token t = get_token(source);
    if (arr.pos >= arr.size) {
      arr.size *= 2;
      arr.token_data = realloc(arr.token_data, arr.size * sizeof(Token));
    }
    arr.token_data[arr.pos] = t;
    arr.pos++;
    if (t.type == TOKEN_EOF) {
      break;
    }
  }

  return arr;
}

void free_lexer_memory(token_array *arr) {
  for (int i = 0; i < arr->pos; i++) {
    if (arr->token_data[i].data != NULL) {
      free(arr->token_data[i].data);
      arr->token_data[i].data = NULL;
    }
  }
  if (arr->token_data != NULL) {
    free(arr->token_data);
    arr->token_data = NULL;
  }
  if (buffer.data != NULL) {
    free(buffer.data);
    buffer.data = NULL;
  }
  if (stack.data != NULL) {
    free(stack.data);
    stack.data = NULL;
  }
}