#ifdef DPL_LEAKCHECK
#   include "stb_leakcheck.h"
#endif

#include <dpl/utils.h>

#include "dpl.h"
#include "error.h"

#define DPL_ERROR DW_ERROR

#define DPL_TOKEN_ERROR(dpl, token, format, ...)                                      \
    do {                                                                              \
        DW_ERROR_MSG(LOC_Fmt": ERROR: ", LOC_Arg((token).location));                  \
        DW_ERROR_MSG(format, ## __VA_ARGS__);                                         \
        DW_ERROR_MSG("\n");                                                           \
        _dpll_print_token(DW_ERROR_STREAM, (dpl), (token));                           \
        exit(1);                                                                      \
    } while(false)

#define DPL_LEXER_ERROR(dpl, format, ...)                                             \
    do {                                                                              \
        DPL_Token error_token = _dpll_build_token((dpl), TOKEN_NONE);                 \
        DW_ERROR_MSG(LOC_Fmt": ERROR: ", LOC_Arg(error_token.location));              \
        DW_ERROR_MSG(format, ## __VA_ARGS__);                                         \
        DW_ERROR_MSG("\n");                                                           \
        _dpll_print_token(DW_ERROR_STREAM, (dpl), error_token);                       \
        exit(1);                                                                      \
    } while(false)

#define DPL_AST_ERROR(dpl, node, format, ...)                                         \
    do {                                                                              \
        DW_ERROR_MSG(LOC_Fmt": ERROR: ", LOC_Arg((node)->first.location));            \
        DW_ERROR_MSG(format, ## __VA_ARGS__);                                         \
        DW_ERROR_MSG("\n");                                                           \
        _dpll_print_token_range(DW_ERROR_STREAM, (dpl), (node)->first, (node)->last); \
        exit(1);                                                                      \
    } while(false)

#define DPL_AST_ERROR_WITH_NOTE(dpl, note_node, note, error_node, error_format, ...)              \
    do {                                                                                          \
        DW_ERROR_MSG(LOC_Fmt": ERROR: ", LOC_Arg((error_node)->first.location));                  \
        DW_ERROR_MSG(error_format, ## __VA_ARGS__);                                                     \
        DW_ERROR_MSG("\n");                                                                       \
        _dpll_print_token_range(DW_ERROR_STREAM, (dpl), (error_node)->first, (error_node)->last); \
        DW_ERROR_MSG(LOC_Fmt": NOTE: " note, LOC_Arg((note_node)->first.location));               \
        DW_ERROR_MSG("\n");                                                                       \
        _dpll_print_token_range(DW_ERROR_STREAM, (dpl), (note_node)->first, (note_node)->last);   \
        exit(1);                                                                                  \
    } while(false)


void dpl_init(DPL *dpl, DPL_ExternalFunctions externals)
{
    // SYMBOL STACK
    dpl_symbols_init(&dpl->symbols);

    // Base types
    dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_NUMBER, TYPE_BASE_NUMBER);
    dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_STRING, TYPE_BASE_STRING);
    dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_BOOLEAN, TYPE_BASE_BOOLEAN);
    dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_NONE, TYPE_BASE_NONE);

    // Operators on base types

    // unary operators
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "negate", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER), INST_NEGATE);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "not", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_BOOLEAN), INST_NOT);

    // binary operators
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "add", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_ADD);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "add", TYPENAME_STRING, DPL_ARGS(TYPENAME_STRING, TYPENAME_STRING), INST_ADD);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "subtract", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_SUBTRACT);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "multiply", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_MULTIPLY);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "divide", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_DIVIDE);

    // comparison operators
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "less", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_LESS);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "lessEqual", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_LESS_EQUAL);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "greater", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_GREATER);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "greaterEqual", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_GREATER_EQUAL);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "equal", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_EQUAL);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "notEqual", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_NOT_EQUAL);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "equal", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_STRING, TYPENAME_STRING), INST_EQUAL);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "notEqual", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_STRING, TYPENAME_STRING), INST_NOT_EQUAL);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "equal", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_BOOLEAN, TYPENAME_BOOLEAN), INST_EQUAL);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "notEqual", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_BOOLEAN, TYPENAME_BOOLEAN), INST_NOT_EQUAL);

    // externals
    for (size_t i = 0; i < da_size(externals); ++i)
    {
        DPL_Symbol *function = dpl_symbols_push_function_external_cstr(
            &dpl->symbols,
            externals[i].name, externals[i].return_type,
            da_size(externals[i].argument_types), externals[i].argument_types,
            i);

        if (!function)
        {
            Nob_String_Builder sb = {0};
            nob_sb_append_cstr(&sb, "Cannot add external function `");
            nob_sb_append_cstr(&sb, externals[i].name);
            nob_sb_append_cstr(&sb, "(");

            for (size_t j = 0; j < da_size(externals[i].argument_types); ++j)
            {
                if (j > 0)
                {
                    nob_sb_append_cstr(&sb, ", ");
                }
                nob_sb_append_cstr(&sb, externals[i].argument_types[j]);
            }
            nob_sb_append_cstr(&sb, "): ");
            nob_sb_append_cstr(&sb, externals[i].return_type);
            nob_sb_append_cstr(&sb, "`: ");
            nob_sb_append_cstr(&sb, dpl_symbols_last_error());

            nob_sb_append_null(&sb);

            DW_ERROR("%s", sb.items);
        }
    }

    // lexer initialization
    dpl->current_line = dpl->source.data;
}

void dpl_free(DPL *dpl)
{
    // symbol stack freeing
    dpl_symbols_free(&dpl->symbols);
    da_free(dpl->user_functions);

    // parser freeing
    arena_free(&dpl->tree.memory);

    // bound tree freeing
    arena_free(&dpl->bound_tree.memory);
}

// CATALOGS

/// TYPES

size_t _dplt_hash(Nob_String_View sv)
{
    size_t hash = 5381;
    while (sv.count > 0) {
        int c = *sv.data;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        sv.data++;
        sv.count--;
    }

    return hash;
}

// LEXER

const char* _dpll_token_kind_name(DPL_TokenKind kind)
{
    switch (kind)
    {
    case TOKEN_NONE:
        return "<none>";
    case TOKEN_EOF:
        return "<end of file>";
    case TOKEN_NUMBER:
        return "number literal";
    case TOKEN_STRING:
        return "string literal";
    case TOKEN_STRING_INTERPOLATION:
        return "string interpolation";
    case TOKEN_TRUE:
    case TOKEN_FALSE:
        return "boolean literal";
    case TOKEN_IDENTIFIER:
        return "<identifier>";
    case TOKEN_KEYWORD_CONSTANT:
        return "keyword `constant`";
    case TOKEN_KEYWORD_FUNCTION:
        return "keyword `function`";
    case TOKEN_KEYWORD_VAR:
        return "keyword `var`";
    case TOKEN_KEYWORD_IF:
        return "keyword `if`";
    case TOKEN_KEYWORD_ELSE:
        return "keyword `else`";
    case TOKEN_KEYWORD_WHILE:
        return "keyword `while`";
    case TOKEN_KEYWORD_TYPE:
        return "keyword `type`";

    case TOKEN_WHITESPACE:
        return "<whitespace>";
    case TOKEN_COMMENT:
        return "<comment>";

    case TOKEN_PLUS:
        return "token `+`";
    case TOKEN_MINUS:
        return "token `-`";
    case TOKEN_STAR:
        return "token `*`";
    case TOKEN_SLASH:
        return "token `/`";

    case TOKEN_LESS:
        return "token `<`";
    case TOKEN_LESS_EQUAL:
        return "token `<=`";
    case TOKEN_GREATER:
        return "token `>`";
    case TOKEN_GREATER_EQUAL:
        return "token `>=`";
    case TOKEN_EQUAL_EQUAL:
        return "token `==`";
    case TOKEN_BANG:
        return "token `!`";
    case TOKEN_BANG_EQUAL:
        return "token `!=`";
    case TOKEN_AND_AND:
        return "token `&&`";
    case TOKEN_PIPE_PIPE:
        return "token `||`";

    case TOKEN_DOT:
        return "token `.`";
    case TOKEN_DOT_DOT:
        return "token `..`";
    case TOKEN_COLON:
        return "token `:`";
    case TOKEN_COLON_EQUAL:
        return "token `:=`";

    case TOKEN_OPEN_PAREN:
        return "token `(`";
    case TOKEN_CLOSE_PAREN:
        return "token `)`";
    case TOKEN_OPEN_BRACE:
        return "token `{`";
    case TOKEN_CLOSE_BRACE:
        return "token `}`";
    case TOKEN_OPEN_BRACKET:
        return "token `[`";
    case TOKEN_CLOSE_BRACKET:
        return "token `]`";
    case TOKEN_COMMA:
        return "token `,`";
    case TOKEN_SEMICOLON:
        return "token `;`";
    default:
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
}

Nob_String_View _dpl_line_view(DPL* dpl, const char* line_start) {
    Nob_String_View line = nob_sv_from_parts(line_start, 0);
    while ((line.count < (size_t)((dpl->source.data + dpl->source.count) - line_start))
            && *(line.data + line.count) != '\n'
            && *(line.data + line.count) != '\0'
          ) {
        line.count++;
    }

    return line;
}

void _dpll_print_token(FILE *f, DPL* dpl, DPL_Token token)
{
    Nob_String_View line_view = _dpl_line_view(dpl, token.location.line_start);

    fprintf(f, "%3zu| "SV_Fmt"\n", token.location.line + 1, SV_Arg(line_view));
    fprintf(f, "   | ");

    for (size_t i = 0; i < token.location.column; ++i) {
        fprintf(f, " ");
    }
    fprintf(f, "^");
    if (token.text.count > 1) {
        for (size_t i = 0; i < token.text.count - 1; ++i) {
            fprintf(f, "~");
        }
    }
    fprintf(f, "\n");
}

void _dpll_print_token_range(FILE* out, DPL* dpl, DPL_Token first, DPL_Token last)
{
    const char* line = first.location.line_start;
    for (size_t line_num = first.location.line; line_num <= last.location.line; ++line_num) {
        Nob_String_View line_view = _dpl_line_view(dpl, line);
        fprintf(out, "%3zu| "SV_Fmt"\n", line_num + 1, SV_Arg(line_view));
        fprintf(out, "   | ");

        size_t start_column = 0;
        if (line_num == first.location.line) {
            start_column = first.location.column;
        }

        size_t end_column = line_view.count;
        if (line_num == last.location.line) {
            end_column = last.location.column + last.text.count;
        }

        for (size_t i = 0; i < start_column; ++i) {
            fprintf(out, " ");
        }

        size_t length = end_column - start_column;
        if (line_num == first.location.line) {
            fprintf(out, "^");
            if (length > 0) {
                length--;
            }
        }

        if (length > 1) {
            for (size_t i = 0; i < length - 1; ++i) {
                fprintf(out, "~");
            }
            if (line_num == last.location.line) {
                fprintf(out, "^");
            } else {
                fprintf(out, "~");
            }
        }
        fprintf(out, "\n");

        line += line_view.count + 1;
    }
}

void _dpll_advance(DPL* dpl)
{
    dpl->position++;
    dpl->column++;
}

bool _dpll_is_eof(DPL* dpl)
{
    return (dpl->position >= dpl->source.count);
}

char _dpll_current(DPL* dpl)
{
    return *(dpl->source.data + dpl->position);
}

bool _dpll_match(DPL* dpl, char c) {
    if (_dpll_current(dpl) == c) {
        _dpll_advance(dpl);
        return true;
    }
    return false;
}

DPL_Location _dpll_current_location(DPL* dpl)
{
    return (DPL_Location) {
        .file_name = dpl->file_name,
        .line = dpl->line,
        .column = dpl->column,
        .line_start = dpl->current_line,
    };
}

DPL_Token _dpll_build_token(DPL* dpl, DPL_TokenKind kind)
{
    DPL_Token token = {
        .kind = kind,
        .location = dpl->token_start_location,
        .text = nob_sv_from_parts(
            dpl->source.data + dpl->token_start,
            dpl->position - dpl->token_start
        ),
    };

    if (dpl->first_token.kind == TOKEN_NONE) {
        dpl->first_token = token;
    }

    if (dpl->debug) {
        printf(LOC_Fmt" - %s: "SV_Fmt"\n", LOC_Arg(token.location), _dpll_token_kind_name(token.kind), SV_Arg(token.text));
    }

    return token;
}

DPL_Token _dpll_build_empty_token(DPL* dpl, DPL_TokenKind kind)
{
    DPL_Token token = {
        .kind = kind,
        .location = dpl->token_start_location,
        .text = SV_NULL,
    };

    if (dpl->debug) {
        printf(LOC_Fmt" - %s\n", LOC_Arg(token.location), _dpll_token_kind_name(token.kind));
    }

    return token;
}

bool _dpll_is_ident_begin(char c) {
    return c == '_' ||  isalpha(c);
}

bool _dpll_is_ident(char c) {
    return c == '_' ||  isalnum(c);
}

DPL_Token _dpll_string(DPL* dpl) {
    bool in_escape = false;
    while (true) {
        if (_dpll_is_eof(dpl)) {
            DPL_LEXER_ERROR(dpl, "Unterminated string literal.");
        }

        if (_dpll_current(dpl) == '$') {
            _dpll_advance(dpl);
            if (_dpll_current(dpl) != '{') {
                continue;
            }

            _dpll_advance(dpl);
            DPL_Token token = _dpll_build_token(dpl, TOKEN_STRING_INTERPOLATION);
            if (dpl->interpolation_depth < DPL_MAX_INTERPOLATION) {

                dpl->interpolation_brackets[dpl->interpolation_depth] = 1;
                dpl->interpolation_depth++;
                return token;
            }

            DPL_TOKEN_ERROR(dpl, token, "String interpolation may nest only %d levels deep.", DPL_MAX_INTERPOLATION);
        }

        if (_dpll_current(dpl) == '"') {
            if (!in_escape) {
                _dpll_advance(dpl);
                return _dpll_build_token(dpl, TOKEN_STRING);
            }
        }

        if (!in_escape && _dpll_current(dpl) == '\\') {
            in_escape = true;
        } else {
            in_escape = false;
        }

        _dpll_advance(dpl);
    }
}

DPL_Token _dpll_next_token(DPL* dpl)
{
    if (dpl->peek_token.kind != TOKEN_NONE) {
        DPL_Token peeked = dpl->peek_token;
        dpl->peek_token = (DPL_Token) {
            0
        };
        return peeked;
    }

    dpl->token_start = dpl->position;
    dpl->token_start_location = _dpll_current_location(dpl);

    if (_dpll_is_eof(dpl)) {
        return _dpll_build_empty_token(dpl, TOKEN_EOF);
    }

    if (isspace(_dpll_current(dpl))) {
        while (!_dpll_is_eof(dpl) && isspace(_dpll_current(dpl))) {
            if (_dpll_current(dpl) == '\n')
            {
                dpl->position++;
                dpl->line++;
                dpl->column = 0;
                dpl->current_line = dpl->source.data + dpl->position;
            } else {
                _dpll_advance(dpl);
            }
        }
        return _dpll_build_token(dpl, TOKEN_WHITESPACE);
    }

    switch (_dpll_current(dpl))
    {
    case '+':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_PLUS);
    case '-':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_MINUS);
    case '*':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_STAR);
    case '/':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_SLASH);
    case '.':
        _dpll_advance(dpl);
        if (_dpll_match(dpl, '.')) {
            return _dpll_build_token(dpl, TOKEN_DOT_DOT);
        }
        return _dpll_build_token(dpl, TOKEN_DOT);
    case ':':
        _dpll_advance(dpl);
        if (_dpll_match(dpl, '=')) {
            return _dpll_build_token(dpl, TOKEN_COLON_EQUAL);
        }
        return _dpll_build_token(dpl, TOKEN_COLON);
    case '<':
        _dpll_advance(dpl);
        if (_dpll_match(dpl, '=')) {
            return _dpll_build_token(dpl, TOKEN_LESS_EQUAL);
        }
        return _dpll_build_token(dpl, TOKEN_LESS);
    case '>':
        _dpll_advance(dpl);
        if (_dpll_match(dpl, '=')) {
            return _dpll_build_token(dpl, TOKEN_GREATER_EQUAL);
        }
        return _dpll_build_token(dpl, TOKEN_GREATER);
    case '=':
        if ((dpl->position < dpl->source.count - 2) && (dpl->source.data[dpl->position + 1] == '=')) {
            _dpll_advance(dpl);
            _dpll_advance(dpl);
            return _dpll_build_token(dpl, TOKEN_EQUAL_EQUAL);
        }
        break;
    case '&':
        if ((dpl->position < dpl->source.count - 2) && (dpl->source.data[dpl->position + 1] == '&')) {
            _dpll_advance(dpl);
            _dpll_advance(dpl);
            return _dpll_build_token(dpl, TOKEN_AND_AND);
        }
        break;
    case '|':
        if ((dpl->position < dpl->source.count - 2) && (dpl->source.data[dpl->position + 1] == '|')) {
            _dpll_advance(dpl);
            _dpll_advance(dpl);
            return _dpll_build_token(dpl, TOKEN_PIPE_PIPE);
        }
        break;
    case '!':
        _dpll_advance(dpl);
        if (_dpll_match(dpl, '=')) {
            return _dpll_build_token(dpl, TOKEN_BANG_EQUAL);
        }
        return _dpll_build_token(dpl, TOKEN_BANG);
    case '(':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_OPEN_PAREN);
    case ')':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_CLOSE_PAREN);
    case '{':
        _dpll_advance(dpl);
        if (dpl->interpolation_depth > 0) {
            dpl->interpolation_brackets[dpl->interpolation_depth - 1]++;
        }
        return _dpll_build_token(dpl, TOKEN_OPEN_BRACE);
    case '}':
        _dpll_advance(dpl);
        if (dpl->interpolation_depth > 0) {
            dpl->interpolation_brackets[dpl->interpolation_depth - 1]--;
            if (dpl->interpolation_brackets[dpl->interpolation_depth - 1] == 0) {
                dpl->interpolation_depth--;
                return _dpll_string(dpl);
            }
        }
        return _dpll_build_token(dpl, TOKEN_CLOSE_BRACE);
    case '[':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_OPEN_BRACKET);
    case ']':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_CLOSE_BRACKET);
    case ',':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_COMMA);
    case ';':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_SEMICOLON);
    case '#':
        _dpll_advance(dpl);
        while (!_dpll_is_eof(dpl)) {
            if (_dpll_current(dpl) == '\n') {
                dpl->position++;
                dpl->line++;
                dpl->column = 0;
                dpl->current_line = dpl->source.data + dpl->position;
                return _dpll_build_token(dpl, TOKEN_COMMENT);
            }
            _dpll_advance(dpl);
        }
        return _dpll_build_token(dpl, TOKEN_COMMENT);
    case '"':
        _dpll_advance(dpl);
        return _dpll_string(dpl);
    }

    if (isdigit(_dpll_current(dpl)))
    {
        // integral part
        while (!_dpll_is_eof(dpl) && isdigit(_dpll_current(dpl)))
            _dpll_advance(dpl);

        if ((dpl->position >= dpl->source.count - 1)
                || *(dpl->source.data + dpl->position) != '.'
                || !isdigit(*(dpl->source.data + dpl->position + 1)))
        {
            return _dpll_build_token(dpl, TOKEN_NUMBER);
        }
        _dpll_advance(dpl); // skip the '.'

        // fractional part
        while (!_dpll_is_eof(dpl) && isdigit(_dpll_current(dpl)))
            _dpll_advance(dpl);

        return _dpll_build_token(dpl, TOKEN_NUMBER);
    }

    if (_dpll_is_ident_begin(_dpll_current(dpl)))
    {
        while (!_dpll_is_eof(dpl) && _dpll_is_ident(_dpll_current(dpl)))
            _dpll_advance(dpl);

        DPL_Token t = _dpll_build_token(dpl, TOKEN_IDENTIFIER);
        if (nob_sv_eq(t.text, nob_sv_from_cstr("constant"))) {
            t.kind = TOKEN_KEYWORD_CONSTANT;
        } else if (nob_sv_eq(t.text, nob_sv_from_cstr("function"))) {
            t.kind = TOKEN_KEYWORD_FUNCTION;
        } else if (nob_sv_eq(t.text, nob_sv_from_cstr("var"))) {
            t.kind = TOKEN_KEYWORD_VAR;
        } else if (nob_sv_eq(t.text, nob_sv_from_cstr("if"))) {
            t.kind = TOKEN_KEYWORD_IF;
        } else if (nob_sv_eq(t.text, nob_sv_from_cstr("else"))) {
            t.kind = TOKEN_KEYWORD_ELSE;
        } else if (nob_sv_eq(t.text, nob_sv_from_cstr("true"))) {
            t.kind = TOKEN_TRUE;
        } else if (nob_sv_eq(t.text, nob_sv_from_cstr("while"))) {
            t.kind = TOKEN_KEYWORD_WHILE;
        } else if (nob_sv_eq(t.text, nob_sv_from_cstr("false"))) {
            t.kind = TOKEN_FALSE;
        } else if (nob_sv_eq(t.text, nob_sv_from_cstr("type"))) {
            t.kind = TOKEN_KEYWORD_TYPE;
        }

        return t;
    }

    DPL_LEXER_ERROR(dpl, "Invalid character '%c'.", _dpll_current(dpl));
}

DPL_Token _dpll_peek_token(DPL *dpl) {
    if (dpl->peek_token.kind == TOKEN_NONE) {
        dpl->peek_token = _dpll_next_token(dpl);
    }
    return dpl->peek_token;
}

// AST

DPL_Ast_Node* _dpla_create_node(DPL_Ast_Tree* tree, DPL_AstNodeKind kind, DPL_Token first, DPL_Token last)
{
    DPL_Ast_Node* node = arena_alloc(&tree->memory, sizeof(DPL_Ast_Node));
    node->kind = kind;
    node->first = first;
    node->last = last;
    return node;
}

DPL_Ast_Node* _dpla_create_literal(DPL_Ast_Tree* tree, DPL_Token token) {
    DPL_Ast_Node* node = _dpla_create_node(tree, AST_NODE_LITERAL, token, token);
    node->as.literal.value = token;
    return node;
}

const char* _dpla_node_kind_name(DPL_AstNodeKind kind) {
    switch (kind) {
    case AST_NODE_LITERAL:
        return "`literal`";
    case AST_NODE_OBJECT_LITERAL:
        return "`object literal`";
    case AST_NODE_UNARY:
        return "`unary operator`";
    case AST_NODE_BINARY:
        return "`binary operator`";
    case AST_NODE_FUNCTIONCALL:
        return "`function call`";
    case AST_NODE_SCOPE:
        return "`scope`";
    case AST_NODE_DECLARATION:
        return "`declaration`";
    case AST_NODE_SYMBOL:
        return "`symbol`";
    case AST_NODE_ASSIGNMENT:
        return "`assignment`";
    case AST_NODE_FUNCTION:
        return "`function`";
    case AST_NODE_CONDITIONAL:
        return "`conditional`";
    case AST_NODE_WHILE_LOOP:
        return "`while loop`";
    case AST_NODE_FIELD_ACCESS:
        return "`field access`";
    case AST_NODE_INTERPOLATION:
        return "`interpolation`";
    default:
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
}

void _dpla_build_type_name(DPL_Ast_Type* ast_type, Nob_String_Builder* sb) {
    switch (ast_type->kind) {
    case TYPE_BASE:
        nob_sb_append_sv(sb, ast_type->as.name.text);
        break;
    case TYPE_OBJECT: {
        nob_sb_append_cstr(sb, "[");
        for (size_t i = 0; i < ast_type->as.object.field_count; ++i) {
            if (i > 0) {
                nob_sb_append_cstr(sb, ", ");
            }
            DPL_Ast_TypeField field = ast_type->as.object.fields[i];
            nob_sb_append_sv(sb, field.name.text);
            nob_sb_append_cstr(sb, ": ");
            _dpla_build_type_name(field.type, sb);
        }
        nob_sb_append_cstr(sb, "]");
    }
    break;
    default:
        DW_UNIMPLEMENTED_MSG("Cannot print ast type %d.", ast_type->kind);
    }
}

const char* _dpla_type_name(DPL_Ast_Type* ast_type) {
    static char result[256];
    if (!ast_type) {
        return _dpll_token_kind_name(TOKEN_NONE);
    }
    Nob_String_Builder sb = {0};
    _dpla_build_type_name(ast_type, &sb);
    nob_sb_append_null(&sb);

    strncpy(result, sb.items, NOB_ARRAY_LEN(result));
    return result;
}

void _dpla_print_indent(size_t level) {
    for (size_t i = 0; i < level; ++i) {
        printf("  ");
    }
}

void _dpla_print(DPL_Ast_Node* node, size_t level) {
    _dpla_print_indent(level);

    if (!node) {
        printf("<nil>\n");
        return;
    }

    printf("%s", _dpla_node_kind_name(node->kind));

    switch (node->kind) {
    case AST_NODE_UNARY: {
        printf(" [%s]\n", _dpll_token_kind_name(node->as.unary.operator.kind));
        _dpla_print(node->as.unary.operand, level + 1);
        break;
    }
    case AST_NODE_BINARY: {
        printf(" [%s]\n", _dpll_token_kind_name(node->as.binary.operator.kind));
        _dpla_print(node->as.binary.left, level + 1);
        _dpla_print(node->as.binary.right, level + 1);
        break;
    }
    case AST_NODE_LITERAL:
    case AST_NODE_SYMBOL: {
        printf(" [%s: "SV_Fmt"]\n", _dpll_token_kind_name(node->as.literal.value.kind), SV_Arg(node->as.literal.value.text));
        break;
    }
    case AST_NODE_OBJECT_LITERAL: {
        DPL_Ast_ObjectLiteral object_literal = node->as.object_literal;
        printf("\n");
        for (size_t i = 0; i < object_literal.field_count; ++i) {
            _dpla_print_indent(level + 1);
            printf("<field #%zu>\n", i);
            _dpla_print(object_literal.fields[i], level + 2);
        }
        break;
    }
    break;
    case AST_NODE_FIELD_ACCESS: {
        DPL_Ast_FieldAccess field_access = node->as.field_access;
        printf("\n");

        _dpla_print_indent(level + 1);
        printf("<expression>\n");
        _dpla_print(field_access.expression, level + 2);

        _dpla_print_indent(level + 1);
        printf("<field>\n");
        _dpla_print(field_access.field, level + 2);
    }
    break;
    case AST_NODE_FUNCTIONCALL: {
        DPL_Ast_FunctionCall call = node->as.function_call;
        printf(" [%s: "SV_Fmt"]\n", _dpll_token_kind_name(call.name.kind), SV_Arg(call.name.text));
        for (size_t i = 0; i < call.argument_count; ++i) {
            _dpla_print_indent(level + 1);
            printf("<arg #%zu>\n", i);
            _dpla_print(call.arguments[i], level + 2);
        }
        break;
    }
    case AST_NODE_SCOPE: {
        DPL_Ast_Scope scope = node->as.scope;
        printf("\n");
        for (size_t i = 0; i < scope.expression_count; ++i) {
            _dpla_print_indent(level + 1);
            printf("<expr #%zu>\n", i);
            _dpla_print(scope.expressions[i], level + 2);
        }
    }
    break;
    case AST_NODE_DECLARATION: {
        DPL_Ast_Declaration declaration = node->as.declaration;
        printf(" [%s "SV_Fmt": %s]\n", _dpll_token_kind_name(declaration.keyword.kind), SV_Arg(declaration.name.text), _dpla_type_name(declaration.type));
        _dpla_print_indent(level + 1);
        printf("<initialization>\n");
        _dpla_print(declaration.initialization, level + 2);
    }
    break;
    case AST_NODE_ASSIGNMENT: {
        DPL_Ast_Assignment assigment = node->as.assignment;
        printf("\n");

        _dpla_print_indent(level + 1);
        printf("<target>\n");
        _dpla_print(assigment.target, level + 2);

        _dpla_print_indent(level + 1);
        printf("<exression>\n");
        _dpla_print(assigment.expression, level + 2);
    }
    break;
    case AST_NODE_FUNCTION: {
        DPL_Ast_Function function = node->as.function;
        printf(" [%s "SV_Fmt": (", _dpll_token_kind_name(function.keyword.kind), SV_Arg(function.name.text));
        for (size_t i = 0; i < function.signature.argument_count; ++i) {
            if (i > 0) {
                printf(", ");
            }
            DPL_Ast_FunctionArgument arg = function.signature.arguments[i];
            printf(SV_Fmt": %s", SV_Arg(arg.name.text), _dpla_type_name(arg.type));
        }
        printf("): %s]\n", _dpla_type_name(function.signature.type));
        _dpla_print_indent(level + 1);
        printf("<body>\n");
        _dpla_print(function.body, level + 2);
    }
    break;
    case AST_NODE_CONDITIONAL: {
        DPL_Ast_Conditional conditional = node->as.conditional;
        printf("\n");

        _dpla_print_indent(level + 1);
        printf("<condition>\n");
        _dpla_print(conditional.condition, level + 2);

        _dpla_print_indent(level + 1);
        printf("<then_clause>\n");
        _dpla_print(conditional.then_clause, level + 2);

        _dpla_print_indent(level + 1);
        printf("<else_clause>\n");
        _dpla_print(conditional.else_clause, level + 2);
    }
    break;
    case AST_NODE_WHILE_LOOP: {
        DPL_Ast_WhileLoop while_loop = node->as.while_loop;
        printf("\n");

        _dpla_print_indent(level + 1);
        printf("<condition>\n");
        _dpla_print(while_loop.condition, level + 2);

        _dpla_print_indent(level + 1);
        printf("<body>\n");
        _dpla_print(while_loop.body, level + 2);
    }
    break;
    case AST_NODE_INTERPOLATION: {
        DPL_Ast_Interpolation interpolation = node->as.interpolation;
        printf("\n");
        for (size_t i = 0; i < interpolation.expression_count; ++i) {
            _dpla_print_indent(level + 1);
            printf("<expr #%zu>\n", i);
            _dpla_print(interpolation.expressions[i], level + 2);
        }
        break;
    }
    default: {
        printf("\n");
        break;
    }
    }
}

// PARSER

void _dplp_skip_whitespace(DPL *dpl) {
    while (_dpll_peek_token(dpl).kind == TOKEN_WHITESPACE || _dpll_peek_token(dpl).kind == TOKEN_COMMENT) {
        _dpll_next_token(dpl);
    }
}

DPL_Token _dplp_next_token(DPL *dpl) {
    _dplp_skip_whitespace(dpl);
    return _dpll_next_token(dpl);
}

DPL_Token _dplp_peek_token(DPL *dpl) {
    _dplp_skip_whitespace(dpl);
    return _dpll_peek_token(dpl);
}

void _dplp_check_token(DPL *dpl, DPL_Token token, DPL_TokenKind kind) {
    if (token.kind != kind) {
        DPL_TOKEN_ERROR(dpl, token, "Unexpected %s, expected %s.",
                        _dpll_token_kind_name(token.kind), _dpll_token_kind_name(kind));
    }
}

DPL_Token _dplp_expect_token(DPL *dpl, DPL_TokenKind kind) {
    DPL_Token next_token = _dplp_next_token(dpl);
    _dplp_check_token(dpl, next_token, kind);
    return next_token;
}

DPL_Ast_Node* _dplp_parse_expression(DPL* dpl);
DPL_Ast_Node* _dplp_parse_scope(DPL* dpl, DPL_Token opening_token, DPL_TokenKind closing_token_kind);

int _dplp_compare_object_type_fields(void const* a, void const* b)
{
    return nob_sv_cmp(((DPL_Ast_TypeField*) a)->name.text, ((DPL_Ast_TypeField*) b)->name.text);
}


DPL_Ast_Type* _dplp_parse_type(DPL* dpl) {
    DPL_Token type_begin = _dplp_peek_token(dpl);
    switch (type_begin.kind) {
    case TOKEN_IDENTIFIER: {
        _dplp_next_token(dpl);

        DPL_Ast_Type* name_type = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_Type));
        name_type->kind = TYPE_BASE;
        name_type->first = type_begin;
        name_type->last = type_begin;
        name_type->as.name = type_begin;
        return name_type;
    }
    case TOKEN_OPEN_BRACKET: {
        _dplp_next_token(dpl);

        da_array(DPL_Ast_TypeField) tmp_fields = NULL;
        while (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_BRACKET) {
            if (da_size(tmp_fields) > 0) {
                _dplp_expect_token(dpl, TOKEN_COMMA);
            }
            DPL_Token name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);
            _dplp_expect_token(dpl, TOKEN_COLON);
            DPL_Ast_Type* type = _dplp_parse_type(dpl);
            DPL_Ast_TypeField field = {
                .name = name,
                .first = name,
                .last = type->last,
                .type = type,
            };

            for (size_t i = 0; i < da_size(tmp_fields); ++i) {
                if (nob_sv_eq(tmp_fields[i].name.text, name.text)) {
                    DPL_AST_ERROR_WITH_NOTE(dpl,
                                            &tmp_fields[i], "Previous declaration was here.",
                                            &field, "Duplicate field `"SV_Fmt"` in object type.", SV_Arg(name.text));
                }
            }
            da_add(tmp_fields, field);
        }
        qsort(tmp_fields, da_size(tmp_fields), sizeof(*tmp_fields), _dplp_compare_object_type_fields);


        DPL_Ast_TypeField* fields = NULL;
        if (da_some(tmp_fields)) {
            fields = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_TypeField) * da_size(tmp_fields));
            memcpy(fields, tmp_fields, sizeof(DPL_Ast_TypeField) * da_size(tmp_fields));
        }

        DPL_Ast_Type* object_type = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_Type));
        object_type->kind = TYPE_OBJECT;
        object_type->first = type_begin;
        object_type->last = _dplp_peek_token(dpl);
        object_type->as.object.field_count = da_size(tmp_fields);
        object_type->as.object.fields = fields;

        da_free(tmp_fields);
        _dplp_next_token(dpl);

        return object_type;
    }
    default:
        DPL_TOKEN_ERROR(dpl, type_begin, "Invalid %s in type reference, expected %s.", _dpll_token_kind_name(type_begin.kind),
                        _dpll_token_kind_name(TOKEN_IDENTIFIER));
    }
}

DPL_Ast_Node* _dplp_parse_declaration(DPL* dpl) {
    DPL_Token keyword_candidate = _dplp_peek_token(dpl);
    if (keyword_candidate.kind == TOKEN_KEYWORD_CONSTANT || keyword_candidate.kind == TOKEN_KEYWORD_VAR) {
        DPL_Token keyword = _dplp_next_token(dpl);
        DPL_Token name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);

        DPL_Ast_Type* type = NULL;
        if (_dplp_peek_token(dpl).kind == TOKEN_COLON) {
            _dplp_expect_token(dpl, TOKEN_COLON);
            type = _dplp_parse_type(dpl);
        }

        DPL_Token assignment = _dplp_expect_token(dpl, TOKEN_COLON_EQUAL);

        DPL_Ast_Node* initialization = _dplp_parse_expression(dpl);

        DPL_Ast_Node* declaration = _dpla_create_node(&dpl->tree, AST_NODE_DECLARATION, keyword, initialization->last);
        declaration->as.declaration.keyword = keyword;
        declaration->as.declaration.name = name;
        declaration->as.declaration.type = type;
        declaration->as.declaration.assignment = assignment;
        declaration->as.declaration.initialization = initialization;
        return declaration;
    } else if (keyword_candidate.kind == TOKEN_KEYWORD_FUNCTION) {
        DPL_Token keyword = _dplp_next_token(dpl);
        DPL_Token name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);

        da_array(DPL_Ast_FunctionArgument) arguments = 0;
        _dplp_expect_token(dpl, TOKEN_OPEN_PAREN);
        if (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_PAREN) {
            while (true) {
                DPL_Ast_FunctionArgument argument = {0};
                argument.name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);
                _dplp_expect_token(dpl, TOKEN_COLON);
                argument.type = _dplp_parse_type(dpl);

                da_add(arguments, argument);

                if (_dplp_peek_token(dpl).kind != TOKEN_COMMA) {
                    break;
                } else {
                    _dplp_next_token(dpl);
                }
            }
        }
        _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);

        DPL_Ast_Type* result_type = NULL;
        if (_dplp_peek_token(dpl).kind == TOKEN_COLON) {
            _dplp_expect_token(dpl, TOKEN_COLON);
            result_type = _dplp_parse_type(dpl);
        }

        _dplp_expect_token(dpl, TOKEN_COLON_EQUAL);

        DPL_Ast_Node *body = _dplp_parse_expression(dpl);

        DPL_Ast_Node* function = _dpla_create_node(&dpl->tree, AST_NODE_FUNCTION, keyword, body->last);
        function->as.function.keyword = keyword;
        function->as.function.name = name;

        function->as.function.signature.argument_count = da_size(arguments);
        if (da_some(arguments)) {
            function->as.function.signature.arguments = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_FunctionArgument) * da_size(arguments));
            memcpy(function->as.function.signature.arguments, arguments, sizeof(DPL_Ast_FunctionArgument) * da_size(arguments));
            da_free(arguments);
        }
        function->as.function.signature.type = result_type;

        function->as.function.body = body;

        return function;
    } else if (keyword_candidate.kind == TOKEN_KEYWORD_TYPE) {
        DPL_Token keyword = _dplp_next_token(dpl);
        DPL_Token name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);

        DPL_Token assignment = _dplp_expect_token(dpl, TOKEN_COLON_EQUAL);

        DPL_Ast_Type* type = _dplp_parse_type(dpl);
        if (_dplp_peek_token(dpl).kind == TOKEN_COLON) {
            _dplp_expect_token(dpl, TOKEN_COLON);
            type = _dplp_parse_type(dpl);
        }

        DPL_Ast_Node* declaration = _dpla_create_node(&dpl->tree, AST_NODE_DECLARATION, keyword, type->last);
        declaration->as.declaration.keyword = keyword;
        declaration->as.declaration.name = name;
        declaration->as.declaration.type = type;
        declaration->as.declaration.assignment = assignment;
        declaration->as.declaration.initialization = NULL;
        return declaration;
    }

    return _dplp_parse_expression(dpl);

}
void _dplp_move_nodelist(DPL* dpl, da_array(DPL_Ast_Node*) list, size_t *target_count, DPL_Ast_Node*** target_items) {
    *target_count = da_size(list);
    if (da_some(list)) {
        *target_items = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_Node*) * da_size(list));
        memcpy(*target_items, list, sizeof(DPL_Ast_Node*) * da_size(list));
        da_free(list);
    }
}

da_array(DPL_Ast_Node*) _dplp_parse_expressions(DPL* dpl, DPL_TokenKind delimiter, DPL_TokenKind closing, bool allow_declarations)
{
    da_array(DPL_Ast_Node*) list = 0;
    if (allow_declarations) {
        da_add(list, _dplp_parse_declaration(dpl));
    } else {
        da_add(list, _dplp_parse_expression(dpl));
    }

    DPL_Token delimiter_candidate = _dplp_peek_token(dpl);
    while (delimiter_candidate.kind == delimiter) {
        _dplp_next_token(dpl);
        if (_dplp_peek_token(dpl).kind == closing) {
            break;
        }

        if (allow_declarations) {
            da_add(list, _dplp_parse_declaration(dpl));
        } else {
            da_add(list, _dplp_parse_expression(dpl));
        }
        delimiter_candidate = _dplp_peek_token(dpl);
    }

    return list;
}

DPL_Ast_Node* _dplp_parse_primary(DPL* dpl)
{
    DPL_Token token = _dplp_next_token(dpl);
    switch (token.kind) {
    case TOKEN_NUMBER:
    case TOKEN_STRING:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
        return _dpla_create_literal(&dpl->tree, token);
    case TOKEN_OPEN_PAREN: {
        DPL_Ast_Node* node = _dplp_parse_expression(dpl);
        node->first = token;
        node->last = _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);
        return node;
    }
    break;
    case TOKEN_OPEN_BRACE: {
        DPL_Ast_Node* node = _dplp_parse_scope(dpl, token, TOKEN_CLOSE_BRACE);
        return node;
    }
    case TOKEN_IDENTIFIER: {
        if (_dplp_peek_token(dpl).kind == TOKEN_OPEN_PAREN) {
            DPL_Token name = token;
            /*DPL_Token open_paren =*/ _dplp_expect_token(dpl, TOKEN_OPEN_PAREN);

            da_array(DPL_Ast_Node*) arguments = 0;
            if (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_PAREN) {
                arguments = _dplp_parse_expressions(dpl, TOKEN_COMMA, TOKEN_CLOSE_PAREN, false);
            }

            DPL_Token closing_paren = _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);

            DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_FUNCTIONCALL, name, closing_paren);
            node->as.function_call.name = name;
            _dplp_move_nodelist(dpl, arguments, &node->as.function_call.argument_count, &node->as.function_call.arguments);
            return node;
        }

        DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_SYMBOL, token, token);
        node->as.symbol = token;
        return node;
    }
    case TOKEN_OPEN_BRACKET: {
        bool first = true;
        da_array(DPL_Ast_Node*) tmp_fields = NULL;
        while (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_BRACKET) {
            if (!first) {
                _dplp_expect_token(dpl, TOKEN_COMMA);
            }

            DPL_Ast_Node* field = _dplp_parse_expression(dpl);
            da_add(tmp_fields, field);

            first = false;
        }

        DPL_Ast_Node* object_literal = _dpla_create_node(&dpl->tree, AST_NODE_OBJECT_LITERAL, token, _dplp_next_token(dpl));
        _dplp_move_nodelist(dpl, tmp_fields, &object_literal->as.object_literal.field_count,
                            &object_literal->as.object_literal.fields);

        return object_literal;
    }
    break;
    case TOKEN_STRING_INTERPOLATION: {
        da_array(DPL_Ast_Node*) tmp_parts = NULL;

        while (token.kind == TOKEN_STRING_INTERPOLATION) {
            if (token.text.count > 3) {
                da_add(tmp_parts, _dpla_create_literal(&dpl->tree, token));
            }

            DPL_Ast_Node* expression = _dplp_parse_expression(dpl);
            da_add(tmp_parts, expression);

            token = _dplp_next_token(dpl);
        }

        _dplp_check_token(dpl, token, TOKEN_STRING);
        if (token.text.count > 2) {
            da_add(tmp_parts, _dpla_create_literal(&dpl->tree, token));
        }

        DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_INTERPOLATION,
                                               da_first(tmp_parts)->first, da_last(tmp_parts)->last);
        _dplp_move_nodelist(dpl, tmp_parts, &node->as.interpolation.expression_count, &node->as.interpolation.expressions);

        return node;
    }
    break;
    default: {
        DPL_TOKEN_ERROR(dpl, token, "Unexpected %s.", _dpll_token_kind_name(token.kind));
    }
    }
}

DPL_Ast_Node *_dplp_parse_dot(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parse_primary(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_DOT) {
        _dplp_next_token(dpl);
        DPL_Ast_Node* new_expression = _dplp_parse_primary(dpl);

        if (new_expression->kind == AST_NODE_SYMBOL) {
            DPL_Ast_Node* field_access = _dpla_create_node(&dpl->tree, AST_NODE_FIELD_ACCESS, expression->first, new_expression->last);
            field_access->as.field_access.expression = expression;
            field_access->as.field_access.field = new_expression;
            expression = field_access;
        } else if (new_expression->kind == AST_NODE_FUNCTIONCALL) {
            DPL_Ast_FunctionCall* fc = &new_expression->as.function_call;
            fc->arguments = arena_realloc(&dpl->tree.memory, fc->arguments,
                                          fc->argument_count * sizeof(DPL_Ast_Node*),
                                          (fc->argument_count + 1) * sizeof(DPL_Ast_Node*));
            fc->argument_count++;

            for (size_t i = fc->argument_count - 1; i > 0; --i) {
                fc->arguments[i] = fc->arguments[i - 1];
            }

            fc->arguments[0] = expression;
            expression = new_expression;
        } else {
            DPL_AST_ERROR(dpl, new_expression, "Right-hand operand of operator `.` must be either a symbol or a function call.");
        }

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_unary(DPL* dpl)
{
    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    if (operator_candidate.kind == TOKEN_MINUS || operator_candidate.kind == TOKEN_BANG || operator_candidate.kind == TOKEN_DOT_DOT) {
        DPL_Token operator = _dplp_next_token(dpl);
        DPL_Ast_Node* operand = _dplp_parse_unary(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_UNARY, operator, operand->last);
        new_expression->as.unary.operator = operator;
        new_expression->as.unary.operand = operand;
        return new_expression;
    }

    return _dplp_parse_dot(dpl);
}

DPL_Ast_Node* _dplp_parse_multiplicative(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parse_unary(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_STAR || operator_candidate.kind == TOKEN_SLASH) {
        _dplp_next_token(dpl);
        DPL_Ast_Node* rhs = _dplp_parse_unary(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_additive(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parse_multiplicative(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_PLUS || operator_candidate.kind == TOKEN_MINUS) {
        _dplp_next_token(dpl);
        DPL_Ast_Node* rhs = _dplp_parse_multiplicative(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_comparison(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parse_additive(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_LESS || operator_candidate.kind == TOKEN_LESS_EQUAL
            || operator_candidate.kind == TOKEN_GREATER || operator_candidate.kind == TOKEN_GREATER_EQUAL)  {
        _dplp_next_token(dpl);
        DPL_Ast_Node* rhs = _dplp_parse_additive(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_equality(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parse_comparison(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_EQUAL_EQUAL || operator_candidate.kind == TOKEN_BANG_EQUAL)  {
        _dplp_next_token(dpl);
        DPL_Ast_Node* rhs = _dplp_parse_comparison(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_and(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parse_equality(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_AND_AND)  {
        _dplp_next_token(dpl);
        DPL_Ast_Node* rhs = _dplp_parse_equality(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_or(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parse_and(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_PIPE_PIPE)  {
        _dplp_next_token(dpl);
        DPL_Ast_Node* rhs = _dplp_parse_and(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_conditional_or_loop(DPL* dpl)
{
    DPL_Token keyword_candidate = _dplp_peek_token(dpl);

    if (keyword_candidate.kind == TOKEN_KEYWORD_IF) {
        _dplp_next_token(dpl);

        _dplp_expect_token(dpl, TOKEN_OPEN_PAREN);
        DPL_Ast_Node* condition = _dplp_parse_expression(dpl);
        _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);
        DPL_Ast_Node* then_clause = _dplp_parse_expression(dpl);
        _dplp_expect_token(dpl, TOKEN_KEYWORD_ELSE);
        DPL_Ast_Node* else_clause = _dplp_parse_expression(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_CONDITIONAL, keyword_candidate, else_clause->last);
        new_expression->as.conditional.condition = condition;
        new_expression->as.conditional.then_clause = then_clause;
        new_expression->as.conditional.else_clause = else_clause;
        return new_expression;
    } else if (keyword_candidate.kind == TOKEN_KEYWORD_WHILE) {
        _dplp_next_token(dpl);

        _dplp_expect_token(dpl, TOKEN_OPEN_PAREN);
        DPL_Ast_Node* condition = _dplp_parse_expression(dpl);
        _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);
        DPL_Ast_Node* body = _dplp_parse_expression(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_WHILE_LOOP, keyword_candidate, body->last);
        new_expression->as.while_loop.condition = condition;
        new_expression->as.while_loop.body = body;
        return new_expression;
    }

    return _dplp_parse_or(dpl);
}

DPL_Ast_Node* _dplp_parse_assignment(DPL* dpl)
{
    DPL_Ast_Node* target = _dplp_parse_conditional_or_loop(dpl);

    if (_dplp_peek_token(dpl).kind == TOKEN_COLON_EQUAL) {
        if (target->kind == AST_NODE_FIELD_ACCESS) {
            DPL_AST_ERROR(dpl, target, "Object fields cannot be assigned directly. Instead, you can compose a new object from the old one.");

        } else if (target->kind != AST_NODE_SYMBOL) {
            DPL_AST_ERROR(dpl, target, "`%s` is not a valid assignment target.",
                          _dpla_node_kind_name(target->kind));
        }

        DPL_Token assignment = _dplp_next_token(dpl);
        DPL_Ast_Node* expression = _dplp_parse_assignment(dpl);

        DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_ASSIGNMENT, target->first, expression->last);
        node->as.assignment.target = target;
        node->as.assignment.assignment = assignment;
        node->as.assignment.expression = expression;
        return node;
    }

    return target;
}

DPL_Ast_Node* _dplp_parse_expression(DPL* dpl)
{
    return _dplp_parse_assignment(dpl);
}

DPL_Ast_Node* _dplp_parse_scope(DPL* dpl, DPL_Token opening_token, DPL_TokenKind closing_token_kind)
{
    DPL_Token closing_token_candidate = _dplp_peek_token(dpl);
    if (closing_token_candidate.kind == closing_token_kind) {
        DPL_TOKEN_ERROR(dpl, closing_token_candidate,
                        "Unexpected %s. Expected at least one expression in scope.",
                        _dpll_token_kind_name(closing_token_kind));
    }

    da_array(DPL_Ast_Node*) expressions = _dplp_parse_expressions(dpl, TOKEN_SEMICOLON, closing_token_kind, true);
    if (opening_token.kind == TOKEN_NONE) {
        opening_token = expressions[0]->first;
    }

    DPL_Token closing_token = _dplp_expect_token(dpl, closing_token_kind);

    DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_SCOPE, opening_token, closing_token);
    _dplp_move_nodelist(dpl, expressions, &node->as.scope.expression_count, &node->as.scope.expressions);
    return node;
}

void _dplp_parse(DPL* dpl)
{
    dpl->tree.root = _dplp_parse_scope(dpl, dpl->first_token, TOKEN_EOF);
}

// BOUND TREE

const char* _dplb_nodekind_name(DPL_BoundNodeKind kind) {
    switch (kind) {
    case BOUND_NODE_VALUE:
        return "BOUND_NODE_VALUE";
    case BOUND_NODE_OBJECT:
        return "BOUND_NODE_OBJECT";
    case BOUND_NODE_FUNCTIONCALL:
        return "BOUND_NODE_FUNCTIONCALL";
    case BOUND_NODE_SCOPE:
        return "BOUND_NODE_SCOPE";
    case BOUND_NODE_VARREF:
        return "BOUND_NODE_VARREF";
    case BOUND_NODE_ARGREF:
        return "BOUND_NODE_ARGREF";
    case BOUND_NODE_ASSIGNMENT:
        return "BOUND_NODE_ASSIGNMENT";
    case BOUND_NODE_CONDITIONAL:
        return "BOUND_NODE_CONDITIONAL";
    case BOUND_NODE_LOGICAL_OPERATOR:
        return "BOUND_NODE_LOGICAL_OPERATOR";
    case BOUND_NODE_WHILE_LOOP:
        return "BOUND_NODE_WHILE_LOOP";
    case BOUND_NODE_LOAD_FIELD:
        return "BOUND_NODE_LOAD_FIELD";
    case BOUND_NODE_INTERPOLATION:
        return "BOUND_NODE_INTERPOLATION";
    default:
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
}

void _dplb_move_nodelist(DPL* dpl, da_array(DPL_Bound_Node*) list, size_t *target_count, DPL_Bound_Node*** target_items) {
    *target_count = da_size(list);
    if (da_some(list)) {
        *target_items = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node*) * da_size(list));
        memcpy(*target_items, list, sizeof(DPL_Bound_Node*) * da_size(list));
        da_free(list);
    }
}

Nob_String_View _dpl_arena_strcpy(Arena* arena, const char* s) {
    char* copy = arena_alloc(arena, strlen(s));
    strcpy(copy, s);
    return nob_sv_from_cstr(copy);
}

Nob_String_View _dpl_arena_svcpy(Arena* arena, Nob_String_View sv) {
    char* copy = arena_alloc(arena, sv.count);
    memcpy(copy, sv.data, sv.count);
    return nob_sv_from_parts(copy, sv.count);
}

DPL_Symbol* _dplb_bind_type(DPL* dpl, DPL_Ast_Type* ast_type) {
    const char* type_name = _dpla_type_name(ast_type);

    DPL_Symbol* s = dpl_symbols_find_kind_cstr(&dpl->symbols, type_name, SYMBOL_TYPE);
    if (s) {
        return s;
    }

    DPL_Symbol* cached_type = dpl_symbols_find_cstr(&dpl->symbols, type_name);
    if (cached_type) {
        return cached_type;
    }

    if (ast_type->kind == TYPE_OBJECT) {
        DPL_Ast_TypeObject object_type = ast_type->as.object;

        DPL_Symbol* new_object_type = dpl_symbols_push_type_object_cstr(&dpl->symbols, type_name, object_type.field_count);
        DPL_Symbol_Type_ObjectField *fields = new_object_type->as.type.as.object.fields;

        for (size_t i = 0; i < object_type.field_count; ++i) {
            DPL_Ast_TypeField ast_field = object_type.fields[i];
            DPL_Symbol* bound_field_type = _dplb_bind_type(dpl, ast_field.type);
            if (!bound_field_type) {
                DPL_AST_ERROR(dpl, ast_field.type, "Unable to bind type `%s` for field `"SV_Fmt"`.",
                              _dpla_type_name(ast_field.type), SV_Arg(ast_field.name.text));
            }
            fields[i].name = ast_field.name.text;
            fields[i].type = bound_field_type;
        }

        return new_object_type;
    }

    return NULL;
}

void _dplb_check_function_used(DPL* dpl, DPL_Symbol* symbol) {
    DPL_Symbol_Function* f = &symbol->as.function;
    if (f->kind == FUNCTION_USER && !f->as.user_function.used) {
        f->as.user_function.used = true;
        f->as.user_function.user_handle = da_size(dpl->user_functions);

        DPL_UserFunction user_function = {
            .function = symbol,
            .arity = f->signature.argument_count,
            .begin_ip = 0,
            .body = (DPL_Bound_Node*) f->as.user_function.body,
        };
        da_add(dpl->user_functions, user_function);
    }
}

DPL_Bound_Node* _dplb_bind_node(DPL* dpl, DPL_Ast_Node* node);

DPL_Bound_Node* _dplb_bind_unary_function_call(DPL* dpl, DPL_Bound_Node* operand, const char* function_name)
{
    DPL_Symbol* function_symbol = dpl_symbols_find_function1_cstr(&dpl->symbols, function_name, operand->type);

    if (function_symbol) {
        _dplb_check_function_used(dpl, function_symbol);

        DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        bound_node->kind = BOUND_NODE_FUNCTIONCALL;
        bound_node->type = function_symbol->as.function.signature.returns;
        bound_node->as.function_call.function = function_symbol;

        da_array(DPL_Bound_Node*) temp_arguments = 0;
        da_add(temp_arguments, operand);
        _dplb_move_nodelist(dpl, temp_arguments, &bound_node->as.function_call.arguments_count, &bound_node->as.function_call.arguments);
        return bound_node;
    }

    return NULL;
}

DPL_Bound_Node* _dplb_bind_unary(DPL* dpl, DPL_Ast_Node* node, const char* function_name)
{
    DPL_Bound_Node* operand = _dplb_bind_node(dpl, node->as.unary.operand);
    if (!operand) {
        DPL_AST_ERROR(dpl, node, "Cannot bind operand of unary expression.");
    }

    DPL_Bound_Node* bound_unary = _dplb_bind_unary_function_call(dpl, operand, function_name);
    if (!bound_unary) {
        DPL_Token operator_token = node->as.unary.operator;
        DPL_AST_ERROR(dpl, node, "Cannot resolve function \"%s("SV_Fmt")\" for unary operator \""SV_Fmt"\".",
                      function_name, SV_Arg(operand->type->name), SV_Arg(operator_token.text));
    }

    return bound_unary;
}

DPL_Bound_Node* _dplb_bind_binary(DPL* dpl, DPL_Ast_Node* node, const char* function_name)
{
    DPL_Bound_Node* lhs = _dplb_bind_node(dpl, node->as.binary.left);
    if (!lhs) {
        DPL_AST_ERROR(dpl, node, "Cannot bind left-hand side of binary expression.");
    }
    DPL_Bound_Node* rhs = _dplb_bind_node(dpl, node->as.binary.right);
    if (!rhs) {
        DPL_AST_ERROR(dpl, node, "Cannot bind right-hand side of binary expression.");
    }

    DPL_Symbol* function_symbol = dpl_symbols_find_function2_cstr(&dpl->symbols, function_name, lhs->type, rhs->type);
    if (function_symbol) {
        _dplb_check_function_used(dpl, function_symbol);

        DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        bound_node->kind = BOUND_NODE_FUNCTIONCALL;
        bound_node->type = function_symbol->as.function.signature.returns;
        bound_node->as.function_call.function = function_symbol;

        da_array(DPL_Bound_Node*) temp_arguments = 0;
        da_add(temp_arguments, lhs);
        da_add(temp_arguments, rhs);
        _dplb_move_nodelist(dpl, temp_arguments, &bound_node->as.function_call.arguments_count, &bound_node->as.function_call.arguments);
        return bound_node;
    }

    DPL_Token operator_token = node->as.binary.operator;
    DPL_AST_ERROR(dpl, node, "Cannot resolve function \"%s("SV_Fmt", "SV_Fmt")\" for binary operator \""SV_Fmt"\".",
                  function_name, SV_Arg(lhs->type->name), SV_Arg(rhs->type->name), SV_Arg(operator_token.text));
}

DPL_Bound_Node* _dplb_bind_function_call(DPL* dpl, DPL_Ast_Node* node)
{
    DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
    bound_node->kind = BOUND_NODE_FUNCTIONCALL;

    da_array(DPL_Symbol*) argument_types = NULL;
    da_array(DPL_Bound_Node*) temp_arguments = NULL;

    DPL_Ast_FunctionCall fc = node->as.function_call;
    for (size_t i = 0; i < fc.argument_count; ++i) {
        DPL_Bound_Node* bound_argument = _dplb_bind_node(dpl, fc.arguments[i]);
        if (!bound_argument) {
            DPL_AST_ERROR(dpl, fc.arguments[i], "Cannot bind argument #%zu of function call.", i);
        }
        da_add(temp_arguments, bound_argument);
        da_add(argument_types, bound_argument->type);
    }
    _dplb_move_nodelist(dpl, temp_arguments, &bound_node->as.function_call.arguments_count,
                        &bound_node->as.function_call.arguments);

    DPL_Symbol* function_symbol = dpl_symbols_find_function(&dpl->symbols, fc.name.text, da_size(argument_types), argument_types);
    if (function_symbol) {
        _dplb_check_function_used(dpl, function_symbol);
        bound_node->as.function_call.function = function_symbol;
        bound_node->type = function_symbol->as.function.signature.returns;
        return bound_node;
    }

    Nob_String_Builder signature_builder = {0};
    {
        nob_sb_append_sv(&signature_builder, fc.name.text);
        nob_sb_append_cstr(&signature_builder, "(");
        DPL_Bound_Node** arguments = bound_node->as.function_call.arguments;
        size_t arguments_count =  bound_node->as.function_call.arguments_count;
        for (size_t i = 0; i < arguments_count; ++i) {
            if (i > 0) {
                nob_sb_append_cstr(&signature_builder, ", ");
            }
            nob_sb_append_sv(&signature_builder, arguments[i]->type->name);
        }
        nob_sb_append_cstr(&signature_builder, ")");
        nob_sb_append_null(&signature_builder);
    }
    DPL_AST_ERROR(dpl, node, "Cannot resolve function `%s`.", signature_builder.items);
}

DPL_Bound_Node* _dplb_bind_scope(DPL* dpl, DPL_Ast_Node* node)
{
    dpl_symbols_push_boundary_cstr(&dpl->symbols, NULL, BOUNDARY_SCOPE);

    DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
    bound_node->kind = BOUND_NODE_SCOPE;

    DPL_Ast_Scope scope = node->as.scope;
    da_array(DPL_Bound_Node*) temp_expressions = 0;
    for (size_t i = 0; i < scope.expression_count; ++i) {
        DPL_Bound_Node* bound_expression = _dplb_bind_node(dpl, scope.expressions[i]);
        if (!bound_expression) {
            continue;
        }

        da_add(temp_expressions, bound_expression);
        bound_node->type = bound_expression->type;
    }
    _dplb_move_nodelist(dpl, temp_expressions, &bound_node->as.scope.expressions_count, &bound_node->as.scope.expressions);

    dpl_symbols_pop_boundary(&dpl->symbols);
    return bound_node;
}

Nob_String_View _dplb_unescape_string(DPL* dpl, Nob_String_View escaped_string,
                                      size_t prefix_count, size_t postfix_count) {
    // unescape source string literal
    char* unescaped_string = arena_alloc(&dpl->bound_tree.memory, sizeof(char) * (escaped_string.count - (prefix_count + postfix_count) + 1));
    // -2 for quotes; +1 for terminating null byte

    const char *source_pos = escaped_string.data + prefix_count;
    const char *source_end = escaped_string.data + (escaped_string.count - postfix_count - 1);
    char *target_pos = unescaped_string;

    while (source_pos <= source_end) {
        if (*source_pos == '\\') {
            source_pos++;
            switch (*source_pos) {
            case 'n':
                *target_pos = '\n';
                break;
            case 'r':
                *target_pos = '\r';
                break;
            case 't':
                *target_pos = '\t';
                break;
            }
        } else {
            *target_pos = *source_pos;
        }

        source_pos++;
        target_pos++;
    }

    *target_pos = '\0';
    return nob_sv_from_cstr(unescaped_string);
}

DPL_Symbol_Constant _dplb_fold_constant(DPL* dpl, DPL_Ast_Node* node) {
    switch (node->kind) {
    case AST_NODE_LITERAL: {
        DPL_Token value = node->as.literal.value;
        switch (value.kind) {
        case TOKEN_NUMBER:
            return (DPL_Symbol_Constant) {
                .type = dpl_symbols_find_type_number(&dpl->symbols),
                .as.number = atof(nob_temp_sv_to_cstr(value.text)),
            };
        case TOKEN_STRING:
            return (DPL_Symbol_Constant) {
                .type = dpl_symbols_find_type_string(&dpl->symbols),
                .as.string = _dplb_unescape_string(dpl, value.text, 1, 1),
            };
        case TOKEN_STRING_INTERPOLATION:
            return (DPL_Symbol_Constant) {
                .type = dpl_symbols_find_type_string(&dpl->symbols),
                .as.string = _dplb_unescape_string(dpl, value.text, 1, 2),
            };
        case TOKEN_TRUE:
            return (DPL_Symbol_Constant) {
                .type = dpl_symbols_find_type_boolean(&dpl->symbols),
                .as.boolean = true,
            };
        case TOKEN_FALSE:
            return (DPL_Symbol_Constant) {
                .type = dpl_symbols_find_type_boolean(&dpl->symbols),
                .as.boolean = false,
            };
        default:
            DPL_TOKEN_ERROR(dpl, value, "Cannot fold %s.", _dpll_token_kind_name(value.kind));
        }
    }
    break;
    case AST_NODE_BINARY: {
        DPL_Token operator = node->as.binary.operator;
        DPL_Symbol_Constant lhs_value = _dplb_fold_constant(dpl, node->as.binary.left);
        DPL_Symbol_Constant rhs_value = _dplb_fold_constant(dpl, node->as.binary.right);

        switch (operator.kind) {
        case TOKEN_PLUS: {
            if (dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_NUMBER) && dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_NUMBER)) {
                return (DPL_Symbol_Constant) {
                    .type = lhs_value.type,
                    .as.number = lhs_value.as.number + rhs_value.as.number,
                };
            }
            if (dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_STRING) && dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_STRING)) {
                return (DPL_Symbol_Constant) {
                    .type = lhs_value.type,
                    .as.string = nob_sv_from_cstr(nob_temp_sprintf(SV_Fmt SV_Fmt, SV_Arg(lhs_value.as.string), SV_Arg(rhs_value.as.string))),
                };
            }

            DPL_TOKEN_ERROR(dpl, operator, "Cannot fold constant for binary operator %s: Both operands must be either of type `number` or `string`.",
                            _dpll_token_kind_name(operator.kind));

        }
        case TOKEN_MINUS: {
            if (!dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_NUMBER)) {
                DPL_AST_ERROR(dpl, node->as.binary.left, "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                              _dpll_token_kind_name(operator.kind));
            }
            if (!dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_NUMBER)) {
                DPL_AST_ERROR(dpl, node->as.binary.right, "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                              _dpll_token_kind_name(operator.kind));
            }

            return (DPL_Symbol_Constant) {
                .type = lhs_value.type,
                .as.number = lhs_value.as.number - rhs_value.as.number,
            };
        }
        break;
        case TOKEN_STAR: {
            if (!dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_NUMBER)) {
                DPL_AST_ERROR(dpl, node->as.binary.left, "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                              _dpll_token_kind_name(operator.kind));
            }
            if (!dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_NUMBER)) {
                DPL_AST_ERROR(dpl, node->as.binary.right, "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                              _dpll_token_kind_name(operator.kind));
            }

            return (DPL_Symbol_Constant) {
                .type = lhs_value.type,
                .as.number = lhs_value.as.number * rhs_value.as.number,
            };
        }
        break;
        case TOKEN_SLASH: {
            if (!dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_NUMBER)) {
                DPL_AST_ERROR(dpl, node->as.binary.left, "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                              _dpll_token_kind_name(operator.kind));
            }
            if (!dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_NUMBER)) {
                DPL_AST_ERROR(dpl, node->as.binary.right, "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                              _dpll_token_kind_name(operator.kind));
            }

            return (DPL_Symbol_Constant) {
                .type = lhs_value.type,
                .as.number = lhs_value.as.number / rhs_value.as.number,
            };
        }
        break;
        default:
            DPL_TOKEN_ERROR(dpl, operator, "Cannot fold constant for binary operator %s.", _dpll_token_kind_name(operator.kind));
        }
    }
    break;
    case AST_NODE_SYMBOL: {
        Nob_String_View symbol_name = node->as.symbol.text;
        DPL_Symbol* symbol = dpl_symbols_find(&dpl->symbols, symbol_name);
        if (!symbol) {
            DPL_TOKEN_ERROR(dpl, node->as.symbol, "Cannot fold constant: unknown symbol `"SV_Fmt"`.", SV_Arg(symbol_name));
        }

        if (symbol->kind != SYMBOL_CONSTANT) {
            DPL_TOKEN_ERROR(dpl, node->as.symbol, "Cannot fold constant: symbol `"SV_Fmt"` does not resolve to a constant value.", SV_Arg(symbol_name));
        }

        return symbol->as.constant;
    }
    break;
    default:
        DPL_AST_ERROR(dpl, node, "Cannot fold constant expression of type `%s`.\n", _dpla_node_kind_name(node->kind));
    }
}

DPL_Symbol* _dplb_resolve_type_alias(DPL_Symbol* type)
{
    while (type && type->as.type.kind == TYPE_ALIAS) {
        type = type->as.type.as.alias;
    }
    return type;
}

void _dplb_check_assignment(DPL* dpl, const char* what, DPL_Ast_Node* node, DPL_Symbol* expression_type)
{
    DPL_Ast_Declaration *decl = &node->as.declaration;
    if (dpl_symbols_is_type_base(expression_type, TYPE_BASE_NONE)) {
        DPL_AST_ERROR(dpl, decl->initialization,
                      "Expressions of type `"SV_Fmt"` cannot be assigned.", SV_Arg(expression_type->name));
    }

    if (decl->type) {
        DPL_Symbol* declared_type = _dplb_bind_type(dpl, decl->type);
        DPL_Symbol* resolved_declared_type = _dplb_resolve_type_alias(declared_type);
        if (!resolved_declared_type) {
            DPL_AST_ERROR(dpl, decl->type, "Unknown type `%s` in declaration of %s `"SV_Fmt"`.",
                          _dpla_type_name(decl->type), what, SV_Arg(decl->name.text));
        }

        DPL_Symbol* resolved_expression_type = _dplb_resolve_type_alias(expression_type);
        if (resolved_expression_type != resolved_declared_type) {
            DPL_AST_ERROR(dpl, node, "Cannot assign expression of type `"SV_Fmt"` to %s `"SV_Fmt"` of type `"SV_Fmt"`.",
                          SV_Arg(expression_type->name), what, SV_Arg(decl->name.text), SV_Arg(declared_type->name));
        }
    }
}

int _dplb_bind_object_literal_compare_fields(void const* a, void const* b)
{
    return nob_sv_cmp(((DPL_Bound_ObjectField*) a)->name, ((DPL_Bound_ObjectField*) b)->name);
}

int _dplb_bind_object_literal_compare_query(void const* a, void const* b)
{
    return nob_sv_cmp(((DPL_Symbol_Type_ObjectField*) a)->name, ((DPL_Symbol_Type_ObjectField*) b)->name);
}

void _dplb_bind_object_literal_add_field(DPL* dpl, da_array(DPL_Bound_ObjectField)* tmp_bound_fields,
        DPL_Symbol_Type_ObjectQuery* type_query, Nob_String_View field_name, DPL_Bound_Node* field_expression) {
    DW_UNUSED(dpl);

    for (size_t i = 0; i < da_size(*tmp_bound_fields); ++i) {
        if (nob_sv_eq((*tmp_bound_fields)[i].name, field_name)) {
            (*tmp_bound_fields)[i].expression = field_expression;
            return;
        }
    }

    DPL_Bound_ObjectField bound_field = {
        .name = field_name,
        .expression = field_expression,
    };
    da_add(*tmp_bound_fields, bound_field);
    qsort(*tmp_bound_fields, da_size(*tmp_bound_fields), sizeof(**tmp_bound_fields), _dplb_bind_object_literal_compare_fields);

    DPL_Symbol_Type_ObjectField query_field = {
        .name = bound_field.name,
        .type = bound_field.expression->type,
    };
    da_add(*type_query, query_field);
    qsort(*type_query, da_size(*type_query), sizeof(**type_query), _dplb_bind_object_literal_compare_query);
}

DPL_Bound_Node* _dplb_bind_object_literal(DPL* dpl, DPL_Ast_Node* node) {
    dpl_symbols_push_boundary_cstr(&dpl->symbols, NULL, BOUNDARY_SCOPE);

    DPL_Ast_ObjectLiteral object_literal = node->as.object_literal;
    da_array(DPL_Bound_ObjectField) tmp_bound_fields = NULL;
    DPL_Symbol_Type_ObjectQuery type_query = NULL;
    da_array(DPL_Bound_Node*) temporaries = NULL;
    for (size_t i = 0; i < object_literal.field_count; ++i) {
        DPL_Ast_Node* field = object_literal.fields[i];
        if (field->kind == AST_NODE_ASSIGNMENT
                && field->as.assignment.target->kind == AST_NODE_SYMBOL) {
            DPL_Ast_Assignment* assignment = &object_literal.fields[i]->as.assignment;
            _dplb_bind_object_literal_add_field(
                dpl, &tmp_bound_fields, &type_query,
                assignment->target->as.symbol.text,
                _dplb_bind_node(dpl, assignment->expression)
            );
        } else if (field->kind == AST_NODE_UNARY
                   && field->as.unary.operator.kind == TOKEN_DOT_DOT) {
            DPL_Bound_Node* bound_temporary = _dplb_bind_node(dpl, field->as.unary.operand);
            bound_temporary->persistent = true;
            if (bound_temporary->type->as.type.kind != TYPE_OBJECT) {
                DPL_AST_ERROR(dpl, field, "Only object expressions can be spread for composing objects.");
            }
            da_add(temporaries, bound_temporary);

            // DPL_Scope* current_scope = _dplb_scopes_current(dpl);
            // size_t scope_index = current_scope->offset + current_scope->count;
            // current_scope->count++;

            DPL_Symbol* x = dpl_symbols_push_cstr(&dpl->symbols, SYMBOL_VAR, NULL);

            DPL_Symbol_Type_Object bound_object_type = bound_temporary->type->as.type.as.object;
            for (size_t i = 0; i < bound_object_type.field_count; ++i) {
                DPL_Bound_Node* var_ref = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
                var_ref->kind = BOUND_NODE_VARREF;
                var_ref->type = bound_temporary->type;
                var_ref->as.varref = x->stack_index;

                DPL_Bound_Node* load_field = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
                load_field->kind = BOUND_NODE_LOAD_FIELD;
                load_field->type = bound_object_type.fields[i].type;
                load_field->as.load_field.expression = var_ref;
                load_field->as.load_field.field_index = i;

                _dplb_bind_object_literal_add_field(
                    dpl, &tmp_bound_fields, &type_query,
                    bound_object_type.fields[i].name,
                    load_field
                );
            }
        } else if (field->kind == AST_NODE_SYMBOL) {
            _dplb_bind_object_literal_add_field(
                dpl, &tmp_bound_fields, &type_query,
                field->as.symbol.text,
                _dplb_bind_node(dpl, field)
            );
        } else {
            DPL_AST_ERROR(dpl, field, "Cannot use a %s in an object expression.",
                          _dpla_node_kind_name(field->kind));
        }
    }

    DPL_Symbol* bound_type = dpl_symbols_find_type_object_query(&dpl->symbols, type_query);
    if (!bound_type) {
        Nob_String_Builder type_name_builder = {0};
        nob_sb_append_cstr(&type_name_builder, "[");
        for (size_t i = 0; i < da_size(type_query); ++i) {
            if (i > 0) {
                nob_sb_append_cstr(&type_name_builder, ", ");
            }
            nob_sb_append_sv(&type_name_builder, type_query[i].name);
            nob_sb_append_cstr(&type_name_builder, ": ");
            nob_sb_append_sv(&type_name_builder, type_query[i].type->name);
        }
        nob_sb_append_cstr(&type_name_builder, "]");
        nob_sb_append_null(&type_name_builder);

        bound_type = dpl_symbols_push_type_object_cstr(&dpl->symbols, type_name_builder.items, da_size(type_query));
        memcpy(bound_type->as.type.as.object.fields, type_query, sizeof(DPL_Symbol_Type_ObjectField) * da_size(type_query));

        nob_sb_free(type_name_builder);
    }
    da_free(type_query);

    DPL_Bound_ObjectField* bound_fields = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_ObjectField) * da_size(tmp_bound_fields));
    memcpy(bound_fields, tmp_bound_fields, sizeof(DPL_Bound_ObjectField) * da_size(tmp_bound_fields));

    DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
    bound_node->kind = BOUND_NODE_OBJECT;
    bound_node->type = bound_type;
    bound_node->as.object.field_count = da_size(tmp_bound_fields);
    bound_node->as.object.fields = bound_fields;

    da_free(tmp_bound_fields);

    dpl_symbols_pop_boundary(&dpl->symbols);

    if (da_some(temporaries)) {
        DPL_Bound_Node* bound_scope = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        bound_scope->kind = BOUND_NODE_SCOPE;
        bound_scope->type = bound_node->type;

        da_add(temporaries, bound_node);
        _dplb_move_nodelist(dpl, temporaries, &bound_scope->as.scope.expressions_count,
                            &bound_scope->as.scope.expressions);

        return bound_scope;
    }

    return bound_node;
}

DPL_Bound_Node* _dplb_bind_node(DPL* dpl, DPL_Ast_Node* node)
{
    switch (node->kind)
    {
    case AST_NODE_LITERAL: {
        switch (node->as.literal.value.kind)
        {
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_STRING_INTERPOLATION:
        case TOKEN_TRUE:
        case TOKEN_FALSE: {
            DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            bound_node->kind = BOUND_NODE_VALUE;
            bound_node->as.value = _dplb_fold_constant(dpl, node);
            bound_node->type = bound_node->as.value.type;
            return bound_node;
        }
        default:
            DPL_AST_ERROR(dpl, node, "Cannot resolve literal type for %s.",
                          _dpll_token_kind_name(node->as.literal.value.kind));
        }
    }
    case AST_NODE_OBJECT_LITERAL:
        return _dplb_bind_object_literal(dpl, node);
    case AST_NODE_FIELD_ACCESS: {
        DPL_Bound_Node* bound_expression = _dplb_bind_node(dpl, node->as.field_access.expression);

        DPL_Symbol* expression_type = bound_expression->type;
        if (expression_type->as.type.kind == TYPE_ALIAS) {
            expression_type = expression_type->as.type.as.alias;
        }

        if (expression_type->as.type.kind != TYPE_OBJECT) {
            DPL_AST_ERROR(dpl, node->as.field_access.expression, "Can access fields only for object types." SV_Fmt, SV_Arg(bound_expression->type->name));
        }

        DPL_Token field_name = node->as.field_access.field->as.symbol;
        DPL_Symbol_Type_Object object_type = expression_type->as.type.as.object;

        bool field_found = false;
        size_t field_index = 0;
        for (size_t i = 0; i < object_type.field_count; ++i) {
            if (nob_sv_eq(object_type.fields[i].name, field_name.text)) {
                field_found = true;
                field_index = i;
                break;
            }
        }

        if (!field_found) {
            DPL_AST_ERROR(dpl, node, "Objects of type `"SV_Fmt"`, have no field `"SV_Fmt"`.",
                          SV_Arg(bound_expression->type->name), SV_Arg(field_name.text));
        }

        DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        bound_node->kind = BOUND_NODE_LOAD_FIELD;
        bound_node->type = object_type.fields[field_index].type;
        bound_node->as.load_field = (DPL_Bound_LoadField) {
            .expression = bound_expression,
            .field_index = field_index,
        };

        return bound_node;
    }
    break;
    case AST_NODE_UNARY: {
        DPL_Token operator = node->as.unary.operator;
        switch(operator.kind) {
        case TOKEN_MINUS:
            return _dplb_bind_unary(dpl, node, "negate");
        case TOKEN_BANG:
            return _dplb_bind_unary(dpl, node, "not");
        default:
            break;
        }

        DPL_AST_ERROR(dpl, node, "Cannot resolve function for unary operator \""SV_Fmt"\".",
                      SV_Arg(operator.text));
    }
    case AST_NODE_BINARY: {
        DPL_Token operator = node->as.binary.operator;
        switch(operator.kind) {
        case TOKEN_PLUS:
            return _dplb_bind_binary(dpl, node, "add");
        case TOKEN_MINUS:
            return _dplb_bind_binary(dpl, node, "subtract");
        case TOKEN_STAR:
            return _dplb_bind_binary(dpl, node, "multiply");
        case TOKEN_SLASH:
            return _dplb_bind_binary(dpl, node, "divide");
        case TOKEN_LESS:
            return _dplb_bind_binary(dpl, node, "less");
        case TOKEN_LESS_EQUAL:
            return _dplb_bind_binary(dpl, node, "lessEqual");
        case TOKEN_GREATER:
            return _dplb_bind_binary(dpl, node, "greater");
        case TOKEN_GREATER_EQUAL:
            return _dplb_bind_binary(dpl, node, "greaterEqual");
        case TOKEN_EQUAL_EQUAL:
            return _dplb_bind_binary(dpl, node, "equal");
        case TOKEN_BANG_EQUAL:
            return _dplb_bind_binary(dpl, node, "notEqual");
        case TOKEN_AND_AND:
        case TOKEN_PIPE_PIPE: {
            DPL_Bound_Node* lhs = _dplb_bind_node(dpl, node->as.binary.left);
            if (!lhs) {
                DPL_AST_ERROR(dpl, node, "Cannot bind left-hand side of binary expression.");
            }
            DPL_Bound_Node* rhs = _dplb_bind_node(dpl, node->as.binary.right);
            if (!rhs) {
                DPL_AST_ERROR(dpl, node, "Cannot bind right-hand side of binary expression.");
            }

            DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            bound_node->kind = BOUND_NODE_LOGICAL_OPERATOR;
            bound_node->type = dpl_symbols_find_type_boolean(&dpl->symbols);
            bound_node->as.logical_operator.operator = node->as.binary.operator;
            bound_node->as.logical_operator.lhs = lhs;
            bound_node->as.logical_operator.rhs = rhs;
            return bound_node;
        }
        default:
            break;
        }

        DPL_AST_ERROR(dpl, node, "Cannot resolve function for binary operator %s.",
                      _dpll_token_kind_name(operator.kind));
    }
    break;
    case AST_NODE_FUNCTIONCALL:
        return _dplb_bind_function_call(dpl, node);
    case AST_NODE_SCOPE:
        return _dplb_bind_scope(dpl, node);
    case AST_NODE_DECLARATION: {
        DPL_Ast_Declaration* decl = &node->as.declaration;

        if (decl->keyword.kind == TOKEN_KEYWORD_CONSTANT) {
            DPL_Symbol* s = dpl_symbols_push(&dpl->symbols, SYMBOL_CONSTANT, decl->name.text);
            s->as.constant = _dplb_fold_constant(dpl, decl->initialization);
            _dplb_check_assignment(dpl, "constant", node, s->as.constant.type);

            return NULL;
        } else if (decl->keyword.kind == TOKEN_KEYWORD_VAR) {
            DPL_Bound_Node* expression = _dplb_bind_node(dpl, decl->initialization);
            expression->persistent = true;
            _dplb_check_assignment(dpl, "variable", node, expression->type);

            dpl_symbols_push_var(&dpl->symbols, decl->name.text, expression->type);

            return expression;
        } else if (decl->keyword.kind == TOKEN_KEYWORD_TYPE) {
            dpl_symbols_push_type_alias(&dpl->symbols, decl->name.text, _dplb_bind_type(dpl, decl->type));
            return NULL;
        } else {
            DW_UNIMPLEMENTED_MSG("Cannot bind declaration with `%s`.",
                                 _dpll_token_kind_name(decl->keyword.kind));
        }
    }
    break;
    case AST_NODE_SYMBOL: {
        DPL_Symbol* symbol = dpl_symbols_find(&dpl->symbols, node->as.symbol.text);
        if (!symbol) {
            DPL_AST_ERROR(dpl, node, "Cannot resolve symbol `"SV_Fmt"` in current scope.",
                          SV_Arg(node->as.symbol.text));
        }

        switch (symbol->kind) {
        case SYMBOL_CONSTANT: {
            DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            node->kind = BOUND_NODE_VALUE;
            node->type = symbol->as.constant.type;
            node->as.value = symbol->as.constant;
            return node;
        }
        break;
        case SYMBOL_VAR: {
            DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            node->kind = BOUND_NODE_VARREF;
            node->type = symbol->as.var.type;
            node->as.varref = symbol->as.var.scope_index;
            return node;
        }
        break;
        case SYMBOL_ARGUMENT: {
            DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            node->kind = BOUND_NODE_ARGREF;
            node->type = symbol->as.argument.type;
            node->as.argref = symbol->as.argument.scope_index;
            return node;
        }
        break;
        default:
            DPL_AST_ERROR(dpl, node, "Cannot resolve symbols of type `%s`.",
                          dpl_symbols_kind_name(symbol->kind));

        }
    }
    case AST_NODE_ASSIGNMENT: {
        if (node->as.assignment.target->kind != AST_NODE_SYMBOL) {
            DPL_AST_ERROR(dpl, node, "Cannot assign to target of type `%s`.",
                          _dpla_node_kind_name(node->as.assignment.target->kind));
        }

        Nob_String_View symbol_name = node->as.assignment.target->as.symbol.text;
        DPL_Symbol* symbol = dpl_symbols_find(&dpl->symbols, symbol_name);
        if (!symbol) {
            DPL_AST_ERROR(dpl, node->as.assignment.target, "Cannot find symbol `"SV_Fmt"`.",
                          SV_Arg(symbol_name));
        }
        if (symbol->kind != SYMBOL_VAR) {
            DPL_AST_ERROR(dpl, node->as.assignment.target, "Cannot assign to %s `"SV_Fmt"`.",
                          dpl_symbols_kind_name(symbol->kind), SV_Arg(symbol_name));
        }

        DPL_Bound_Node* bound_expression = _dplb_bind_node(dpl, node->as.assignment.expression);

        if (symbol->as.var.type != bound_expression->type) {
            DPL_AST_ERROR(dpl, node, "Cannot assign expression of type `"SV_Fmt"` to variable `"SV_Fmt"` of type `"SV_Fmt"`.",
                          SV_Arg(bound_expression->type->name), SV_Arg(symbol->name), SV_Arg(symbol->as.var.type->name));
        }

        DPL_Bound_Node* ct_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        ct_node->kind = BOUND_NODE_ASSIGNMENT;
        ct_node->type = symbol->as.var.type;
        ct_node->as.assignment.scope_index = symbol->as.var.scope_index;
        ct_node->as.assignment.expression = bound_expression;

        return ct_node;
    }
    case AST_NODE_FUNCTION: {
        DPL_Ast_Function* function = &node->as.function;

        DPL_Symbol* function_symbol = dpl_symbols_push_function_user(&dpl->symbols, function->name.text, function->signature.argument_count);

        for (size_t i = 0; i < function->signature.argument_count; ++i) {
            DPL_Ast_FunctionArgument arg = function->signature.arguments[i];
            DPL_Symbol* arg_type = _dplb_resolve_type_alias(_dplb_bind_type(dpl, arg.type));
            if (!arg_type) {
                DPL_AST_ERROR(dpl, arg.type,
                              "Cannot resolve type `%s` for argument `"SV_Fmt"` in current scope.",
                              _dpla_type_name(arg.type), SV_Arg(arg.name.text));
            }

            function_symbol->as.function.signature.arguments[i] = arg_type;
        }

        dpl_symbols_push_boundary_cstr(&dpl->symbols, NULL, BOUNDARY_FUNCTION);

        for (size_t i = 0; i < function->signature.argument_count; ++i) {
            dpl_symbols_push_argument(&dpl->symbols, function->signature.arguments[i].name.text, 
                function_symbol->as.function.signature.arguments[i]);
        }

        DPL_Bound_Node *bound_body = _dplb_bind_node(dpl, function->body);

        if (function->signature.type) {
            DPL_Symbol* return_type = _dplb_bind_type(dpl, function->signature.type);
            if (!return_type) {
                DPL_AST_ERROR(dpl, function->signature.type,
                              "Cannot resolve return type `%s` in current scope.",
                              _dpla_type_name(function->signature.type));
            }

            if (return_type != bound_body->type) {
                DPL_AST_ERROR(dpl, node,
                              "Declared return type `%s` of function `"SV_Fmt"` is not compatible with body expression type `"SV_Fmt"`.",
                              _dpla_type_name(function->signature.type),
                              SV_Arg(function->name.text),
                              SV_Arg(bound_body->type->name));
            }

            function_symbol->as.function.signature.returns = return_type;
        } else {
            function_symbol->as.function.signature.returns = bound_body->type;
        }

        function_symbol->as.function.as.user_function.body = bound_body;

        dpl_symbols_pop_boundary(&dpl->symbols);
        return NULL;
    }
    case AST_NODE_CONDITIONAL: {
        DPL_Ast_Conditional conditional =  node->as.conditional;

        DPL_Bound_Node* bound_condition = _dplb_bind_node(dpl, conditional.condition);
        if (!dpl_symbols_is_type_base(bound_condition->type, TYPE_BASE_BOOLEAN)) {
            DPL_Symbol* boolean_type = dpl_symbols_find_type_boolean(&dpl->symbols);

            DPL_AST_ERROR(dpl, conditional.condition,
                          "Condition operand type `"SV_Fmt"` does not match type `"SV_Fmt"`.",
                          SV_Arg(bound_condition->type->name), SV_Arg(boolean_type->name));
        }

        DPL_Bound_Node* bound_then_clause = _dplb_bind_node(dpl, conditional.then_clause);
        DPL_Bound_Node* bound_else_clause = _dplb_bind_node(dpl, conditional.else_clause);
        if (bound_then_clause->type != bound_else_clause->type) {
            DPL_AST_ERROR(dpl, node, "Types `"SV_Fmt"` and `"SV_Fmt"` do not match in the conditional expression clauses.",
                          SV_Arg(bound_then_clause->type->name), SV_Arg(bound_else_clause->type->name));
        }

        DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        node->kind = BOUND_NODE_CONDITIONAL;
        node->type = bound_then_clause->type;
        node->as.conditional.condition = bound_condition;
        node->as.conditional.then_clause = bound_then_clause;
        node->as.conditional.else_clause = bound_else_clause;
        return node;
    }
    break;
    case AST_NODE_WHILE_LOOP: {
        DPL_Ast_WhileLoop while_loop =  node->as.while_loop;

        DPL_Bound_Node* bound_condition = _dplb_bind_node(dpl, while_loop.condition);
        if (!dpl_symbols_is_type_base(bound_condition->type, TYPE_BASE_BOOLEAN)) {
            DPL_Symbol* boolean_type = dpl_symbols_find_type_boolean(&dpl->symbols);

            DPL_AST_ERROR(dpl, while_loop.condition,
                          "Condition operand type `"SV_Fmt"` does not match type `"SV_Fmt"`.",
                          SV_Arg(bound_condition->type->name), SV_Arg(boolean_type->name));
        }

        DPL_Bound_Node* bound_body = _dplb_bind_node(dpl, while_loop.body);

        // TODO: At the moment, loops do not produce values and therefore are of type `none`.
        //       This should change in the future, where they can yield optional values or
        //       arrays.
        DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        node->kind = BOUND_NODE_WHILE_LOOP;
        node->type = dpl_symbols_find_kind_cstr(&dpl->symbols, TYPENAME_NONE, SYMBOL_TYPE);
        node->as.while_loop.condition = bound_condition;
        node->as.while_loop.body = bound_body;
        return node;
    }
    break;
    case AST_NODE_INTERPOLATION: {
        da_array(DPL_Bound_Node*) bound_expressions = NULL;
        for (size_t i = 0; i < node->as.interpolation.expression_count; ++i) {
            DPL_Bound_Node* bound_expression = _dplb_bind_node(dpl, node->as.interpolation.expressions[i]);

            if (!dpl_symbols_is_type_base(bound_expression->type, TYPE_BASE_STRING)) {
                DPL_Bound_Node* bound_expression_to_string = _dplb_bind_unary_function_call(dpl, bound_expression, "toString");
                if (!bound_expression_to_string) {
                    DPL_AST_ERROR(dpl, node, "Cannot resolve function `toString("SV_Fmt")` for string interpolation.",
                                  SV_Arg(bound_expression->type->name));
                }

                bound_expression = bound_expression_to_string;
            }

            da_add(bound_expressions, bound_expression);
        }

        DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        node->kind = BOUND_NODE_INTERPOLATION;
        node->type = dpl_symbols_find_type_string(&dpl->symbols);
        _dplb_move_nodelist(dpl, bound_expressions, &node->as.interpolation.expressions_count,
                            &node->as.interpolation.expressions);
        return node;
    }
    break;
    default:
        DPL_AST_ERROR(dpl, node, "Cannot bind %s expressions.",
                      _dpla_node_kind_name(node->kind));
    }

}

void _dplb_bind(DPL* dpl)
{
    dpl->bound_tree.root = _dplb_bind_node(dpl, dpl->tree.root);
}

void _dplb_print(DPL* dpl, DPL_Bound_Node* node, size_t level) {
    for (size_t i = 0; i < level; ++i) {
        printf("  ");
    }

    if (!node) {
        printf("<nil>\n");
        return;
    }

    if (node->persistent) {
        printf("*");
    }

    if (node->type) {
        printf("["SV_Fmt"] ", SV_Arg(node->type->name));
    } else {
        printf("[<unknown>] ");
    }

    switch(node->kind) {
    case BOUND_NODE_FUNCTIONCALL: {
        DPL_Symbol* function = node->as.function_call.function;
        printf(SV_Fmt"(\n", SV_Arg(function->name));

        for (size_t i = 0; i < node->as.function_call.arguments_count; ++i) {
            _dplb_print(dpl, node->as.function_call.arguments[i], level + 1);
        }

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");

    }
    break;
    case BOUND_NODE_VALUE: {
        printf("Value `");
        if (dpl_symbols_is_type_base(node->as.value.type, TYPE_BASE_NUMBER)) {
            printf("%f", node->as.value.as.number);
        } else if (dpl_symbols_is_type_base(node->as.value.type, TYPE_BASE_STRING)) {
            Nob_String_View value = node->as.value.as.string;
            dplp_print_escaped_string(value.data, value.count);
        } else if (dpl_symbols_is_type_base(node->as.value.type, TYPE_BASE_BOOLEAN)) {
            printf("%s", node->as.value.as.boolean ? "true" : "false");
        } else {
            printf("<unknown>");
        }
        printf("`\n");
    }
    break;
    case BOUND_NODE_OBJECT: {
        printf("$object(\n");

        DPL_Bound_Object object = node->as.object;
        for (size_t field_index = 0; field_index < object.field_count; ++field_index) {
            for (size_t i = 0; i < level + 1; ++i) {
                printf("  ");
            }
            printf(SV_Fmt":\n", SV_Arg(object.fields[field_index].name));
            _dplb_print(dpl, object.fields[field_index].expression, level + 2);
        }

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }

        printf(")\n");
    }
    break;
    case BOUND_NODE_SCOPE: {
        printf("$scope(\n");

        for (size_t i = 0; i < node->as.scope.expressions_count; ++i) {
            _dplb_print(dpl, node->as.scope.expressions[i], level + 1);
        }

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_VARREF: {
        printf("$varref(scope_index = %zu)\n", node->as.varref);
    }
    break;
    case BOUND_NODE_ARGREF: {
        printf("$argref(scope_index = %zu)\n", node->as.argref);
    }
    break;
    case BOUND_NODE_ASSIGNMENT: {
        printf("$assignment(\n");

        for (size_t i = 0; i < level + 1; ++i) {
            printf("  ");
        }
        printf("scope_index %zu\n", node->as.assignment.scope_index);

        _dplb_print(dpl, node->as.assignment.expression, level + 1);

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_CONDITIONAL: {
        printf("$conditional(\n");
        _dplb_print(dpl, node->as.conditional.condition, level + 1);
        _dplb_print(dpl, node->as.conditional.then_clause, level + 1);
        _dplb_print(dpl, node->as.conditional.else_clause, level + 1);

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_LOGICAL_OPERATOR: {
        printf("$logical_operator(\n");
        for (size_t i = 0; i < level + 1; ++i) {
            printf("  ");
        }
        printf("%s\n", _dpll_token_kind_name(node->as.logical_operator.operator.kind));
        _dplb_print(dpl, node->as.logical_operator.lhs, level + 1);
        _dplb_print(dpl, node->as.logical_operator.rhs, level + 1);

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_WHILE_LOOP: {
        printf("$while(\n");
        _dplb_print(dpl, node->as.while_loop.condition, level + 1);
        _dplb_print(dpl, node->as.while_loop.body, level + 1);

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_LOAD_FIELD: {
        printf("$load_field( #%zu\n", node->as.load_field.field_index);
        _dplb_print(dpl, node->as.load_field.expression, level + 1);

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_INTERPOLATION: {
        printf("$interpolation(\n");

        for (size_t i = 0; i < node->as.interpolation.expressions_count; ++i) {
            _dplb_print(dpl, node->as.interpolation.expressions[i], level + 1);
        }

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    default:
        DW_UNIMPLEMENTED_MSG("%s", _dplb_nodekind_name(node->kind));
    }
}

// PROGRAM GENERATION

void _dplg_generate(DPL* dpl, DPL_Bound_Node* node, DPL_Program* program) {
    switch (node->kind)
    {
    case BOUND_NODE_VALUE: {
        if (dpl_symbols_is_type_base(node->type, TYPE_BASE_NUMBER)) {
            dplp_write_push_number(program, node->as.value.as.number);
        } else if (dpl_symbols_is_type_base(node->type, TYPE_BASE_STRING)) {
            dplp_write_push_string(program, node->as.value.as.string.data);
        } else if (dpl_symbols_is_type_base(node->type, TYPE_BASE_BOOLEAN)) {
            dplp_write_push_boolean(program, node->as.value.as.boolean);
        } else {
            DPL_ERROR("Cannot generate program for value node of type "SV_Fmt".",
                        SV_Arg(node->type->name));
        }
    }
    break;
    case BOUND_NODE_OBJECT: {
        DPL_Bound_Object object = node->as.object;
        for (size_t i = 0; i < object.field_count; ++i) {
            _dplg_generate(dpl, object.fields[i].expression, program);
        }

        dplp_write_create_object(program, object.field_count);
    }
    break;
    case BOUND_NODE_LOAD_FIELD: {
        _dplg_generate(dpl, node->as.load_field.expression, program);
        dplp_write_load_field(program, node->as.load_field.field_index);
    }
    break;
    case BOUND_NODE_FUNCTIONCALL: {
        DPL_Bound_FunctionCall f = node->as.function_call;
        for (size_t i = 0; i < f.arguments_count; ++i) {
            _dplg_generate(dpl, f.arguments[i], program);
        }

        switch (f.function->as.function.kind) {
        case FUNCTION_INSTRUCTION:
            dplp_write(program, f.function->as.function.as.instruction_function);
            break;
        case FUNCTION_EXTERNAL:
            dplp_write_call_external(program, f.function->as.function.as.external_function);
            break;
        case FUNCTION_USER: {
            DPL_UserFunction* uf = &dpl->user_functions[f.function->as.function.as.user_function.user_handle];
            dplp_write_call_user(program, uf->arity, uf->begin_ip);
        }
        break;
        default:
            DW_UNIMPLEMENTED_MSG("Function kind %d", f.function->as.function.kind);
        }
    }
    break;
    case BOUND_NODE_SCOPE: {
        DPL_Bound_Scope s = node->as.scope;
        bool prev_was_persistent = false;
        size_t persistent_count = 0;
        for (size_t i = 0; i < s.expressions_count; ++i) {
            if (i > 0) {
                if (!prev_was_persistent) {

                    dplp_write_pop(program);
                } else {
                    persistent_count++;
                }
            }
            _dplg_generate(dpl, s.expressions[i], program);
            prev_was_persistent = s.expressions[i]->persistent;
        }

        if (persistent_count > 0) {
            dplp_write_pop_scope(program, persistent_count);
        }
    }
    break;
    case BOUND_NODE_ARGREF:
    case BOUND_NODE_VARREF: {
        dplp_write_push_local(program, node->as.varref);
    }
    break;
    case BOUND_NODE_ASSIGNMENT: {
        _dplg_generate(dpl, node->as.assignment.expression, program);
        dplp_write_store_local(program, node->as.assignment.scope_index);
    }
    break;
    case BOUND_NODE_CONDITIONAL: {
        _dplg_generate(dpl, node->as.conditional.condition, program);

        // jump over then clause if condition is false
        size_t then_jump = dplp_write_jump(program, INST_JUMP_IF_FALSE);

        dplp_write_pop(program);
        _dplg_generate(dpl, node->as.conditional.then_clause, program);

        // jump over else clause if condition is true
        size_t else_jump = dplp_write_jump(program, INST_JUMP);
        dplp_patch_jump(program, then_jump);

        // else clause
        dplp_write_pop(program);
        _dplg_generate(dpl, node->as.conditional.else_clause, program);

        dplp_patch_jump(program, else_jump);
    }
    break;
    case BOUND_NODE_LOGICAL_OPERATOR: {
        _dplg_generate(dpl, node->as.logical_operator.lhs, program);

        size_t jump;
        if (node->as.logical_operator.operator.kind == TOKEN_AND_AND) {
            jump = dplp_write_jump(program, INST_JUMP_IF_FALSE);
        } else {
            jump = dplp_write_jump(program, INST_JUMP_IF_TRUE);
        }

        _dplg_generate(dpl, node->as.logical_operator.rhs, program);

        dplp_patch_jump(program, jump);
    }
    break;
    case BOUND_NODE_WHILE_LOOP: {
        size_t loop_start = da_size(program->code);

        _dplg_generate(dpl, node->as.while_loop.condition, program);

        // jump over loop if condition is false
        size_t exit_jump = dplp_write_jump(program, INST_JUMP_IF_FALSE);

        dplp_write_pop(program);
        _dplg_generate(dpl, node->as.while_loop.body, program);

        // TODO: Once while loops can generate values, this pop should do something else
        dplp_write_pop(program);

        // jump over else clause if condition is true
        dplp_write_loop(program, loop_start);
        dplp_patch_jump(program, exit_jump);
    }
    break;
    case BOUND_NODE_INTERPOLATION: {
        size_t count = node->as.interpolation.expressions_count;
        for (size_t i = 0; i < count; ++i) {
            _dplg_generate(dpl, node->as.interpolation.expressions[i], program);
        }

        dplp_write_interpolation(program, count);
    }
    break;
    default:
        DW_UNIMPLEMENTED_MSG("`%s`", _dplb_nodekind_name(node->kind));
    }
}

// COMPILATION PROCESS

void dpl_compile(DPL *dpl, DPL_Program* program)
{
    _dplp_parse(dpl);
    if (dpl->debug)
    {
        _dpla_print(dpl->tree.root, 0);
        printf("\n");
    }

    _dplb_bind(dpl);
    if (dpl->debug)
    {
        for (size_t i = 0; i < da_size(dpl->user_functions); ++i) {
            DPL_UserFunction* uf = &dpl->user_functions[i];
            printf("### "SV_Fmt" (arity: %zu) ###\n", SV_Arg(uf->function->name), uf->arity);
            _dplb_print(dpl, uf->body, 0);
            printf("\n");
        }

        printf("### program ###\n");
        _dplb_print(dpl, dpl->bound_tree.root, 0);
        printf("\n");
    }

    for (size_t i = 0; i < da_size(dpl->user_functions); ++i) {
        DPL_UserFunction* uf = &dpl->user_functions[i];
        uf->begin_ip = da_size(program->code);
        _dplg_generate(dpl, uf->body, program);
        dplp_write_return(program);
    }

    program->entry = da_size(program->code);
    _dplg_generate(dpl, dpl->bound_tree.root, program);
    if (dpl->debug)
    {
        dplp_print(program);
    }


    if (dpl->debug)
    {
        printf("Final symbol table:\n");
        dpl_symbols_print_table(&dpl->symbols);
        printf("\n");
    }
}