typedef enum {
    opening,
    closing
} bracket_variant;

typedef enum {
    token_identifier,
    token_keyword,
    token_operator,
    token_int,
    token_double,
    token_string,
    token_specialchar,
    token_bracket_variant,
    token_semicolon,
    token_indent,
    token_dedent,
    token_newline,
    token_eof
} token_type;

typedef enum {
    missing_char,
    unexpected,
    invalid_operand,
    missmatched_parenthasis
} error_messages_types;