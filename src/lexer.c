#include "lexer.h"

void dpll_init(DPL_Lexer* lexer, const char* file_name, Nob_String_View source)
{
    *lexer = (DPL_Lexer) {
        0
    };
    lexer->file_name = nob_sv_from_cstr(file_name);
    lexer->source = source;
    lexer->current_line = lexer->source.data;
}

void dpll_advance(DPL_Lexer* lexer)
{
    lexer->position++;
    lexer->column++;
}

bool dpll_is_eof(DPL_Lexer* lexer)
{
    return (lexer->position >= lexer->source.count);
}

char dpll_current(DPL_Lexer* lexer)
{
    return *(lexer->source.data + lexer->position);
}

DPL_Location dpll_current_location(DPL_Lexer* lexer)
{
    return (DPL_Location) {
        .file_name = lexer->file_name,
        .line = lexer->line,
        .column = lexer->column,
        .line_start = lexer->current_line,
    };
}

DPL_Token dpll_build_token(DPL_Lexer* lexer, DPL_TokenKind kind)
{
    return (DPL_Token) {
        .kind = kind,
        .location = lexer->token_start_location,
        .text = nob_sv_from_parts(
                    lexer->source.data + lexer->token_start,
                    lexer->position - lexer->token_start
                ),
    };
}

DPL_Token dpll_build_empty_token(DPL_Lexer* lexer, DPL_TokenKind kind)
{
    return (DPL_Token) {
        .kind = kind,
        .location = lexer->token_start_location,
        .text = SV_NULL,
    };
}

void _dpll_print_highlighted(FILE *f, DPL_Lexer* lexer, const char* line_start, size_t column, size_t length)
{
    Nob_String_View line = nob_sv_from_parts(line_start, 0);
    while ((line.count < (size_t)((lexer->source.data + lexer->source.count) - line_start))
            && *(line.data + line.count) != '\n'
            && *(line.data + line.count) != '\0'
          ) {
        line.count++;
    }

    fprintf(f, " -> " SV_Fmt "\n", SV_Arg(line));
    fprintf(f, "    ");

    for (size_t i = 0; i < column; ++i) {
        fprintf(f, " ");
    }
    fprintf(f, "^");
    if (length > 1) {
        for (size_t i = 0; i < length - 1; ++i) {
            fprintf(f, "~");
        }
    }
    fprintf(f, "\n");
}

void dpll_error(DPL_Lexer* lexer, const char *fmt, ...) __attribute__ ((noreturn));
void dpll_error(DPL_Lexer* lexer, const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, LOC_Fmt ": ERROR: ", LOC_Arg(lexer->token_start_location));

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    _dpll_print_highlighted(stderr, lexer, lexer->current_line, lexer->token_start,
                            lexer->position - lexer->token_start);

    exit(1);
}

DPL_Token dpll_next_token(DPL_Lexer* lexer)
{
    if (lexer->peek_token.kind != TOKEN_NONE) {
        DPL_Token peeked = lexer->peek_token;
        lexer->peek_token = (DPL_Token) {
            0
        };
        return peeked;
    }

    lexer->token_start = lexer->position;
    lexer->token_start_location = dpll_current_location(lexer);

    if (dpll_is_eof(lexer)) {
        return dpll_build_empty_token(lexer, TOKEN_EOF);
    }

    if (isspace(dpll_current(lexer))) {
        while (!dpll_is_eof(lexer) && isspace(dpll_current(lexer))) {
            if (dpll_current(lexer) == '\n')
            {
                lexer->position++;
                lexer->line++;
                lexer->column = 0;
                lexer->current_line = lexer->source.data + lexer->position;
            } else {
                dpll_advance(lexer);
            }
        }
        return dpll_build_token(lexer, TOKEN_WHITESPACE);
    }

    switch (dpll_current(lexer))
    {
    case '+':
        dpll_advance(lexer);
        return dpll_build_token(lexer, TOKEN_PLUS);
    case '-':
        dpll_advance(lexer);
        return dpll_build_token(lexer, TOKEN_MINUS);
    case '*':
        dpll_advance(lexer);
        return dpll_build_token(lexer, TOKEN_STAR);
    case '/':
        dpll_advance(lexer);
        return dpll_build_token(lexer, TOKEN_SLASH);
    case '(':
        dpll_advance(lexer);
        return dpll_build_token(lexer, TOKEN_OPEN_PAREN);
    case ')':
        dpll_advance(lexer);
        return dpll_build_token(lexer, TOKEN_CLOSE_PAREN);
    }

    if (isdigit(dpll_current(lexer)))
    {
        while (!dpll_is_eof(lexer) && isdigit(dpll_current(lexer)))
            dpll_advance(lexer);

        return dpll_build_token(lexer, TOKEN_NUMBER);
    }

    if (isalpha(dpll_current(lexer)))
    {
        while (!dpll_is_eof(lexer) && isalnum(dpll_current(lexer)))
            dpll_advance(lexer);

        return dpll_build_token(lexer, TOKEN_IDENTIFIER);
    }

    dpll_error(lexer, "Invalid character '%c'.\n", dpll_current(lexer));
    dpll_advance(lexer);
}

DPL_Token dpll_peek_token(DPL_Lexer *lexer) {
    if (lexer->peek_token.kind == TOKEN_NONE) {
        lexer->peek_token = dpll_next_token(lexer);
    }
    return lexer->peek_token;
}

const char* dpll_token_kind_name(DPL_TokenKind kind)
{
    switch (kind)
    {
    case TOKEN_NONE:
        return "NONE";
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_NUMBER:
        return "NUMBER";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_WHITESPACE:
        return "WHITESPACE";

    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_STAR:
        return "STAR";
    case TOKEN_SLASH:
        return "SLASH";

    case TOKEN_OPEN_PAREN:
        return "OPEN_PAREN";
    case TOKEN_CLOSE_PAREN:
        return "CLOSE_PAREN";

    default:
        assert(false && "Unreachable");
        exit(1);
    }
}

void _dpll_print_token(DPL_Lexer* lexer, DPL_Token token)
{
    printf(LOC_Fmt ": %s (" SV_Fmt ")\n",
           LOC_Arg(token.location),
           dpll_token_kind_name(token.kind),
           SV_Arg(token.text));

    _dpll_print_highlighted(stdout, lexer, token.location.line_start,
                            token.location.column, token.text.count);
}

void dpll_print_token_location(FILE* out, DPL_Lexer* lexer, DPL_Token token)
{
    _dpll_print_highlighted(out, lexer, token.location.line_start,
                            token.location.column, token.text.count);
}
