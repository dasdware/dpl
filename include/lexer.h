#ifndef __DPL_LEXER_H
#define __DPL_LEXER_H

#include "nob.h"
#include "location.h"

typedef enum
{
    TOKEN_NONE = 0,
    TOKEN_EOF,
    TOKEN_WHITESPACE,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,

    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
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
    DPL_Token peek_token;
} DPL_Lexer;

const char* dpll_token_kind_name(DPL_TokenKind kind);

void dpll_init(DPL_Lexer* lexer, const char* file_name, Nob_String_View source);
bool dpll_is_eof(DPL_Lexer* lexer);

DPL_Token dpll_next_token(DPL_Lexer* lexer);
DPL_Token dpll_peek_token(DPL_Lexer* lexer);

void dpll_print_token_location(FILE* out, DPL_Lexer* lexer, DPL_Token token);

#endif // __DPL_LEXER_H
