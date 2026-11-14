/*
 * IFJ26 - testy parsera (tests.c)
 * Autori: <doplň mená a loginy>
 *
 * AI GENERATED: prompt: "nakolko ti pravidla dovoluju generovat testy vygeneruj
 * testy pre kazdu funkciu, v kazdej funkcii testuj edge case, nahodne hodnoty,
 * null a spravne hodnoty" + "vygeneruj znova" (oprava: parse_program namiesto
 * parse_head_funtion, bez -Waddress warningu, NULL testy volitelne)
 *
 * Preklad:  gcc -std=gnu11 -g -Wall -Wextra -fsanitize=address,undefined -o tests tests.c
 * Spustenie: ASAN_OPTIONS=detect_leaks=0 ./tests
 * NULL testy (volitelne): pridaj -DTEST_NULL
 * Chybove cesty (print_error -> exit) sa testuju vo fork()-nutom procese.
 */
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>

#define main parser_main
#include "Parser.c"
#undef main

static int g_run = 0, g_fail = 0;

#define CHECK(c)                                                \
    do                                                          \
    {                                                           \
        g_run++;                                                \
        if (!(c))                                               \
        {                                                       \
            g_fail++;                                           \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); \
        }                                                       \
    } while (0)

/* telo bezi v potomkovi; ocakavame exit kod `code` */
#define EXPECT_EXIT(code, ...)                               \
    do                                                       \
    {                                                        \
        fflush(NULL);                                        \
        pid_t pid_ = fork();                                 \
        if (pid_ == 0)                                       \
        {                                                    \
            if (freopen("/dev/null", "w", stderr))           \
            {                                                \
            }                                                \
            __VA_ARGS__;                                     \
            _exit(0);                                        \
        }                                                    \
        int st_ = 0;                                         \
        waitpid(pid_, &st_, 0);                              \
        CHECK(WIFEXITED(st_) && WEXITSTATUS(st_) == (code)); \
    } while (0)

/* potomok nesmie spadnut na signal (napr. SIGSEGV pri NULL) */
#define EXPECT_NOCRASH(...)                        \
    do                                             \
    {                                              \
        fflush(NULL);                              \
        pid_t pid_ = fork();                       \
        if (pid_ == 0)                             \
        {                                          \
            if (freopen("/dev/null", "w", stderr)) \
            {                                      \
            }                                      \
            __VA_ARGS__;                           \
            _exit(0);                              \
        }                                          \
        int st_ = 0;                               \
        waitpid(pid_, &st_, 0);                    \
        CHECK(!WIFSIGNALED(st_));                  \
    } while (0)

/* rucne zostavene tokeny */
#define T(ty, d) {(ty), (char *)(d), 1, 1}
#define ARR(name, ...)                \
    token name##_t[] = {__VA_ARGS__}; \
    token_array name = {name##_t, (int)(sizeof(name##_t) / sizeof(token)), 0}

/* ---------- mini lexer len pre testy (EOL/IND/DED ako ocakava parser) ---------- */
static const struct
{
    const char *w;
    TokenType t;
} KW[] = {
    {"def", TOKEN_KW_DEF}, {"do", TOKEN_KW_DO}, {"else", TOKEN_KW_ELSE}, {"if", TOKEN_KW_IF}, {"Int", TOKEN_KW_INT}, {"import", TOKEN_KW_IMPORT}, {"Unit", TOKEN_KW_UNIT}, {"val", TOKEN_KW_VAL}, {"var", TOKEN_KW_VAR}, {"return", TOKEN_KW_RETURN}, {"while", TOKEN_KW_WHILE}, {"then", TOKEN_KW_THEN}, {"Double", TOKEN_KW_DOUBLE}, {"String", TOKEN_KW_STRING}};

static char *dup_or_null(const char *s) { return s ? strduplicate(s) : NULL; }

#define ADD(ty, d)                       \
    do                                   \
    {                                    \
        t[n].type = (ty);                \
        t[n].data = dup_or_null(d);      \
        t[n].row = row;                  \
        t[n].column = (int)(p - ls) + 1; \
        n++;                             \
    } while (0)

static token_array lex(const char *src)
{
    size_t cap = strlen(src) * 2 + 16;
    token *t = calloc(cap, sizeof(token));
    int n = 0, row = 1, top = 0, ind[64] = {0};
    const char *p = src, *ls = src;
    while (*p)
    {
        ls = p;
        int col0 = 0;
        while (*p == ' ')
        {
            p++;
            col0++;
        }
        if (*p == '\n')
        {
            p++;
            row++;
            continue;
        }
        if (p[0] == '/' && p[1] == '/')
        {
            while (*p && *p != '\n')
                p++;
            if (*p)
                p++;
            row++;
            continue;
        }
        if (*p == '\0')
            break;
        if (col0 > ind[top])
        {
            ind[++top] = col0;
            ADD(TOKEN_IND, NULL);
        }
        while (col0 < ind[top])
        {
            top--;
            ADD(TOKEN_DED, NULL);
        }
        while (*p && *p != '\n')
        {
            if (*p == ' ')
            {
                p++;
                continue;
            }
            if (isdigit((unsigned char)*p))
            {
                char b[64];
                int i = 0;
                TokenType ty = TOKEN_INT_LITERAL;
                while (isdigit((unsigned char)*p))
                    b[i++] = *p++;
                if (*p == '.' && isdigit((unsigned char)p[1]))
                {
                    ty = TOKEN_DOUBLE_LITERAL;
                    b[i++] = *p++;
                    while (isdigit((unsigned char)*p))
                        b[i++] = *p++;
                }
                b[i] = 0;
                ADD(ty, b);
                continue;
            }
            if (isalpha((unsigned char)*p) || *p == '_')
            {
                char b[64];
                int i = 0;
                TokenType ty = TOKEN_IDENTIFIER;
                while (isalnum((unsigned char)*p) || *p == '_')
                    b[i++] = *p++;
                b[i] = 0;
                for (size_t k = 0; k < sizeof KW / sizeof *KW; k++)
                    if (!strcmp(KW[k].w, b))
                        ty = KW[k].t;
                ADD(ty, b);
                continue;
            }
            if (*p == '"')
            {
                char b[256];
                int i = 0;
                p++;
                while (*p && *p != '"')
                    b[i++] = *p++;
                b[i] = 0;
                if (*p)
                    p++;
                ADD(TOKEN_STRING_LITERAL, b);
                continue;
            }
            char c = *p, d = p[1];
            TokenType ty = TOKEN_EOF;
            const char *s = "";
            int len = 1;
            switch (c)
            {
            case '+':
                ty = TOKEN_OP_PLUS;
                s = "+";
                break;
            case '-':
                ty = TOKEN_OP_MINUS;
                s = "-";
                break;
            case '*':
                ty = TOKEN_OP_MUL;
                s = "*";
                break;
            case '/':
                ty = TOKEN_OP_DIV;
                s = "/";
                break;
            case '(':
                ty = TOKEN_LPAR;
                s = "(";
                break;
            case ')':
                ty = TOKEN_RPAR;
                s = ")";
                break;
            case ':':
                ty = TOKEN_COLON;
                s = ":";
                break;
            case ',':
                ty = TOKEN_COMMA;
                s = ",";
                break;
            case '.':
                ty = TOKEN_DOT;
                s = ".";
                break;
            case '@':
                ty = TOKEN_AT;
                s = "@";
                break;
            case ';':
                ty = TOKEN_SEMICOLON;
                s = ";";
                break;
            case '=':
                if (d == '=')
                {
                    ty = TOKEN_OP_EQ;
                    s = "==";
                    len = 2;
                }
                else
                {
                    ty = TOKEN_OP_ASSIGN;
                    s = "=";
                }
                break;
            case '!':
                ty = TOKEN_OP_NEQ;
                s = "!=";
                len = 2;
                break;
            case '<':
                if (d == '=')
                {
                    ty = TOKEN_OP_LESS_EQ;
                    s = "<=";
                    len = 2;
                }
                else
                {
                    ty = TOKEN_OP_LESS;
                    s = "<";
                }
                break;
            case '>':
                if (d == '=')
                {
                    ty = TOKEN_OP_MORE_EQ;
                    s = ">=";
                    len = 2;
                }
                else
                {
                    ty = TOKEN_OP_MORE;
                    s = ">";
                }
                break;
            default:
                p++;
                continue;
            }
            ADD(ty, s);
            p += len;
        }
        ADD(TOKEN_EOL, NULL);
        if (*p == '\n')
            p++;
        row++;
    }
    while (top > 0)
    {
        top--;
        ADD(TOKEN_DED, NULL);
    }
    ADD(TOKEN_EOF, NULL);
    token_array a = {t, n, 0};
    return a;
}

/* lexer s predsadenym prologom (pre parse_program) */
static token_array lexp(const char *body)
{
    char *s = malloc(strlen(body) + 32);
    strcpy(s, "import ifj26.*\n");
    strcat(s, body);
    token_array a = lex(s);
    free(s);
    return a;
}

/* ---------- pomocne funkcie pre stromy ---------- */
static const char *opname(operator o)
{
    switch (o)
    {
    case add:
        return "+";
    case subtract:
        return "-";
    case multiply:
        return "*";
    case divide:
        return "/";
    case lesser:
        return "<";
    case lesser_or_equal:
        return "<=";
    case greater:
        return ">";
    case greater_or_equal:
        return ">=";
    case equal:
        return "==";
    case not_equal:
        return "!=";
    case negation:
        return "neg";
    default:
        return "?";
    }
}

static void dump(node *n, char *o)
{
    char b[64];
    if (!n)
    {
        strcat(o, "NULL");
        return;
    }
    switch (n->type)
    {
    case node_lit:
        if (n->as.lit.type == type_int)
            snprintf(b, sizeof b, "%d", n->as.lit.i);
        else if (n->as.lit.type == type_double)
            snprintf(b, sizeof b, "%g", n->as.lit.d);
        else
            snprintf(b, sizeof b, "\"%s\"", n->as.lit.s);
        strcat(o, b);
        break;
    case node_id:
        strcat(o, n->as.ident.name);
        break;
    case node_unar:
        strcat(o, "(neg ");
        dump(n->as.unar.operand, o);
        strcat(o, ")");
        break;
    case node_binar:
        strcat(o, "(");
        strcat(o, opname(n->as.binar.op));
        strcat(o, " ");
        dump(n->as.binar.left, o);
        strcat(o, " ");
        dump(n->as.binar.right, o);
        strcat(o, ")");
        break;
    case node_call:
        strcat(o, "(call ");
        strcat(o, n->as.call.name);
        for (int i = 0; i < n->as.call.size; i++)
        {
            strcat(o, " ");
            dump(n->as.call.items[i], o);
        }
        strcat(o, ")");
        break;
    default:
        strcat(o, "?");
    }
}

static unsigned long long eval(node *n)
{
    if (n->type == node_lit)
        return (unsigned long long)n->as.lit.i;
    if (n->type == node_unar)
        return 0ULL - eval(n->as.unar.operand);
    unsigned long long l = eval(n->as.binar.left), r = eval(n->as.binar.right);
    if (n->as.binar.op == add)
        return l + r;
    if (n->as.binar.op == subtract)
        return l - r;
    return l * r;
}

/* referencny vyhodnocovac a generator nahodnych vyrazov (diferencialny test) */
static const char *rp;
static void sk(void)
{
    while (*rp == ' ')
        rp++;
}
static unsigned long long r_expr(void);
static unsigned long long r_factor(void)
{
    sk();
    if (*rp == '-')
    {
        rp++;
        return 0ULL - r_factor();
    }
    if (*rp == '(')
    {
        rp++;
        unsigned long long v = r_expr();
        sk();
        rp++;
        return v;
    }
    unsigned long long v = (unsigned long long)(*rp - '0');
    rp++;
    return v;
}
static unsigned long long r_term(void)
{
    unsigned long long v = r_factor();
    sk();
    while (*rp == '*')
    {
        rp++;
        v *= r_factor();
        sk();
    }
    return v;
}
static unsigned long long r_expr(void)
{
    unsigned long long v = r_term();
    sk();
    while (*rp == '+' || *rp == '-')
    {
        char c = *rp++;
        unsigned long long w = r_term();
        v = (c == '+') ? v + w : v - w;
        sk();
    }
    return v;
}
static void g_expr(char *o, int d);
static void g_factor(char *o, int d)
{
    int r = rand() % 5;
    if (d < 4 && r == 0)
    {
        strcat(o, rand() % 2 ? "- " : "-");
        g_factor(o, d + 1);
    }
    else if (d < 4 && r == 1)
    {
        strcat(o, "(");
        g_expr(o, d + 1);
        strcat(o, ")");
    }
    else
    {
        char b[2] = {(char)('0' + rand() % 10), 0};
        strcat(o, b);
    }
}
static void g_term(char *o, int d)
{
    g_factor(o, d);
    while (rand() % 3 == 0)
    {
        strcat(o, " * ");
        g_factor(o, d);
    }
}
static void g_expr(char *o, int d)
{
    g_term(o, d);
    while (rand() % 3 == 0)
    {
        strcat(o, rand() % 2 ? " + " : " - ");
        g_term(o, d);
    }
}

/* stack pomocne */
static void push_t(stack *s, int term)
{
    stack_item *i = create_stack_item(terminal);
    i->ast.term = term;
    stack_add(i, s);
}
static void push_n(stack *s, node *n)
{
    stack_item *i = create_stack_item(node_item);
    i->ast.n = n;
    stack_add(i, s);
}
static stack *new_st(void)
{
    stack *s = stack_init(2);
    push_t(s, 7);
    return s;
}
static node *lit_i(int v)
{
    node *n = create_node(node_lit, 1, 1);
    n->as.lit.type = type_int;
    n->as.lit.i = v;
    return n;
}

/* ============================== TESTY ============================== */

static void test_strduplicate(void)
{
    char *p = strduplicate("");
    CHECK(p && p[0] == '\0');
    free(p);
    p = strduplicate("abc");
    CHECK(p && strcmp(p, "abc") == 0);
    free(p);
    for (int i = 0; i < 200; i++)
    {
        char buf[65];
        int len = rand() % 64;
        for (int j = 0; j < len; j++)
            buf[j] = (char)('a' + rand() % 26);
        buf[len] = '\0';
        p = strduplicate(buf);
        CHECK(p && p != buf && strcmp(p, buf) == 0);
        free(p);
    }
#ifdef TEST_NULL
    EXPECT_NOCRASH({ strduplicate(NULL); });
#endif
}

static void test_is_keyword(void)
{
    TokenType yes[] = {TOKEN_KW_DEF, TOKEN_KW_DO, TOKEN_KW_ELSE, TOKEN_KW_IF, TOKEN_KW_INT,
                       TOKEN_KW_IMPORT, TOKEN_KW_UNIT, TOKEN_KW_VAL, TOKEN_KW_VAR,
                       TOKEN_KW_RETURN, TOKEN_KW_WHILE, TOKEN_KW_THEN, TOKEN_KW_DOUBLE,
                       TOKEN_KW_STRING};
    TokenType no[] = {TOKEN_IDENTIFIER, TOKEN_INT_LITERAL, TOKEN_DOUBLE_LITERAL,
                      TOKEN_STRING_LITERAL, TOKEN_OP_PLUS, TOKEN_LPAR, TOKEN_EOL,
                      TOKEN_EOF, TOKEN_IND, TOKEN_DED, TOKEN_AT};
    for (size_t i = 0; i < sizeof yes / sizeof *yes; i++)
    {
        token t = {yes[i], NULL, 1, 1};
        CHECK(is_keyword(&t));
    }
    for (size_t i = 0; i < sizeof no / sizeof *no; i++)
    {
        token t = {no[i], NULL, 1, 1};
        CHECK(!is_keyword(&t));
    }
#ifdef TEST_NULL
    EXPECT_NOCRASH({ is_keyword(NULL); });
#endif
}

static void test_token_name(void)
{
    for (int t = TOKEN_IDENTIFIER; t <= TOKEN_DED; t++)
    {
        const char *s = token_name((TokenType)t);
        CHECK(s != NULL && s[0] != '\0');
    }
    CHECK(strcmp(token_name(TOKEN_LPAR), "'('") == 0);
    CHECK(strcmp(token_name(TOKEN_RPAR), "')'") == 0);
    CHECK(strcmp(token_name(TOKEN_EOF), "end of file") == 0);
    CHECK(strcmp(token_name(TOKEN_IDENTIFIER), "identifier") == 0);
}

static void test_skip_peek_get(void)
{
    ARR(a, T(TOKEN_IDENTIFIER, "a"), T(TOKEN_IDENTIFIER, "b"), T(TOKEN_EOF, NULL));
    skip_token(&a);
    CHECK(a.pos == 1);
    skip_token(&a);
    CHECK(a.pos == 2);
    skip_token(&a);
    CHECK(a.pos == 2); /* za koniec sa neposuva */
    skip_token(NULL);  /* NULL je ignorovane */

    a.pos = 0;
    CHECK(peek_token_array(&a, 0) == &a_t[0]);
    CHECK(peek_token_array(&a, 2) == &a_t[2]);
    EXPECT_EXIT(2, { peek_token_array(&a, 5); });

    token *g = get_token_from_array(&a, TOKEN_IDENTIFIER, NULL);
    CHECK(g == &a_t[0] && a.pos == 1);
    g = get_token_from_array(&a, TOKEN_IDENTIFIER, "b");
    CHECK(g == &a_t[1] && a.pos == 2);
    EXPECT_EXIT(2, { get_token_from_array(&a, TOKEN_IDENTIFIER, NULL); }); /* zly typ */
    a.pos = 0;
    EXPECT_EXIT(2, { get_token_from_array(&a, TOKEN_IDENTIFIER, "zzz"); }); /* zle data */
    EXPECT_EXIT(2, { get_token_from_array(&a, TOKEN_KW_DEF, NULL); });
    a.pos = 2;
    EXPECT_EXIT(2, { get_token_from_array(&a, TOKEN_IDENTIFIER, "x"); }); /* data == NULL */
}

static void test_parse_prologue(void)
{
    token_array a = lex("import ifj26.*\n");
    parse_prologue(&a);
    CHECK(a.token_data[a.pos].type == TOKEN_EOF);

    ARR(b, T(TOKEN_EOL, NULL), T(TOKEN_EOL, NULL), T(TOKEN_KW_IMPORT, "import"),
        T(TOKEN_IDENTIFIER, "ifj26"), T(TOKEN_DOT, "."), T(TOKEN_OP_MUL, "*"),
        T(TOKEN_EOL, NULL), T(TOKEN_EOF, NULL));
    parse_prologue(&b);
    CHECK(b.token_data[b.pos].type == TOKEN_EOF);

    const char *bad[] = {"def f() = 1\n", "x\n", "import ifj27.*\n", "import ifj26.\n",
                         "import ifj26*\n", "import .*\n", "", "5\n"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++)
    {
        token_array c = lex(bad[i]);
        EXPECT_EXIT(2, { parse_prologue(&c); });
    }
    ARR(d, T(TOKEN_EOL, NULL), T(TOKEN_EOL, NULL), T(TOKEN_EOF, NULL)); /* len prazdne riadky */
    EXPECT_EXIT(2, { parse_prologue(&d); });
}

static void test_create_free_node(void)
{
    for (int t = node_program; t <= node_call; t++)
    {
        node *n = create_node((node_type)t, 3, 7);
        CHECK(n && n->type == (node_type)t && n->row == 3 && n->column == 7);
        free_node(n); /* vynulovany uzol sa musi dat uvolnit */
    }
    free_node(NULL);

    /* zlozity strom: function(params, block(if bez else, return bez hodnoty, call, lit string)) */
    node *f = create_node(node_function, 1, 1);
    f->as.function.name = strduplicate("f");
    f->as.function.params = calloc(2, sizeof(param));
    f->as.function.param_count = 2;
    f->as.function.params[0].name = strduplicate("a");
    f->as.function.params[1].name = strduplicate("b");
    node *b = create_node(node_block, 2, 1);
    b->as.block.items = malloc(sizeof(node *) * 4);
    b->as.block.size = 4;
    node *iff = create_node(node_if, 2, 1);
    iff->as.if_n.condition = lit_i(1);
    iff->as.if_n.then = create_node(node_block, 3, 1);
    b->as.block.items[0] = iff;                            /* else_n == NULL */
    b->as.block.items[1] = create_node(node_return, 4, 1); /* value == NULL */
    node *c = create_node(node_call, 5, 1);
    c->as.call.name = strduplicate("g");
    c->as.call.items = malloc(sizeof(node *));
    c->as.call.items[0] = lit_i(2);
    c->as.call.size = 1;
    b->as.block.items[2] = c;
    node *s = create_node(node_lit, 6, 1);
    s->as.lit.type = type_string;
    s->as.lit.s = strduplicate("txt");
    b->as.block.items[3] = s;
    f->as.function.body = b;
    free_node(f); /* ASan nesmie hlasit double free / use-after-free */
}

static void test_parse_type_operator(void)
{
    struct
    {
        TokenType t;
        data_types d;
    } ty[] = {
        {TOKEN_KW_INT, type_int}, {TOKEN_KW_UNIT, type_unit}, {TOKEN_KW_DOUBLE, type_double}, {TOKEN_KW_STRING, type_string}, {TOKEN_IDENTIFIER, type_none}, {TOKEN_INT_LITERAL, type_none}, {TOKEN_STRING_LITERAL, type_none}, {TOKEN_EOL, type_none}, {TOKEN_KW_DEF, type_none}};
    for (size_t i = 0; i < sizeof ty / sizeof *ty; i++)
    {
        token t = {ty[i].t, NULL, 1, 1};
        CHECK(parse_type(&t) == ty[i].d);
    }
    struct
    {
        TokenType t;
        operator o;
    } op[] = {
        {TOKEN_OP_PLUS, add}, {TOKEN_OP_MINUS, subtract}, {TOKEN_OP_MUL, multiply}, {TOKEN_OP_DIV, divide}, {TOKEN_OP_EQ, equal}, {TOKEN_OP_NEQ, not_equal}, {TOKEN_OP_LESS, lesser}, {TOKEN_OP_MORE, greater}, {TOKEN_OP_LESS_EQ, lesser_or_equal}, {TOKEN_OP_MORE_EQ, greater_or_equal}, {TOKEN_OP_ASSIGN, none}, {TOKEN_LPAR, none}, {TOKEN_IDENTIFIER, none}, {TOKEN_EOF, none}};
    for (size_t i = 0; i < sizeof op / sizeof *op; i++)
    {
        token t = {op[i].t, NULL, 1, 1};
        CHECK(parse_operator(&t) == op[i].o);
    }
}

static void test_stack(void)
{
    stack *s = stack_init(0);
    CHECK(s && s->cap >= 1 && s->size == 0 && is_empty_stack(s));
    CHECK(stack_pop(s) == NULL);
    free_stack(s);

    int caps[] = {-5, 0, 1, 2, 16};
    for (size_t c = 0; c < sizeof caps / sizeof *caps; c++)
    {
        s = stack_init(caps[c]);
        int n = rand() % 200;
        for (int i = 0; i < n; i++)
            push_t(s, i);
        CHECK(s->size == n && s->cap >= n);
        for (int i = n - 1; i >= 0; i--)
        {
            stack_item *it = stack_pop(s);
            CHECK(it && it->type == terminal && it->ast.term == i);
            free(it);
        }
        CHECK(is_empty_stack(s) && stack_pop(s) == NULL);
        free_stack(s);
    }
    s = stack_init(2); /* uzly zostanu na zasobniku -> free_stack ich uvolni */
    for (int i = 0; i < 5; i++)
    {
        push_t(s, i);
        push_n(s, lit_i(i));
    }
    CHECK(s->size == 10);
    free_stack(s);
#ifdef TEST_NULL
    EXPECT_NOCRASH({ free_stack(NULL); });
#endif
}

static void test_top_terminal_idx_to_op(void)
{
    CHECK(top_terminal(NULL) == 7);
    stack *s = stack_init(2);
    CHECK(top_terminal(s) == 7); /* prazdny */
    push_t(s, 7);
    CHECK(top_terminal(s) == 7);
    push_n(s, lit_i(1));
    CHECK(top_terminal(s) == 7); /* uzol sa preskoci */
    push_t(s, 0);
    push_n(s, lit_i(2));
    push_n(s, lit_i(3));
    CHECK(top_terminal(s) == 0);
    for (int i = 0; i < 100; i++)
    {
        int v = rand() % 15;
        push_t(s, v);
        push_n(s, lit_i(0));
        CHECK(top_terminal(s) == v);
    }
    free_stack(s);

    operator exp[15] = {add, subtract, multiply, divide, negation, none, none, none, none,
                        lesser, lesser_or_equal, greater, greater_or_equal, equal, not_equal};
    for (int i = 0; i < 15; i++)
        CHECK(idx_to_op(i) == exp[i]);
    CHECK(idx_to_op(-1) == none && idx_to_op(15) == none && idx_to_op(9999) == none);
    for (int i = 0; i < 100; i++)
        CHECK(idx_to_op(100 + rand() % 1000) == none);
}

static int idx(TokenType ty, const char *d, int depth, int prev)
{
    token t[2] = {{ty, (char *)d, 1, 1}, {TOKEN_EOF, NULL, 1, 2}};
    token_array a = {t, 2, 0};
    return token_to_index(&a, depth, prev);
}

static void test_token_to_index(void)
{
    CHECK(idx(TOKEN_IDENTIFIER, "a", 0, 7) == 8);
    CHECK(idx(TOKEN_INT_LITERAL, "1", 0, 7) == 8);
    CHECK(idx(TOKEN_DOUBLE_LITERAL, "1.5", 0, 7) == 8);
    CHECK(idx(TOKEN_STRING_LITERAL, "s", 0, 7) == 8);
    CHECK(idx(TOKEN_OP_PLUS, "+", 0, 8) == 0);
    CHECK(idx(TOKEN_OP_MUL, "*", 0, 8) == 2);
    CHECK(idx(TOKEN_OP_DIV, "/", 0, 8) == 3);
    CHECK(idx(TOKEN_LPAR, "(", 0, 7) == 5);
    CHECK(idx(TOKEN_OP_LESS, "<", 0, 8) == 9);
    CHECK(idx(TOKEN_OP_LESS_EQ, "<=", 0, 8) == 10);
    CHECK(idx(TOKEN_OP_MORE, ">", 0, 8) == 11);
    CHECK(idx(TOKEN_OP_MORE_EQ, ">=", 0, 8) == 12);
    CHECK(idx(TOKEN_OP_EQ, "==", 0, 8) == 13);
    CHECK(idx(TOKEN_OP_NEQ, "!=", 0, 8) == 14);
    /* ) podla hlbky */
    CHECK(idx(TOKEN_RPAR, ")", 0, 8) == 7);
    CHECK(idx(TOKEN_RPAR, ")", -3, 8) == 7);
    CHECK(idx(TOKEN_RPAR, ")", 1, 8) == 6);
    CHECK(idx(TOKEN_RPAR, ")", 50, 8) == 6);
    /* tokeny, ktore ukoncuju vyraz */
    TokenType end[] = {TOKEN_EOL, TOKEN_EOF, TOKEN_COMMA, TOKEN_KW_THEN, TOKEN_KW_DO,
                       TOKEN_SEMICOLON, TOKEN_DED, TOKEN_IND, TOKEN_KW_ELSE};
    for (size_t i = 0; i < sizeof end / sizeof *end; i++)
        CHECK(idx(end[i], NULL, 0, 8) == 7);
    /* minus: binarne po operande/ ), inak unarne - vsetky prev_index 0..14 */
    for (int prev = 0; prev < 15; prev++)
        CHECK(idx(TOKEN_OP_MINUS, "-", 0, prev) == ((prev == 8 || prev == 6) ? 1 : 4));
    for (int i = 0; i < 100; i++)
    {
        int d = rand() % 40 - 10;
        CHECK(idx(TOKEN_RPAR, ")", d, 8) == (d > 0 ? 6 : 7));
    }
    /* id nasledovany ( je v zakladnom zadani chyba 2; id ako posledny token je v poriadku */
    ARR(f, T(TOKEN_IDENTIFIER, "f"), T(TOKEN_LPAR, "("), T(TOKEN_EOF, NULL));
    EXPECT_EXIT(2, { token_to_index(&f, 0, 7); });
    ARR(g, T(TOKEN_IDENTIFIER, "x"));
    CHECK(token_to_index(&g, 0, 7) == 8);
}

static node *term_of(TokenType ty, const char *d)
{
    token t[2] = {{ty, (char *)d, 1, 1}, {TOKEN_EOF, NULL, 1, 2}};
    token_array a = {t, 2, 0};
    return parse_term(&a);
}

static void test_parse_term(void)
{
    node *n = term_of(TOKEN_IDENTIFIER, "abc");
    CHECK(n && n->type == node_id && strcmp(n->as.ident.name, "abc") == 0);
    free_node(n);
    n = term_of(TOKEN_STRING_LITERAL, "hi");
    CHECK(n && n->type == node_lit && n->as.lit.type == type_string && !strcmp(n->as.lit.s, "hi"));
    free_node(n);
    n = term_of(TOKEN_STRING_LITERAL, "");
    CHECK(n && n->as.lit.s && n->as.lit.s[0] == 0);
    free_node(n);
    n = term_of(TOKEN_DOUBLE_LITERAL, "3.14");
    CHECK(n && n->as.lit.type == type_double && n->as.lit.d > 3.139999 && n->as.lit.d < 3.140001);
    free_node(n);
    n = term_of(TOKEN_INT_LITERAL, "0");
    CHECK(n && n->as.lit.i == 0);
    free_node(n);
    for (int i = 0; i < 100; i++)
    {
        int v = rand() % 1000000;
        char b[16];
        snprintf(b, sizeof b, "%d", v);
        n = term_of(TOKEN_INT_LITERAL, b);
        CHECK(n && n->type == node_lit && n->as.lit.type == type_int && n->as.lit.i == v);
        free_node(n);
    }
    ARR(a, T(TOKEN_IDENTIFIER, "a"), T(TOKEN_EOF, NULL));
    n = parse_term(&a);
    CHECK(a.pos == 1);
    free_node(n); /* token sa spotrebuje */
    EXPECT_EXIT(2, { term_of(TOKEN_EOL, NULL); });
    EXPECT_EXIT(2, { term_of(TOKEN_INT_LITERAL, NULL); });
    EXPECT_EXIT(2, { term_of(TOKEN_OP_PLUS, "+"); });
    EXPECT_EXIT(2, { term_of(TOKEN_KW_DEF, "def"); });
    EXPECT_EXIT(2, { term_of(TOKEN_LPAR, "("); });
}

static void test_reduce_stack(void)
{
    token tk = {TOKEN_EOL, NULL, 1, 1};
    int ops[] = {0, 1, 2, 3, 9, 10, 11, 12, 13, 14};
    for (size_t i = 0; i < sizeof ops / sizeof *ops; i++)
    {
        int a = rand() % 1000, b = rand() % 1000;
        stack *s = new_st();
        push_n(s, lit_i(a));
        push_t(s, ops[i]);
        push_n(s, lit_i(b));
        reduce_stack(s, &tk);
        CHECK(s->size == 2 && s->items[1]->type == node_item);
        node *n = s->items[1]->ast.n;
        CHECK(n->type == node_binar && n->as.binar.op == idx_to_op(ops[i]));
        CHECK(n->as.binar.left->as.lit.i == a && n->as.binar.right->as.lit.i == b); /* poradie! */
        free_stack(s);
    }
    stack *s = new_st();
    push_t(s, 4);
    push_n(s, lit_i(5)); /* neg E */
    reduce_stack(s, &tk);
    CHECK(s->size == 2 && s->items[1]->ast.n->type == node_unar);
    CHECK(s->items[1]->ast.n->as.unar.op == negation);
    CHECK(s->items[1]->ast.n->as.unar.operand->as.lit.i == 5);
    free_stack(s);

    s = new_st();
    push_t(s, 5);
    node *in = lit_i(9);
    push_n(s, in);
    push_t(s, 6); /* ( E ) */
    reduce_stack(s, &tk);
    CHECK(s->size == 2 && s->items[1]->type == node_item && s->items[1]->ast.n == in);
    free_stack(s);

    /* chyby: samotne $, 0 E ), E ( E */
    EXPECT_EXIT(2, { stack *x = new_st(); reduce_stack(x, &tk); });
    EXPECT_EXIT(2, { stack *x = new_st(); push_t(x, 0); push_n(x, lit_i(1)); push_t(x, 6);
                     reduce_stack(x, &tk); });
    EXPECT_EXIT(2, { stack *x = new_st(); push_n(x, lit_i(1)); push_t(x, 5); push_n(x, lit_i(2));
                     reduce_stack(x, &tk); });
}

static void test_parse_expr(void)
{
    static const struct
    {
        const char *src, *dump;
    } OK[] = {
        {"1 + 2 * 3", "(+ 1 (* 2 3))"},
        {"(1 + 2) * 3", "(* (+ 1 2) 3)"},
        {"1 - 2 - 3", "(- (- 1 2) 3)"},
        {"8 / 2 / 2", "(/ (/ 8 2) 2)"},
        {"2 * (3 + 4) - 5", "(- (* 2 (+ 3 4)) 5)"},
        {"-a * b", "(* (neg a) b)"},
        {"a - -b", "(- a (neg b))"},
        {"- - a", "(neg (neg a))"},
        {"a * -b", "(* a (neg b))"},
        {"a < b + 1", "(< a (+ b 1))"},
        {"a + 1 < b * 2", "(< (+ a 1) (* b 2))"},
        {"a <= b", "(<= a b)"},
        {"a >= b", "(>= a b)"},
        {"a > b", "(> a b)"},
        {"a == b", "(== a b)"},
        {"a != b", "(!= a b)"},
        {"1 < 2 == 3 < 4", "(== (< 1 2) (< 3 4))"},
        {"((1))", "1"},
        {"a", "a"},
        {"7", "7"},
        {"1.5 * 2", "(* 1.5 2)"},
        {"\"x\" + \"y\"", "(+ \"x\" \"y\")"},
    };
    for (size_t i = 0; i < sizeof OK / sizeof *OK; i++)
    {
        token_array a = lex(OK[i].src);
        node *n = parse_expr(&a);
        char out[512] = "";
        dump(n, out);
        if (strcmp(out, OK[i].dump) != 0)
            printf("  expr '%s' -> %s\n", OK[i].src, out);
        CHECK(strcmp(out, OK[i].dump) == 0);
        CHECK(a.token_data[a.pos].type == TOKEN_EOL); /* pos ostane na prvom nespotrebovanom */
        free_node(n);
    }
    const char *ERR[] = {"a < b < c", "1 < 2 < 3", "a == b != c", "a <= b >= c", "(1 + 2",
                         "1 +", "( + 1)", "()", "", "1 2", "a b", "* 1", "1 * * 2", "-",
                         "(", ")", "1 + (2", "a (b)", "f(1)"};
    for (size_t i = 0; i < sizeof ERR / sizeof *ERR; i++)
    {
        token_array a = lex(ERR[i]);
        EXPECT_EXIT(2, { parse_expr(&a); });
    }
    /* vyraz konci na tokene, ktory don nepatri */
    struct
    {
        const char *src;
        TokenType stop;
    } END[] = {
        {"a ) b", TOKEN_RPAR}, {"a , b", TOKEN_COMMA}, {"1 + 2 then", TOKEN_KW_THEN}, {"x < 3 do", TOKEN_KW_DO}, {"a ; b", TOKEN_SEMICOLON}};
    for (size_t i = 0; i < sizeof END / sizeof *END; i++)
    {
        token_array a = lex(END[i].src);
        node *n = parse_expr(&a);
        CHECK(n && a.token_data[a.pos].type == END[i].stop);
        free_node(n);
    }
    /* nahodne vyrazy vs. referencny vyhodnocovac */
    for (int i = 0; i < 500; i++)
    {
        char src[8192] = "";
        g_expr(src, 0);
        token_array a = lex(src);
        node *n = parse_expr(&a);
        rp = src;
        unsigned long long want = r_expr();
        CHECK(n != NULL && eval(n) == want);
        if (n && eval(n) != want)
            printf("  random expr: %s\n", src);
        free_node(n);
    }
}

static void test_parse_call(void)
{
    token_array a = lex("f()");
    node *n = parse_call(&a);
    char o[256] = "";
    dump(n, o);
    CHECK(strcmp(o, "(call f)") == 0);
    free_node(n);
    a = lex("f(a)");
    n = parse_call(&a);
    o[0] = 0;
    dump(n, o);
    CHECK(strcmp(o, "(call f a)") == 0);
    free_node(n);
    a = lex("f(1, b, \"s\")");
    n = parse_call(&a);
    o[0] = 0;
    dump(n, o);
    CHECK(strcmp(o, "(call f 1 b \"s\")") == 0);
    free_node(n);
    CHECK(a.token_data[a.pos].type == TOKEN_EOL);
    a = lex("f(1 + 2)");
    n = parse_call(&a);
    o[0] = 0;
    dump(n, o);
    CHECK(strcmp(o, "(call f (+ 1 2))") == 0);
    free_node(n);
    for (int k = 0; k < 20; k++)
    { /* nahodny pocet argumentov */
        int cnt = rand() % 15;
        char src[512] = "g(";
        for (int i = 0; i < cnt; i++)
        {
            char b[16];
            snprintf(b, sizeof b, "%s%d", i ? ", " : "", rand() % 100);
            strcat(src, b);
        }
        strcat(src, ")");
        a = lex(src);
        n = parse_call(&a);
        CHECK(n && n->as.call.size == cnt);
        free_node(n);
    }
    const char *bad[] = {"f(,)", "f(a,,b)", "f(a", "f(a, b", "f(a,)", "f(", "f(a b)", "f(,a)", "f(+)"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++)
    {
        token_array b = lex(bad[i]);
        EXPECT_EXIT(2, { parse_call(&b); });
    }
}

static void test_statements(void)
{
    /* val / var */
    token_array a = lex("val x = 5");
    node *n = parse_val_var(&a, &a.token_data[0]);
    CHECK(n && n->type == node_val_var && !n->as.val_var.changable &&
          !strcmp(n->as.val_var.name, "x") && n->as.val_var.type == type_none &&
          n->as.val_var.init->type == node_lit);
    free_node(n);
    a = lex("var y: Int = 1 + 2");
    n = parse_val_var(&a, &a.token_data[0]);
    CHECK(n && n->as.val_var.changable && n->as.val_var.type == type_int &&
          n->as.val_var.init->type == node_binar);
    free_node(n);
    a = lex("var s: String = f(1)");
    n = parse_val_var(&a, &a.token_data[0]);
    CHECK(n && n->as.val_var.type == type_string && n->as.val_var.init->type == node_call);
    free_node(n);
    const char *badv[] = {"val = 5", "val x 5", "val x: = 5", "val x: Foo = 1", "val x =",
                          "val 5 = 5", "var", "val x: Int"};
    for (size_t i = 0; i < sizeof badv / sizeof *badv; i++)
    {
        token_array b = lex(badv[i]);
        EXPECT_EXIT(2, { parse_val_var(&b, &b.token_data[0]); });
    }
    /* priradenie */
    a = lex("x = 1");
    n = parse_asign(&a);
    CHECK(n && n->type == node_asign && !strcmp(n->as.asign.name, "x") &&
          n->as.asign.init->type == node_lit);
    free_node(n);
    a = lex("x = f(2)");
    n = parse_asign(&a);
    CHECK(n->as.asign.init->type == node_call);
    free_node(n);
    a = lex("x = a * (b + 1)");
    n = parse_asign(&a);
    CHECK(n->as.asign.init->type == node_binar);
    free_node(n);
    const char *bada[] = {"x 1", "x =", "x = = 1", "x = 1 2"};
    for (size_t i = 0; i < sizeof bada / sizeof *bada; i++)
    {
        token_array b = lex(bada[i]);
        EXPECT_EXIT(2, { parse_asign(&b); });
    }
    /* return */
    a = lex("return");
    n = parse_return(&a);
    CHECK(n && n->type == node_return && n->as.ret.value == NULL);
    free_node(n);
    a = lex("return;");
    n = parse_return(&a);
    CHECK(n && n->as.ret.value == NULL);
    free_node(n);
    a = lex("return 1 + 2");
    n = parse_return(&a);
    CHECK(n->as.ret.value->type == node_binar);
    free_node(n);
    a = lex("return f(1)");
    n = parse_return(&a);
    CHECK(n->as.ret.value->type == node_call);
    free_node(n);
    a = lex("return )");
    EXPECT_EXIT(2, { parse_return(&a); });
}

static void test_control_flow(void)
{
    token_array a = lex("if a < 1 then\n  x = 1\nelse\n  x = 2\n");
    node *n = parse_if(&a);
    CHECK(n && n->type == node_if && n->as.if_n.condition->type == node_binar &&
          n->as.if_n.then->as.block.size == 1 && n->as.if_n.else_n->as.block.size == 1);
    CHECK(a.token_data[a.pos].type == TOKEN_EOF);
    free_node(n);
    const char *badi[] = {"if a then\n  x = 1\n", "if a\n  x = 1\nelse\n  x = 2\n",
                          "if a then\nx = 1\nelse\n  x = 2\n", "if then\n  x = 1\nelse\n  x = 2\n",
                          "if a then\n  x = 1\nelse\nx = 2\n"};
    for (size_t i = 0; i < sizeof badi / sizeof *badi; i++)
    {
        token_array b = lex(badi[i]);
        EXPECT_EXIT(2, { parse_if(&b); });
    }
    a = lex("while i > 0 do\n  i = i - 1\n  j = 2\n");
    n = parse_while(&a);
    CHECK(n && n->type == node_while && n->as.while_n.condition->type == node_binar &&
          n->as.while_n.body->as.block.size == 2);
    CHECK(a.token_data[a.pos].type == TOKEN_EOF);
    free_node(n);
    const char *badw[] = {"while i > 0\n  i = 1\n", "while i > 0 do\ni = 1\n", "while do\n  i = 1\n",
                          "while i > 0 do"};
    for (size_t i = 0; i < sizeof badw / sizeof *badw; i++)
    {
        token_array b = lex(badw[i]);
        EXPECT_EXIT(2, { parse_while(&b); });
    }
}

static void test_block_and_body(void)
{
    token_array a = lex("  val a = 1\n  a = 2\n");
    node *b = parse_function_body(&a);
    CHECK(b && b->type == node_block && b->as.block.size == 2 &&
          b->as.block.items[0]->type == node_val_var && b->as.block.items[1]->type == node_asign);
    free_node(b);
    a = lex("  a = 1; b = 2\n"); /* viac prikazov cez ; */
    b = parse_function_body(&a);
    CHECK(b && b->as.block.size == 2);
    free_node(b);
    a = lex("  print(1)\n  5\n  x\n"); /* volanie, literal, identifikator */
    b = parse_function_body(&a);
    CHECK(b && b->as.block.size == 3 && b->as.block.items[0]->type == node_expression);
    free_node(b);
    a = lex("1 + 2\n"); /* jednoriadkove telo */
    b = parse_function_body(&a);
    CHECK(b && b->as.block.size == 1);
    free_node(b);
    ARR(e, T(TOKEN_IND, NULL), T(TOKEN_DED, NULL), T(TOKEN_EOF, NULL)); /* prazdny blok */
    EXPECT_EXIT(2, { parse_function_body(&e); });
    ARR(k, T(TOKEN_IND, NULL), T(TOKEN_KW_DEF, "def"), T(TOKEN_DED, NULL), T(TOKEN_EOF, NULL));
    EXPECT_EXIT(2, { parse_function_body(&k); }); /* klucove slovo ako prikaz */
    ARR(f, T(TOKEN_IND, NULL), T(TOKEN_IDENTIFIER, "a"), T(TOKEN_OP_ASSIGN, "="),
        T(TOKEN_INT_LITERAL, "1"), T(TOKEN_EOL, NULL), T(TOKEN_EOF, NULL));
    EXPECT_EXIT(2, { parse_function_body(&f); }); /* chyba dedent */
}

static void test_def_and_main(void)
{
    token_array a = lex("def f(a: Int, b: String): Int =\n  return a\n");
    node *n = parse_def(&a, &a.token_data[0]);
    node *fn = n;
    CHECK(fn && fn->type == node_function && !strcmp(fn->as.function.name, "f") &&
          fn->as.function.param_count == 2 && fn->as.function.params[0].types == type_int &&
          fn->as.function.params[1].types == type_string && !strcmp(fn->as.function.params[1].name, "b") &&
          fn->as.function.return_type == type_int && fn->as.function.explicit &&
          !fn->as.function.is_main && fn->as.function.body->as.block.size == 1);
    free_node(n);
    a = lex("def k() = 1 + 2\n");
    n = parse_def(&a, &a.token_data[0]);
    CHECK(n && n->as.function.param_count == 0 && !n->as.function.explicit &&
          n->as.function.return_type == type_none);
    free_node(n);
    a = lex("def h(): Unit =\n  print(1)\n");
    n = parse_def(&a, &a.token_data[0]);
    CHECK(n && n->as.function.return_type == type_unit);
    free_node(n);
    a = lex("def f(a: Int,\n) = 1\n"); /* koncova ciarka + novy riadok */
    n = parse_def(&a, &a.token_data[0]);
    CHECK(n && n->as.function.param_count == 1);
    free_node(n);
    for (int k = 0; k < 30; k++)
    { /* nahodne parametre */
        int cnt = rand() % 20;
        char src[1024] = "def r(";
        data_types want[20];
        for (int i = 0; i < cnt; i++)
        {
            int t = rand() % 3;
            const char *tn[] = {"Int", "Double", "String"};
            want[i] = t == 0 ? type_int : t == 1 ? type_double
                                                 : type_string;
            char b[48];
            snprintf(b, sizeof b, "%sp%d: %s", i ? ", " : "", i, tn[t]);
            strcat(src, b);
        }
        strcat(src, ") = 1\n");
        a = lex(src);
        n = parse_def(&a, &a.token_data[0]);
        CHECK(n && n->as.function.param_count == cnt);
        for (int i = 0; n && i < cnt; i++)
            CHECK(n->as.function.params[i].types == want[i]);
        free_node(n);
    }
    const char *bad[] = {"def (a: Int) = 1\n", "def f(a Int) = 1\n", "def f(a: Foo) = 1\n",
                         "def f(a: Int,) = 1\n", "def f(a: Int = 1\n", "def f(: Int) = 1\n",
                         "def f() Int = 1\n", "def f(): = 1\n", "def f(\n", "def f() =\n",
                         "def 5() = 1\n", "def f(a: Int b: Int) = 1\n"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++)
    {
        token_array b = lex(bad[i]);
        EXPECT_EXIT(2, { parse_def(&b, &b.token_data[0]); });
    }
    /* @main */
    a = lex("@main\ndef run(): Unit =\n  print(1)\n");
    n = parse_main(&a, &a.token_data[0]);
    CHECK(n && n->as.function.is_main && !strcmp(n->as.function.name, "run"));
    free_node(n);
    a = lex("@main def run(): Unit = print(1)\n");
    n = parse_main(&a, &a.token_data[0]);
    CHECK(n && n->as.function.is_main);
    free_node(n);
    const char *badm[] = {"@foo def f() = 1\n", "@main\n", "@main 5\n", "@main val x = 1\n", "@\n"};
    for (size_t i = 0; i < sizeof badm / sizeof *badm; i++)
    {
        token_array b = lex(badm[i]);
        EXPECT_EXIT(2, { parse_main(&b, &b.token_data[0]); });
    }
    token_array nm = lex("def f() = 1\n");
    EXPECT_EXIT(2, { parse_main(&nm, &nm.token_data[0]); }); /* chyba '@' */
}

/* parse_program = prolog + zoznam funkcii (vzdy s prologom pred zdrojom) */
static void test_program(void)
{
    token_array a = lexp("def f(n: Int): Int =\n  var i = 0\n  while i < n do\n    i = i + 1\n  return i\n"
                         "@main def run(): Unit =\n  print(1)\n");
    node *p = parse_program(&a);
    CHECK(p && p->type == node_program && p->as.program.size == 2);
    if (p && p->as.program.size == 2)
    {
        node *f = p->as.program.items[0];
        CHECK(f->as.function.body->as.block.size == 3 &&
              f->as.function.body->as.block.items[1]->type == node_while &&
              f->as.function.body->as.block.items[2]->type == node_return);
        CHECK(!f->as.function.is_main && p->as.program.items[1]->as.function.is_main);
    }
    free_node(p);

    a = lexp("\n\n"); /* ziadna funkcia */
    p = parse_program(&a);
    CHECK(p && p->as.program.size == 0);
    free_node(p);

    token_array g = lexp("val x = 1\n"); /* nie funkcia na top-level */
    EXPECT_EXIT(2, { parse_program(&g); });
    token_array g2 = lexp("5\n");
    EXPECT_EXIT(2, { parse_program(&g2); });

    for (int k = 0; k < 20; k++)
    { /* nahodny pocet funkcii */
        int cnt = rand() % 15;
        char src[2048] = "";
        for (int i = 0; i < cnt; i++)
        {
            char b[48];
            snprintf(b, sizeof b, "def f%d() = %d\n", i, i);
            strcat(src, b);
        }
        a = lexp(src);
        p = parse_program(&a);
        CHECK(p && p->as.program.size == cnt);
        if (p && p->as.program.size != cnt)
            printf("  cnt=%d size=%d cap=%d\n", cnt, p->as.program.size, p->as.program.cap);
        for (int i = 0; p && i < cnt && i < p->as.program.size; i++)
        {
            char nm[16];
            snprintf(nm, sizeof nm, "f%d", i);
            CHECK(!strcmp(p->as.program.items[i]->as.function.name, nm)); /* poradie zdrojaku */
        }
        free_node(p);
    }
    /* @main pred bezou funkciou, nazov main funkcie je lubovolny */
    a = lexp("\n@main def run(): Unit =\n  print(1)\ndef f(a: Int): Int = a\n");
    p = parse_program(&a);
    CHECK(p && p->type == node_program && p->as.program.size == 2);
    if (p && p->as.program.size == 2)
        CHECK(p->as.program.items[0]->as.function.is_main && !strcmp(p->as.program.items[1]->as.function.name, "f"));
    free_node(p);

    token_array np = lex("def f() = 1\n"); /* chyba prolog */
    EXPECT_EXIT(2, { parse_program(&np); });
}

int main(void)
{
    srand(12345); /* pevny seed -> reprodukovatelne "nahodne" testy */
    test_strduplicate();
    test_is_keyword();
    test_token_name();
    test_skip_peek_get();
    test_parse_prologue();
    test_create_free_node();
    test_parse_type_operator();
    test_stack();
    test_top_terminal_idx_to_op();
    test_token_to_index();
    test_parse_term();
    test_reduce_stack();
    test_parse_expr();
    test_parse_call();
    test_statements();
    test_control_flow();
    test_block_and_body();
    test_def_and_main();
    test_program();
    printf("%d checks, %d failed\n", g_run, g_fail);
    return g_fail != 0;
}