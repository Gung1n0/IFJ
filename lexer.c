//DORIESIT @ ERROR

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef enum
{
    TOKEN_IDENTIFIER,     // napr. 'moj_vysledok'
    TOKEN_INT_LITERAL,    // napr. 42
    TOKEN_DOUBLE_LITERAL, // napr. 3.14
    TOKEN_STRING_LITERAL, // napr. "ahoj"

    TOKEN_KW_DEF,
    TOKEN_KW_DO,
    TOKEN_KW_ELSE,
    TOKEN_KW_IF,
    TOKEN_KW_INT,
    TOKEN_KW_STR,
    TOKEN_KW_DOUBLE,
    TOKEN_KW_MAIN,
    TOKEN_KW_IMPORT,
    TOKEN_KW_UNIT,
    TOKEN_KW_VAL,
    TOKEN_KW_VAR,
    TOKEN_KW_RETURN,
    TOKEN_KW_WHILE,
    TOKEN_KW_THEN,

    TOKEN_OP_PLUS,    // +
    TOKEN_OP_MINUS,   // -
    TOKEN_OP_MUL,     // *
    TOKEN_OP_DIV,     // /
    TOKEN_OP_ASSIGN,  // =
    TOKEN_OP_EQ,      // ==
    TOKEN_OP_NEQ,     // !=
    TOKEN_OP_LESS,    // <
    TOKEN_OP_MORE,    // >
    TOKEN_OP_LESSEQ,  // <=
    TOKEN_OP_MOREEQ,  // >=
    TOKEN_OP_UNDERSC, // _

    TOKEN_LPAR,  // (
    TOKEN_RPAR,  // )
    TOKEN_COLON, // :
    TOKEN_COMMA, // ,
    TOKEN_DOT,   // .
    TOKEN_AT,    // @

    TOKEN_EOL, // \n
    TOKEN_EOF, // koniec subou
    TOKEN_IND, // vacsie odsadenie
    TOKEN_DED  // mensie odsadenie
} TokenType;

typedef struct
{
    TokenType type;
    char *data;
    int row;
    int column;
} Token;

typedef enum
{
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
} State;

typedef struct
{
    int *data;
    int top;
    int capacity;
} IndentStack;

typedef struct
{
    char *data;
    int len;
    int capacity;
} Buffer;

void bufferCheck()
{
    if (buffer.len + 1 >= buffer.capacity)
    {
        buffer.capacity *= 2;
        buffer.data = realloc(buffer.data, buffer.capacity * sizeof(char));
    }
}

void fillToken(TokenType type)
{
    if (buffer.data != NULL){
        token.data = malloc(buffer.len + 1);
        for (int i = 0; i < buffer.len; i++)
        {
            token.data[i] = buffer.data[i];
        }
        token.data[buffer.len] = '\0';
    }
    token.column = column;
    token.row = row;
    token.type = type;
}

static int line_beg = 1;
static IndentStack stack = {NULL, 0, 0};
static int space_count = 0;
static int row = 1;
static int column = 1;
static int pen_dot = 0;
Buffer buffer = {NULL, 0, 32};
Token token;
State state;
int e_pen = 0;
int comm_lvl;
int comm_eol;

Token get_token(FILE *source)
{
    buffer.len = 0;
    if (stack.data == NULL)
    {
        stack.capacity = 10;
        stack.data = malloc(stack.capacity * sizeof(int));
        stack.top = 0;
        stack.data[0] = 0;
    }

    if (line_beg)
    {
        if (space_count < stack.data[stack.top])
            state = STATE_DED;
        else
            state = STATE_START_LINE;
    }
    else
    {
        state = STATE_READ;
    }

    buffer.data = malloc(buffer.capacity * sizeof(char));

    if (pen_dot == 1){
        pen_dot = 0;
        column++;
        fillToken(TOKEN_DOT);
        return token;
    }

    while (1)
    {
        int c;
        switch (state)
        {
        case (STATE_START_LINE):
            c = fgetc(source);
            column++;
            if (c == EOF)
            {
                state = STATE_EOF;
            }
            else if (c == ' ')
            {
                space_count++;
            }
            else if (c == '\n')
            {
                state = STATE_EOL;
            }
            else
            {
                ungetc(c, source);
                column--;
                state = STATE_SPACE;
            }
            break;
        case (STATE_SPACE):
            if (space_count == stack.data[stack.top])
            {
                line_beg = 0;
                state = STATE_READ;
            }
            else if (space_count > stack.data[stack.top])
                state = STATE_IND;
            else if (space_count < stack.data[stack.top])
                state = STATE_DED;
            break;
        case (STATE_IND):
            if (stack.top + 1 == stack.capacity)
            {
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
            fillToken(TOKEN_DED);
            return token;
            break;
        case (STATE_EOL):
            line_beg = 1;
            space_count = 0;
            fillToken(TOKEN_EOL);
            column = 1;
            row++;
            return token;
            break;
        case (STATE_EOF):
            fillToken(TOKEN_EOF);
            return token;
            break;
        case (STATE_READ):
            c = fgetc(source);
            column++;
            if (c == '"')
            {
                ungetc(c, source);
                column--;
                state = STATE_STRING_LOAD;
            }
            else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            {
                ungetc(c, source);
                column--;
                state = STATE_CHAR;
            }
            else if (c >= '0' && c <= '9')
            {
                if (c == '0') state = STATE_NUM_ZERO;
                else {
                    ungetc(c, source);
                    column--;
                    state = STATE_NUM;
                }
            }
            else if (c == '+'){
                fillToken(TOKEN_OP_PLUS);
                return token;
            } else if (c == '-'){
                fillToken(TOKEN_OP_MINUS);
                return token;
            } else if (c == '*'){
                fillToken(TOKEN_OP_MUL);
                return token;
            } else if (c == '/') state = STATE_SLASH;
            else if (c == '=')
                state = STATE_OP_ASSIGN;
            else if (c == '!')
                state = STATE_OP_NEQ;
            else if (c == '<')
                state = STATE_LESS;
            else if (c == '>')
                state = STATE_MORE;
            else if (c == '_') state = STATE_UNDERSC_LOAD;
            else if (c == '('){
                fillToken(TOKEN_LPAR);
                return token;
            } else if (c == ')'){
                fillToken(TOKEN_RPAR);
                return token;
            } else if (c == ':'){
                fillToken(TOKEN_COLON);
                return token;
            } else if (c == ','){
                fillToken(TOKEN_COMMA);
                return token;
            }else if (c == '.'){
                fillToken(TOKEN_DOT);
                return token;
            }else if (c == '@'){
                fillToken(TOKEN_AT);
                return token;
            } else {
                fprintf(stderr, "Lexikalna chyba: Neplatny znak\n");
                exit(1);
            }
            break;
        case (STATE_STRING_LOAD):
            c = fgetc(source);
            column++;
            bufferCheck();
            if (c == '"')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
                buffer.data[buffer.len] = '\0';
                state = STATE_STRING;
            }
            else if (c == EOF)
            {
                // ERROR
                state = STATE_EOF;
            }
            else
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
            }
            break;
        case (STATE_STRING):
            fillToken(TOKEN_STRING_LITERAL);
            return token;
            break;
        case (STATE_CHAR):
            if (e_pen != 0){
                c = e_pen;
                e_pen = 0;
            }
            else c = fgetc(source);
            column++;
            bufferCheck();
            if ((c >= '0' && c <= '9') || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
            }
            else
            {
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
                    fillToken(TOKEN_KW_STR);
                else if (strcmp(buffer.data, "Double") == 0)
                    fillToken(TOKEN_KW_DOUBLE);
                else if (strcmp(buffer.data, "main") == 0)
                    fillToken(TOKEN_KW_MAIN);
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
                else
                {
                    fillToken(TOKEN_IDENTIFIER);
                }
                return token;
            }
            break;
        case STATE_NUM:
            c = fgetc(source);
            column++;
            bufferCheck();
            if (c >= '0' && c <= '9')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
            } else if (c == '.')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
                state = STATE_DOUBLE_LOAD_DOT;
            } else if (c == 'e' || c == 'E')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
                state = STATE_E_SIGN;
            } else if (c == '_')
            {
                c = fgetc(source);
                if (c >= '0' && c <= '9') {
                    ungetc(c, source);
                } else {
                    fprintf(stderr, "Lexikalna chyba: Zle umiestneny podciarkovnik na riadku %d\n", row);
                    exit(1);
                }
            } else
            {
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
            if (c >= '0' && c <= '9')
            {
                bufferCheck();
                buffer.data[buffer.len] = c;
                buffer.len++;
                state = STATE_DOUBLE_LOAD;
            }
            else
            {
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
            if (c == 'E' || c == 'e')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
                int e = c;
                c = fgetc(source);
                if ((c >= '0' && c <= '9') || c == '+' || c == '-') state = STATE_E_SIGN;
                else{
                    column--;
                    buffer.len--;
                    e_pen = e;
                }
                ungetc(c, source);
            } else if (c >= '0' && c <= '9')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
            } else if (c == 'd' || c == 'D'){
                buffer.data[buffer.len] = c;
                buffer.len++;
                state = STATE_DOUBLE;
            } else if (c == '_')
            {
                c = fgetc(source);
                if (c >= '0' && c <= '9') {
                    ungetc(c, source);
                } else {
                    fprintf(stderr, "Lexikalna chyba: Zle umiestneny podciarkovnik na riadku %d\n", row);
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
            if (c >= '0' && c <= '9')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
            } else if(c == 'd' || c == 'D'){
                buffer.data[buffer.len] = c;
                buffer.len++;
                state = STATE_DOUBLE;
            } else if (c == '_')
            {
                c = fgetc(source);
                if ((c >= '0' && c <= '9') && (buffer.data[buffer.len-1] >= '0' && buffer.data[buffer.len-1] <= '9')) {
                    ungetc(c, source);
                } else {
                    fprintf(stderr, "Lexikalna chyba: Zle umiestneny podciarkovnik na riadku %d\n", row);
                    exit(1);
                }
            }else{
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
            state = STATE_DOUBLE_LOAD_E;
        } 
        else if (c >= '0' && c <= '9') {
            ungetc(c, source);
            column--;
            state = STATE_DOUBLE_LOAD_E;
        } 
        else {
            fprintf(stderr, "Lexikalna chyba: Neplatny format exponentu na riadku %d, stlpec %d\n", row, column);
            exit(1);
        }
        break;
        case STATE_OP_ASSIGN:
            c = fgetc(source);
            if (c == '=')
            {
                column++;
                fillToken(TOKEN_OP_EQ);
                return token;
            } else{
                ungetc(c, source);
                fillToken(TOKEN_OP_ASSIGN);
                return token;
            }
            break;
        case STATE_OP_NEQ:
            c = fgetc(source);
            if (c == '=')
            {
                column++;
                fillToken(TOKEN_OP_NEQ);
                return token;
            } else{
                ungetc(c, source);
                column++;
                fprintf(stderr, "Lexikalna chyba: Neplatny znak na riadku %d, v stlpci %d\n", row, column);
                exit(1);
            }
            break;
        case STATE_LESS:
            c = fgetc(source);
            if (c == '=')
            {
                column++;
                fillToken(TOKEN_OP_LESSEQ);
                return token;
            } else{
                ungetc(c, source);
                fillToken(TOKEN_OP_LESS);
                return token;
            }
            break;
        case STATE_MORE:
            c = fgetc(source);
            if (c == '=')
            {
                column++;
                fillToken(TOKEN_OP_MOREEQ);
                return token;
            } else{
                ungetc(c, source);
                fillToken(TOKEN_OP_MORE);
                return token;
            }
            break;
        case STATE_UNDERSC_LOAD:
            c = fgetc(source);
            ungetc(c, source);
            if ((c >= '0' && c <= '9') || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_')
            {
                buffer.data[buffer.len] = '_';
                buffer.len++;
                state = STATE_IDENTIFIER;
            } else{
                fillToken(TOKEN_OP_UNDERSC);
                return token;
            }
            break;
        case STATE_IDENTIFIER:
            c = fgetc(source);
            if ((c >= '0' && c <= '9') || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_')
            {
                buffer.data[buffer.len] = c;
                buffer.len++;
                column++;
            } else{
                ungetc(c, source);
                fillToken(TOKEN_IDENTIFIER);
                return token;
            }
            break;
        case STATE_SLASH:
            c = fgetc(source);
            if (c == '/')
            {
                column--;
                state = STATE_COMM;
            } else if(c == '*'){
                column--;
                comm_lvl++;
                state = STATE_COMM_LVL;
            } else{
                fillToken(TOKEN_OP_DIV);
                return token;
            }
            break;
        case STATE_COMM:
            c = fgetc(source);
            if (c == '\n')
            {
                column++;
                state = STATE_EOL;
            } else if (c == EOF)
            {
                column++;
                state = STATE_EOF;
            } 
            break;
        case STATE_COMM_LVL:
            c = fgetc(source);
            column++;
            if (c == '*')
            {
                c = fgetc(source);
                if (c == '/'){
                    comm_lvl--;
                    column++;
                }
                else ungetc(c, source);
                if (comm_lvl == 0)
                {
                    if (comm_eol != 0) state = STATE_COMM_EOL;
                }
            } else if (c == '/')
            {
                c = fgetc(source);
                if (c == '*'){
                    comm_lvl++;
                    column++;
                }
                else ungetc(c, source);
            } else if(c == EOF){
                fprintf(stderr, "Lexikalna chyba: Neukonceny blokovy komentar (EOF na riadku %d, stlpec %d)\n", row, column);
                exit(1);   
            } else if (c == '\n'){
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
            if (c == '\n') state = STATE_EOL;
            else if (c != '\n' && c != ' ' && c != EOF){
                fprintf(stderr, "Lexikalna chyba: Nepovoleny znak za viacriadkovym blokovym komentarom na riadku %d, stlpec %d\n", row, column);
                exit(1); 
            } else if (c == EOF){
                state = STATE_EOF;
            }
            break;
        case STATE_NUM_ZERO:
            c = fgetc(source);
            if (c == '.' || c == 'e' || c == 'E')
            {
                ungetc(c, source);
                buffer.data[buffer.len] = '0';
                buffer.len++;
                state = STATE_NUM;
            } else if (c >= '0' && c <= '9'){
                ungetc(c, source);
                fprintf(stderr, "Lexikalna chyba: Nenulovy literal nesmie zacinat nulou na riadku %d, stlpec %d\n", row, column);
                exit(1);
            } else {
                ungetc(c, source);
                buffer.data[buffer.len] = '0';
                buffer.len++;
                state = STATE_INT;
            }
            
    }
}
