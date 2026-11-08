#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef enum{
    TOKEN_IDENTIFIER,     // napr. 'moj_vysledok'
    TOKEN_INT_LITERAL,    // napr. 42
    TOKEN_DOUBLE_LITERAL, // napr. 3.14
    TOKEN_STRING_LITERAL, // napr. "ahoj"

    TOKEN_KW_DEF,
    TOKEN_KW_DO,
    TOKEN_KW_ELSE,
    TOKEN_KW_IF,
    TOKEN_KW_INT,

    TOKEN_OP_PLUS,        // +
    TOKEN_OP_MINUS,       // -
    TOKEN_OP_MUL,         // *
    TOKEN_OP_DIV,         // /
    TOKEN_OP_ASSIGN,      // =
    TOKEN_OP_EQ,          // ==
    TOKEN_OP_NEQ,         // !=
    TOKEN_OP_LESS,        // <
    TOKEN_OP_MORE,        // >
    TOKEN_OP_UNDERSC,     // _

    TOKEN_LPAR,           // (
    TOKEN_RPAR,           // )
    TOKEN_COLON,          // :
    TOKEN_COMMA,          // ,
    TOKEN_DOT,            // .
    TOKEN_AT,             // @

    TOKEN_EOL,            // \n
    TOKEN_EOF,            // Koniec súboru
    TOKEN_IND,         // Zväčšenie odsadenia
    TOKEN_DED          // Zmenšenie odsadenia
} TokenType;

typedef struct{
    TokenType type;
    int i_val;
    double d_val; 
    char *s_val;  
} Token;

typedef enum{
    STATE_START_LINE,
    STATE_SPACE,
    STATE_EOL,
    STATE_IND,
    STATE_DED,
    STATE_READ,
    STATE_ID, 
    STATE_INT,
    STATE_STRING,
    STATE_STRING_LOAD,
    STATE_CHAR,
    STATE_NUM,
    STATE_PLUS,
    STATE_MINUS,
    STATE_MUL,
    STATE_DIV,
    STATE_OP_ASSIGN,
    STATE_OP_NEQ,
    STATE_LESS,
    STATE_MORE,
    STATE_UNDERSC,
    STATE_LPAR,
    STATE_RPAR,
    STATE_COLON,
    STATE_COMMA,
    STATE_DOT,
    STATE_AT,
    STATE_EOF
} State;

typedef struct{
    int *data;
    int top;
    int capacity;
} IndentStack;

typedef struct {
    char *data;
    int len;
    int capacity;
} Buffer;

Token get_token(FILE *source){
    static int line_beg = 1;
    static IndentStack stack = {NULL, 0, 0};
    static int space_count = 0;


    if (stack.data == NULL) {
        stack.capacity = 10;
        stack.data = malloc(stack.capacity * sizeof(int));
        stack.top = 0;
        stack.data[0] = 0;
    }

    Token token;
    State state;

    if(line_beg){
        if (space_count < stack.data[stack.top]) state = STATE_DED;
        else state = STATE_START_LINE;
    } else {
        state = STATE_READ;
    }

    while(1){
        int c;
        switch(state){
            case(STATE_START_LINE):
                c = fgetc(source);
                if (c == EOF)
                {
                    state = STATE_EOF;
                } else if (c == ' '){
                    space_count++;
                } else if(c == '\n'){
                    state = STATE_EOL;
                } else {
                    ungetc(c, source);
                    state = STATE_SPACE;
                }  
                break;
            case(STATE_SPACE):
                if (space_count == stack.data[stack.top]){
                    line_beg = 0;
                    state = STATE_READ;
                }
                else if (space_count > stack.data[stack.top]) state = STATE_IND;
                else if (space_count < stack.data[stack.top]) state = STATE_DED;
                break;
            case(STATE_IND):
                if (stack.top + 1 == stack.capacity)
                {
                    stack.capacity *= 2;
                    stack.data = realloc(stack.data, stack.capacity * sizeof(int));
                }
                stack.data[stack.top+1] = space_count;
                stack.top++;
                space_count = 0;
                line_beg = 0;
                token.type = TOKEN_IND;
                return token;
                break;
            case(STATE_DED):
                stack.top -= 1;
                token.type = TOKEN_DED;
                return token;
                break;
            case(STATE_EOL):
                line_beg = 1;
                space_count = 0;
                token.type = TOKEN_EOL;
                return token;
                break;
            case(STATE_EOF):
                token.type = TOKEN_EOF;
                return token;
                break;
            case(STATE_READ):
                c = fgetc(source);
                if('"') state = STATE_STRING_LOAD;
                else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')){
                    ungetc(c, source);
                    state = STATE_CHAR;
                } else if (c >= '0' && c <= '9') state = STATE_NUM;
                else if (c == '+') state = STATE_PLUS;
                else if (c == '-') state = STATE_MINUS;
                else if (c == '*') state = STATE_MUL;
                else if (c == '/') state = STATE_DIV;
                else if (c == '=') state = STATE_OP_ASSIGN;
                else if (c == '!') state = STATE_OP_NEQ;
                else if (c == '<') state = STATE_LESS;
                else if (c == '>') state = STATE_MORE;
                else if (c == '_') state = STATE_UNDERSC;
                else if (c == '(') state = STATE_LPAR;
                else if (c == ')') state = STATE_RPAR;
                else if (c == ':') state = STATE_COLON;
                else if (c == ',') state = STATE_COMMA;
                else if (c == '.') state = STATE_DOT;
                else if (c == '@') state = STATE_AT;
                break;
            case(STATE_STRING_LOAD):
                c = getc(source);
                Buffer str;
                if (str.data == NULL)
                {
                    str.capacity = 32;
                    str.data = malloc(str.capacity * sizeof(char));
                    str.len = 0;
                }
                str.data[str.len] = c;
                str.len++;
        }

    }
}

