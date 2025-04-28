#include <error.h>
#include <nobx.h>

#include <dpl/lexer.h>

#define DPL_LEXER_ERROR(lexer, format, ...)                                   \
    do                                                                        \
    {                                                                         \
        DPL_Token error_token = dpl_lexer_build_token((lexer), TOKEN_NONE);   \
        DW_ERROR_MSG(LOC_Fmt ": ERROR: ", LOC_Arg(error_token.location));     \
        DW_ERROR_MSG(format, ##__VA_ARGS__);                                  \
        DW_ERROR_MSG("\n");                                                   \
        dpl_lexer_print_token((lexer)->source, DW_ERROR_STREAM, error_token); \
        exit(1);                                                              \
    } while (false)

const char *TOKEN_KIND_NAMES[COUNT_TOKEN_KINDS] = {
    [TOKEN_AND_AND] = "token `&&`",
    [TOKEN_BANG_EQUAL] = "token `!=`",
    [TOKEN_BANG] = "token `!`",
    [TOKEN_CLOSE_BRACE] = "token `}`",
    [TOKEN_CLOSE_BRACKET] = "token `]`",
    [TOKEN_CLOSE_PAREN] = "token `)`",
    [TOKEN_COLON_EQUAL] = "token `:=`",
    [TOKEN_COLON] = "token `:`",
    [TOKEN_COMMA] = "token `,`",
    [TOKEN_COMMENT] = "<comment>",
    [TOKEN_DOT_DOT] = "token `..`",
    [TOKEN_DOT] = "token `.`",
    [TOKEN_EOF] = "<end of file>",
    [TOKEN_EQUAL_EQUAL] = "token `==`",
    [TOKEN_FALSE] = "boolean literal",
    [TOKEN_GREATER_EQUAL] = "token `>=`",
    [TOKEN_GREATER] = "token `>`",
    [TOKEN_IDENTIFIER] = "<identifier>",
    [TOKEN_KEYWORD_CONSTANT] = "keyword `constant`",
    [TOKEN_KEYWORD_ELSE] = "keyword `else`",
    [TOKEN_KEYWORD_FUNCTION] = "keyword `function`",
    [TOKEN_KEYWORD_IF] = "keyword `if`",
    [TOKEN_KEYWORD_TYPE] = "keyword `type`",
    [TOKEN_KEYWORD_VAR] = "keyword `var`",
    [TOKEN_KEYWORD_WHILE] = "keyword `while`",
    [TOKEN_KEYWORD_FOR] = "keyword `for`",
    [TOKEN_KEYWORD_IN] = "keyword `in`",
    [TOKEN_LESS_EQUAL] = "token `<=`",
    [TOKEN_LESS] = "token `<`",
    [TOKEN_MINUS] = "token `-`",
    [TOKEN_NONE] = "<none>",
    [TOKEN_NUMBER] = "number literal",
    [TOKEN_OPEN_BRACE] = "token `{`",
    [TOKEN_OPEN_BRACKET] = "token `[`",
    [TOKEN_OPEN_DOLLAR_BRACKET] = "token `$[`",
    [TOKEN_OPEN_PAREN] = "token `(`",
    [TOKEN_PIPE_PIPE] = "token `||`",
    [TOKEN_PLUS] = "token `+`",
    [TOKEN_SEMICOLON] = "token `;`",
    [TOKEN_SLASH] = "token `/`",
    [TOKEN_STAR] = "token `*`",
    [TOKEN_STRING_INTERPOLATION] = "string interpolation",
    [TOKEN_STRING] = "string literal",
    [TOKEN_TRUE] = "boolean literal",
    [TOKEN_WHITESPACE] = "<whitespace>",
};

static_assert(COUNT_TOKEN_KINDS == 45,
              "Count of token kinds has changed, please update token kind names map.");

const char *dpl_lexer_token_kind_name(DPL_TokenKind kind)
{
    if (kind < 0 || kind >= COUNT_TOKEN_KINDS)
    {
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
    return TOKEN_KIND_NAMES[kind];
}

static Nob_String_View dpl_lexer_line_view(Nob_String_View source, const char *line_start)
{
    Nob_String_View line = nob_sv_from_parts(line_start, 0);
    while ((line.count < (size_t)((source.data + source.count) - line_start)) && *(line.data + line.count) != '\n' && *(line.data + line.count) != '\0')
    {
        line.count++;
    }

    return line;
}

void dpl_lexer_print_token(Nob_String_View source, FILE *f, DPL_Token token)
{
    Nob_String_View line_view = dpl_lexer_line_view(source, token.location.line_start);

    fprintf(f, "%3zu| " SV_Fmt "\n", token.location.line + 1, SV_Arg(line_view));
    fprintf(f, "   | ");

    for (size_t i = 0; i < token.location.column; ++i)
    {
        fprintf(f, " ");
    }
    fprintf(f, "^");
    if (token.text.count > 1)
    {
        for (size_t i = 0; i < token.text.count - 1; ++i)
        {
            fprintf(f, "~");
        }
    }
    fprintf(f, "\n");
}

void dpl_lexer_print_token_range(Nob_String_View source, FILE *out, DPL_Token first, DPL_Token last)
{
    const char *line = first.location.line_start;
    for (size_t line_num = first.location.line; line_num <= last.location.line; ++line_num)
    {
        Nob_String_View line_view = dpl_lexer_line_view(source, line);
        fprintf(out, "%3zu| " SV_Fmt "\n", line_num + 1, SV_Arg(line_view));
        fprintf(out, "   | ");

        size_t start_column = 0;
        if (line_num == first.location.line)
        {
            start_column = first.location.column;
        }

        size_t end_column = line_view.count;
        if (line_num == last.location.line)
        {
            end_column = last.location.column + last.text.count;
        }

        for (size_t i = 0; i < start_column; ++i)
        {
            fprintf(out, " ");
        }

        size_t length = end_column - start_column;
        if (line_num == first.location.line)
        {
            fprintf(out, "^");
            if (length > 0)
            {
                length--;
            }
        }

        if (length > 1)
        {
            for (size_t i = 0; i < length - 1; ++i)
            {
                fprintf(out, "~");
            }
            if (line_num == last.location.line)
            {
                fprintf(out, "^");
            }
            else
            {
                fprintf(out, "~");
            }
        }
        fprintf(out, "\n");

        line += line_view.count + 1;
    }
}

DPL_Token dpl_lexer_build_token(DPL_Lexer *lexer, DPL_TokenKind kind)
{
    DPL_Token token = {
        .kind = kind,
        .location = lexer->token_start_location,
        .text = nob_sv_from_parts(
            lexer->source.data + lexer->token_start,
            lexer->position - lexer->token_start),
    };

    if (lexer->first_token.kind == TOKEN_NONE)
    {
        lexer->first_token = token;
    }

    return token;
}

DPL_Token dpl_lexer_build_empty_token(DPL_Lexer *lexer, DPL_TokenKind kind)
{
    DPL_Token token = {
        .kind = kind,
        .location = lexer->token_start_location,
        .text = SV_NULL,
    };

    return token;
}

void dpl_lexer_advance(DPL_Lexer *lexer)
{
    lexer->position++;
    lexer->column++;
}

bool dpl_lexer_is_eof(DPL_Lexer *lexer)
{
    return (lexer->position >= lexer->source.count);
}

char dpl_lexer_current(DPL_Lexer *lexer)
{
    return *(lexer->source.data + lexer->position);
}

bool dpl_lexer_match(DPL_Lexer *lexer, char c)
{
    if (dpl_lexer_current(lexer) == c)
    {
        dpl_lexer_advance(lexer);
        return true;
    }
    return false;
}

DPL_Location dpl_lexer_current_location(DPL_Lexer *lexer)
{
    return (DPL_Location){
        .file_name = lexer->file_name,
        .line = lexer->line,
        .column = lexer->column,
        .line_start = lexer->current_line,
    };
}

bool dpl_lexer_is_ident_begin(char c)
{
    return c == '_' || isalpha(c);
}

bool dpl_lexer_is_ident(char c)
{
    return c == '_' || isalnum(c);
}

DPL_Token dpl_lexer_string(DPL_Lexer *lexer)
{
    bool in_escape = false;
    while (true)
    {
        if (dpl_lexer_is_eof(lexer))
        {
            DPL_LEXER_ERROR(lexer, "Unterminated string literal.");
        }

        if (dpl_lexer_current(lexer) == '$')
        {
            dpl_lexer_advance(lexer);
            if (dpl_lexer_current(lexer) != '{')
            {
                continue;
            }

            dpl_lexer_advance(lexer);
            DPL_Token token = dpl_lexer_build_token(lexer, TOKEN_STRING_INTERPOLATION);
            if (lexer->interpolation_depth < DPL_MAX_INTERPOLATION)
            {

                lexer->interpolation_brackets[lexer->interpolation_depth] = 1;
                lexer->interpolation_depth++;
                return token;
            }

            DPL_TOKEN_ERROR(lexer->source, token, "String interpolation may nest only %d levels deep.", DPL_MAX_INTERPOLATION);
        }

        if (dpl_lexer_current(lexer) == '"')
        {
            if (!in_escape)
            {
                dpl_lexer_advance(lexer);
                return dpl_lexer_build_token(lexer, TOKEN_STRING);
            }
        }

        if (!in_escape && dpl_lexer_current(lexer) == '\\')
        {
            in_escape = true;
        }
        else
        {
            in_escape = false;
        }

        dpl_lexer_advance(lexer);
    }
}

DPL_Token dpl_lexer_next_token(DPL_Lexer *lexer)
{
    if (lexer->peek_token.kind != TOKEN_NONE)
    {
        DPL_Token peeked = lexer->peek_token;
        lexer->peek_token = (DPL_Token){
            0};
        return peeked;
    }

    lexer->token_start = lexer->position;
    lexer->token_start_location = dpl_lexer_current_location(lexer);

    if (dpl_lexer_is_eof(lexer))
    {
        return dpl_lexer_build_empty_token(lexer, TOKEN_EOF);
    }

    if (isspace(dpl_lexer_current(lexer)))
    {
        while (!dpl_lexer_is_eof(lexer) && isspace(dpl_lexer_current(lexer)))
        {
            if (dpl_lexer_current(lexer) == '\n')
            {
                lexer->position++;
                lexer->line++;
                lexer->column = 0;
                lexer->current_line = lexer->source.data + lexer->position;
            }
            else
            {
                dpl_lexer_advance(lexer);
            }
        }
        return dpl_lexer_build_token(lexer, TOKEN_WHITESPACE);
    }

    switch (dpl_lexer_current(lexer))
    {
    case '+':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_PLUS);
    case '-':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_MINUS);
    case '*':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_STAR);
    case '/':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_SLASH);
    case '.':
        dpl_lexer_advance(lexer);
        if (dpl_lexer_match(lexer, '.'))
        {
            return dpl_lexer_build_token(lexer, TOKEN_DOT_DOT);
        }
        return dpl_lexer_build_token(lexer, TOKEN_DOT);
    case ':':
        dpl_lexer_advance(lexer);
        if (dpl_lexer_match(lexer, '='))
        {
            return dpl_lexer_build_token(lexer, TOKEN_COLON_EQUAL);
        }
        return dpl_lexer_build_token(lexer, TOKEN_COLON);
    case '<':
        dpl_lexer_advance(lexer);
        if (dpl_lexer_match(lexer, '='))
        {
            return dpl_lexer_build_token(lexer, TOKEN_LESS_EQUAL);
        }
        return dpl_lexer_build_token(lexer, TOKEN_LESS);
    case '>':
        dpl_lexer_advance(lexer);
        if (dpl_lexer_match(lexer, '='))
        {
            return dpl_lexer_build_token(lexer, TOKEN_GREATER_EQUAL);
        }
        return dpl_lexer_build_token(lexer, TOKEN_GREATER);
    case '=':
        if ((lexer->position < lexer->source.count - 2) && (lexer->source.data[lexer->position + 1] == '='))
        {
            dpl_lexer_advance(lexer);
            dpl_lexer_advance(lexer);
            return dpl_lexer_build_token(lexer, TOKEN_EQUAL_EQUAL);
        }
        break;
    case '&':
        if ((lexer->position < lexer->source.count - 2) && (lexer->source.data[lexer->position + 1] == '&'))
        {
            dpl_lexer_advance(lexer);
            dpl_lexer_advance(lexer);
            return dpl_lexer_build_token(lexer, TOKEN_AND_AND);
        }
        break;
    case '|':
        if ((lexer->position < lexer->source.count - 2) && (lexer->source.data[lexer->position + 1] == '|'))
        {
            dpl_lexer_advance(lexer);
            dpl_lexer_advance(lexer);
            return dpl_lexer_build_token(lexer, TOKEN_PIPE_PIPE);
        }
        break;
    case '!':
        dpl_lexer_advance(lexer);
        if (dpl_lexer_match(lexer, '='))
        {
            return dpl_lexer_build_token(lexer, TOKEN_BANG_EQUAL);
        }
        return dpl_lexer_build_token(lexer, TOKEN_BANG);
    case '(':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_OPEN_PAREN);
    case ')':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_CLOSE_PAREN);
    case '{':
        dpl_lexer_advance(lexer);
        if (lexer->interpolation_depth > 0)
        {
            lexer->interpolation_brackets[lexer->interpolation_depth - 1]++;
        }
        return dpl_lexer_build_token(lexer, TOKEN_OPEN_BRACE);
    case '}':
        dpl_lexer_advance(lexer);
        if (lexer->interpolation_depth > 0)
        {
            lexer->interpolation_brackets[lexer->interpolation_depth - 1]--;
            if (lexer->interpolation_brackets[lexer->interpolation_depth - 1] == 0)
            {
                lexer->interpolation_depth--;
                return dpl_lexer_string(lexer);
            }
        }
        return dpl_lexer_build_token(lexer, TOKEN_CLOSE_BRACE);
    case '[':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_OPEN_BRACKET);
    case ']':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_CLOSE_BRACKET);
    case '$':
        if ((lexer->position < lexer->source.count - 2) && (lexer->source.data[lexer->position + 1] == '['))
        {
            dpl_lexer_advance(lexer);
            dpl_lexer_advance(lexer);
            return dpl_lexer_build_token(lexer, TOKEN_OPEN_DOLLAR_BRACKET);
        }
        break;
    case ',':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_COMMA);
    case ';':
        dpl_lexer_advance(lexer);
        return dpl_lexer_build_token(lexer, TOKEN_SEMICOLON);
    case '#':
        dpl_lexer_advance(lexer);
        while (!dpl_lexer_is_eof(lexer))
        {
            if (dpl_lexer_current(lexer) == '\n')
            {
                lexer->position++;
                lexer->line++;
                lexer->column = 0;
                lexer->current_line = lexer->source.data + lexer->position;
                return dpl_lexer_build_token(lexer, TOKEN_COMMENT);
            }
            dpl_lexer_advance(lexer);
        }
        return dpl_lexer_build_token(lexer, TOKEN_COMMENT);
    case '"':
        dpl_lexer_advance(lexer);
        return dpl_lexer_string(lexer);
    }

    if (isdigit(dpl_lexer_current(lexer)))
    {
        // integral part
        while (!dpl_lexer_is_eof(lexer) && isdigit(dpl_lexer_current(lexer)))
            dpl_lexer_advance(lexer);

        if ((lexer->position >= lexer->source.count - 1) || *(lexer->source.data + lexer->position) != '.' || !isdigit(*(lexer->source.data + lexer->position + 1)))
        {
            return dpl_lexer_build_token(lexer, TOKEN_NUMBER);
        }
        dpl_lexer_advance(lexer); // skip the '.'

        // fractional part
        while (!dpl_lexer_is_eof(lexer) && isdigit(dpl_lexer_current(lexer)))
            dpl_lexer_advance(lexer);

        return dpl_lexer_build_token(lexer, TOKEN_NUMBER);
    }

    if (dpl_lexer_is_ident_begin(dpl_lexer_current(lexer)))
    {
        while (!dpl_lexer_is_eof(lexer) && dpl_lexer_is_ident(dpl_lexer_current(lexer)))
            dpl_lexer_advance(lexer);

        DPL_Token t = dpl_lexer_build_token(lexer, TOKEN_IDENTIFIER);
        if (nob_sv_eq(t.text, nob_sv_from_cstr("constant")))
        {
            t.kind = TOKEN_KEYWORD_CONSTANT;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("function")))
        {
            t.kind = TOKEN_KEYWORD_FUNCTION;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("var")))
        {
            t.kind = TOKEN_KEYWORD_VAR;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("if")))
        {
            t.kind = TOKEN_KEYWORD_IF;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("else")))
        {
            t.kind = TOKEN_KEYWORD_ELSE;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("true")))
        {
            t.kind = TOKEN_TRUE;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("while")))
        {
            t.kind = TOKEN_KEYWORD_WHILE;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("false")))
        {
            t.kind = TOKEN_FALSE;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("type")))
        {
            t.kind = TOKEN_KEYWORD_TYPE;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("for")))
        {
            t.kind = TOKEN_KEYWORD_FOR;
        }
        else if (nob_sv_eq(t.text, nob_sv_from_cstr("in")))
        {
            t.kind = TOKEN_KEYWORD_IN;
        }

        return t;
    }

    DPL_LEXER_ERROR(lexer, "Invalid character '%c'.", dpl_lexer_current(lexer));
}

DPL_Token dpl_lexer_peek_token(DPL_Lexer *lexer)
{
    if (lexer->peek_token.kind == TOKEN_NONE)
    {
        lexer->peek_token = dpl_lexer_next_token(lexer);
    }
    return lexer->peek_token;
}
