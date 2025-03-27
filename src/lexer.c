#include <error.h>

#include <dpl/lexer.h>

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
    [TOKEN_LESS_EQUAL] = "token `<=`",
    [TOKEN_LESS] = "token `<`",
    [TOKEN_MINUS] = "token `-`",
    [TOKEN_NONE] = "<none>",
    [TOKEN_NUMBER] = "number literal",
    [TOKEN_OPEN_BRACE] = "token `{`",
    [TOKEN_OPEN_BRACKET] = "token `[`",
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