typedef enum {
    opening,
    closing
} bracket_variant;

typedef enum {
    token_identifier,
    token_keyword,
    token_operator,
    token_number,
    token_string,
    token_specialChar,
    token_bracket_variant,
    token_semicolon
} token_type;

typedef enum {
    missing_char,
    unexpected,
    invalid_operand,
    missmatched_parenthasis
} error_messages_types;