#ifndef __DPL_LEXER_H
#define __DPL_LEXER_H

#include <nobx.h>

#ifndef DPL_MAX_INTERPOLATION
#define DPL_MAX_INTERPOLATION 8
#endif

#define LOC_Fmt SV_Fmt ":%zu:%zu"
#define LOC_Arg(loc) SV_Arg(loc.file_name), (loc.line + 1), (loc.column + 1)

#define DPL_TOKEN_ERROR(source, token, format, ...)                   \
    do                                                                \
    {                                                                 \
        DW_ERROR_MSG(LOC_Fmt ": ERROR: ", LOC_Arg((token).location)); \
        DW_ERROR_MSG(format, ##__VA_ARGS__);                          \
        DW_ERROR_MSG("\n");                                           \
        dpl_lexer_print_token((source), DW_ERROR_STREAM, (token));    \
        exit(1);                                                      \
    } while (false)

// LOCATION

typedef struct
{
    Nob_String_View file_name;

    size_t line;
    size_t column;

    const char *line_start;
} DPL_Location;

// LEXER

typedef enum
{
    TOKEN_NONE = 0,
    TOKEN_EOF,
    TOKEN_WHITESPACE,
    TOKEN_COMMENT,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_AND_AND,
    TOKEN_PIPE_PIPE,

    TOKEN_DOT,
    TOKEN_DOT_DOT,
    TOKEN_COLON,
    TOKEN_COLON_EQUAL,

    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_OPEN_BRACKET,
    TOKEN_CLOSE_BRACKET,
    TOKEN_OPEN_DOLLAR_BRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_STRING_INTERPOLATION,
    TOKEN_TRUE,
    TOKEN_FALSE,

    TOKEN_KEYWORD_CONSTANT,
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_TYPE,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_IN,

    COUNT_TOKEN_KINDS,
} DPL_TokenKind;

typedef struct
{
    DPL_TokenKind kind;
    DPL_Location location;
    Nob_String_View text;
} DPL_Token;

typedef struct
{
    Nob_String_View file_name;
    Nob_String_View source;

    size_t position;
    size_t token_start;
    DPL_Location token_start_location;
    const char *current_line;
    size_t line;
    size_t column;
    DPL_Token first_token;
    DPL_Token peek_token;
    int interpolation_brackets[DPL_MAX_INTERPOLATION];
    int interpolation_depth;
} DPL_Lexer;

const char *dpl_lexer_token_kind_name(DPL_TokenKind kind);

void dpl_lexer_print_token(Nob_String_View source, FILE *f, DPL_Token token);
void dpl_lexer_print_token_range(Nob_String_View source, FILE *out, DPL_Token first, DPL_Token last);

DPL_Token dpl_lexer_next_token(DPL_Lexer *lexer);
DPL_Token dpl_lexer_peek_token(DPL_Lexer *lexer);

#endif