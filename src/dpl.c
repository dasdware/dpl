#ifdef DPL_LEAKCHECK
#include "stb_leakcheck.h"
#endif

#include <dpl/utils.h>
#include <dpl/generator.h>

#include "dpl.h"
#include "error.h"

#define DPL_ERROR DW_ERROR

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
    dpl->lexer.current_line = dpl->lexer.source.data;
}

void dpl_free(DPL *dpl)
{
    // symbol stack freeing
    dpl_symbols_free(&dpl->symbols);

    // parser freeing
    arena_free(&dpl->tree.memory);

    // bound tree freeing
    arena_free(&dpl->bound_tree.memory);
}

// AST

DPL_Ast_Node *_dpla_create_node(DPL_Ast_Tree *tree, DPL_AstNodeKind kind, DPL_Token first, DPL_Token last)
{
    DPL_Ast_Node *node = arena_alloc(&tree->memory, sizeof(DPL_Ast_Node));
    node->kind = kind;
    node->first = first;
    node->last = last;
    return node;
}

DPL_Ast_Node *_dpla_create_literal(DPL_Ast_Tree *tree, DPL_Token token)
{
    DPL_Ast_Node *node = _dpla_create_node(tree, AST_NODE_LITERAL, token, token);
    node->as.literal.value = token;
    return node;
}

const char *_dpla_node_kind_name(DPL_AstNodeKind kind)
{
    switch (kind)
    {
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

void _dpla_build_type_name(DPL_Ast_Type *ast_type, Nob_String_Builder *sb)
{
    switch (ast_type->kind)
    {
    case TYPE_BASE:
        nob_sb_append_sv(sb, ast_type->as.name.text);
        break;
    case TYPE_OBJECT:
    {
        nob_sb_append_cstr(sb, "[");
        for (size_t i = 0; i < ast_type->as.object.field_count; ++i)
        {
            if (i > 0)
            {
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

const char *_dpla_type_name(DPL_Ast_Type *ast_type)
{
    static char result[256];
    if (!ast_type)
    {
        return dpl_lexer_token_kind_name(TOKEN_NONE);
    }
    Nob_String_Builder sb = {0};
    _dpla_build_type_name(ast_type, &sb);
    nob_sb_append_null(&sb);

    strncpy(result, sb.items, NOB_ARRAY_LEN(result));
    return result;
}

void _dpla_print_indent(size_t level)
{
    for (size_t i = 0; i < level; ++i)
    {
        printf("  ");
    }
}

void _dpla_print(DPL_Ast_Node *node, size_t level)
{
    _dpla_print_indent(level);

    if (!node)
    {
        printf("<nil>\n");
        return;
    }

    printf("%s", _dpla_node_kind_name(node->kind));

    switch (node->kind)
    {
    case AST_NODE_UNARY:
    {
        printf(" [%s]\n", dpl_lexer_token_kind_name(node->as.unary.operator.kind));
        _dpla_print(node->as.unary.operand, level + 1);
        break;
    }
    case AST_NODE_BINARY:
    {
        printf(" [%s]\n", dpl_lexer_token_kind_name(node->as.binary.operator.kind));
        _dpla_print(node->as.binary.left, level + 1);
        _dpla_print(node->as.binary.right, level + 1);
        break;
    }
    case AST_NODE_LITERAL:
    case AST_NODE_SYMBOL:
    {
        printf(" [%s: " SV_Fmt "]\n", dpl_lexer_token_kind_name(node->as.literal.value.kind), SV_Arg(node->as.literal.value.text));
        break;
    }
    case AST_NODE_OBJECT_LITERAL:
    {
        DPL_Ast_ObjectLiteral object_literal = node->as.object_literal;
        printf("\n");
        for (size_t i = 0; i < object_literal.field_count; ++i)
        {
            _dpla_print_indent(level + 1);
            printf("<field #%zu>\n", i);
            _dpla_print(object_literal.fields[i], level + 2);
        }
        break;
    }
    break;
    case AST_NODE_FIELD_ACCESS:
    {
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
    case AST_NODE_FUNCTIONCALL:
    {
        DPL_Ast_FunctionCall call = node->as.function_call;
        printf(" [%s: " SV_Fmt "]\n", dpl_lexer_token_kind_name(call.name.kind), SV_Arg(call.name.text));
        for (size_t i = 0; i < call.argument_count; ++i)
        {
            _dpla_print_indent(level + 1);
            printf("<arg #%zu>\n", i);
            _dpla_print(call.arguments[i], level + 2);
        }
        break;
    }
    case AST_NODE_SCOPE:
    {
        DPL_Ast_Scope scope = node->as.scope;
        printf("\n");
        for (size_t i = 0; i < scope.expression_count; ++i)
        {
            _dpla_print_indent(level + 1);
            printf("<expr #%zu>\n", i);
            _dpla_print(scope.expressions[i], level + 2);
        }
    }
    break;
    case AST_NODE_DECLARATION:
    {
        DPL_Ast_Declaration declaration = node->as.declaration;
        printf(" [%s " SV_Fmt ": %s]\n", dpl_lexer_token_kind_name(declaration.keyword.kind), SV_Arg(declaration.name.text), _dpla_type_name(declaration.type));
        _dpla_print_indent(level + 1);
        printf("<initialization>\n");
        _dpla_print(declaration.initialization, level + 2);
    }
    break;
    case AST_NODE_ASSIGNMENT:
    {
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
    case AST_NODE_FUNCTION:
    {
        DPL_Ast_Function function = node->as.function;
        printf(" [%s " SV_Fmt ": (", dpl_lexer_token_kind_name(function.keyword.kind), SV_Arg(function.name.text));
        for (size_t i = 0; i < function.signature.argument_count; ++i)
        {
            if (i > 0)
            {
                printf(", ");
            }
            DPL_Ast_FunctionArgument arg = function.signature.arguments[i];
            printf(SV_Fmt ": %s", SV_Arg(arg.name.text), _dpla_type_name(arg.type));
        }
        printf("): %s]\n", _dpla_type_name(function.signature.type));
        _dpla_print_indent(level + 1);
        printf("<body>\n");
        _dpla_print(function.body, level + 2);
    }
    break;
    case AST_NODE_CONDITIONAL:
    {
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
    case AST_NODE_WHILE_LOOP:
    {
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
    case AST_NODE_INTERPOLATION:
    {
        DPL_Ast_Interpolation interpolation = node->as.interpolation;
        printf("\n");
        for (size_t i = 0; i < interpolation.expression_count; ++i)
        {
            _dpla_print_indent(level + 1);
            printf("<expr #%zu>\n", i);
            _dpla_print(interpolation.expressions[i], level + 2);
        }
        break;
    }
    default:
    {
        printf("\n");
        break;
    }
    }
}

// PARSER

void _dplp_skip_whitespace(DPL *dpl)
{
    while (dpl_lexer_peek_token(&dpl->lexer).kind == TOKEN_WHITESPACE || dpl_lexer_peek_token(&dpl->lexer).kind == TOKEN_COMMENT)
    {
        dpl_lexer_next_token(&dpl->lexer);
    }
}

DPL_Token _dplp_next_token(DPL *dpl)
{
    _dplp_skip_whitespace(dpl);
    return dpl_lexer_next_token(&dpl->lexer);
}

DPL_Token _dplp_peek_token(DPL *dpl)
{
    _dplp_skip_whitespace(dpl);
    return dpl_lexer_peek_token(&dpl->lexer);
}

void _dplp_check_token(DPL *dpl, DPL_Token token, DPL_TokenKind kind)
{
    if (token.kind != kind)
    {
        DPL_TOKEN_ERROR(dpl->lexer.source, token, "Unexpected %s, expected %s.",
                        dpl_lexer_token_kind_name(token.kind), dpl_lexer_token_kind_name(kind));
    }
}

DPL_Token _dplp_expect_token(DPL *dpl, DPL_TokenKind kind)
{
    DPL_Token next_token = _dplp_next_token(dpl);
    _dplp_check_token(dpl, next_token, kind);
    return next_token;
}

DPL_Ast_Node *_dplp_parse_expression(DPL *dpl);
DPL_Ast_Node *_dplp_parse_scope(DPL *dpl, DPL_Token opening_token, DPL_TokenKind closing_token_kind);

int _dplp_compare_object_type_fields(void const *a, void const *b)
{
    return nob_sv_cmp(((DPL_Ast_TypeField *)a)->name.text, ((DPL_Ast_TypeField *)b)->name.text);
}

DPL_Ast_Type *_dplp_parse_type(DPL *dpl)
{
    DPL_Token type_begin = _dplp_peek_token(dpl);
    switch (type_begin.kind)
    {
    case TOKEN_IDENTIFIER:
    {
        _dplp_next_token(dpl);

        DPL_Ast_Type *name_type = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_Type));
        name_type->kind = TYPE_BASE;
        name_type->first = type_begin;
        name_type->last = type_begin;
        name_type->as.name = type_begin;
        return name_type;
    }
    case TOKEN_OPEN_BRACKET:
    {
        _dplp_next_token(dpl);

        da_array(DPL_Ast_TypeField) tmp_fields = NULL;
        while (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_BRACKET)
        {
            if (da_size(tmp_fields) > 0)
            {
                _dplp_expect_token(dpl, TOKEN_COMMA);
            }
            DPL_Token name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);
            _dplp_expect_token(dpl, TOKEN_COLON);
            DPL_Ast_Type *type = _dplp_parse_type(dpl);
            DPL_Ast_TypeField field = {
                .name = name,
                .first = name,
                .last = type->last,
                .type = type,
            };

            for (size_t i = 0; i < da_size(tmp_fields); ++i)
            {
                if (nob_sv_eq(tmp_fields[i].name.text, name.text))
                {
                    DPL_AST_ERROR_WITH_NOTE(dpl->lexer.source,
                                            &tmp_fields[i], "Previous declaration was here.",
                                            &field, "Duplicate field `" SV_Fmt "` in object type.", SV_Arg(name.text));
                }
            }
            da_add(tmp_fields, field);
        }
        qsort(tmp_fields, da_size(tmp_fields), sizeof(*tmp_fields), _dplp_compare_object_type_fields);

        DPL_Ast_TypeField *fields = NULL;
        if (da_some(tmp_fields))
        {
            fields = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_TypeField) * da_size(tmp_fields));
            memcpy(fields, tmp_fields, sizeof(DPL_Ast_TypeField) * da_size(tmp_fields));
        }

        DPL_Ast_Type *object_type = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_Type));
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
        DPL_TOKEN_ERROR(dpl->lexer.source, type_begin, "Invalid %s in type reference, expected %s.", dpl_lexer_token_kind_name(type_begin.kind),
                        dpl_lexer_token_kind_name(TOKEN_IDENTIFIER));
    }
}

DPL_Ast_Node *_dplp_parse_declaration(DPL *dpl)
{
    DPL_Token keyword_candidate = _dplp_peek_token(dpl);
    if (keyword_candidate.kind == TOKEN_KEYWORD_CONSTANT || keyword_candidate.kind == TOKEN_KEYWORD_VAR)
    {
        DPL_Token keyword = _dplp_next_token(dpl);
        DPL_Token name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);

        DPL_Ast_Type *type = NULL;
        if (_dplp_peek_token(dpl).kind == TOKEN_COLON)
        {
            _dplp_expect_token(dpl, TOKEN_COLON);
            type = _dplp_parse_type(dpl);
        }

        DPL_Token assignment = _dplp_expect_token(dpl, TOKEN_COLON_EQUAL);

        DPL_Ast_Node *initialization = _dplp_parse_expression(dpl);

        DPL_Ast_Node *declaration = _dpla_create_node(&dpl->tree, AST_NODE_DECLARATION, keyword, initialization->last);
        declaration->as.declaration.keyword = keyword;
        declaration->as.declaration.name = name;
        declaration->as.declaration.type = type;
        declaration->as.declaration.assignment = assignment;
        declaration->as.declaration.initialization = initialization;
        return declaration;
    }
    else if (keyword_candidate.kind == TOKEN_KEYWORD_FUNCTION)
    {
        DPL_Token keyword = _dplp_next_token(dpl);
        DPL_Token name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);

        da_array(DPL_Ast_FunctionArgument) arguments = 0;
        _dplp_expect_token(dpl, TOKEN_OPEN_PAREN);
        if (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_PAREN)
        {
            while (true)
            {
                DPL_Ast_FunctionArgument argument = {0};
                argument.name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);
                _dplp_expect_token(dpl, TOKEN_COLON);
                argument.type = _dplp_parse_type(dpl);

                da_add(arguments, argument);

                if (_dplp_peek_token(dpl).kind != TOKEN_COMMA)
                {
                    break;
                }
                else
                {
                    _dplp_next_token(dpl);
                }
            }
        }
        _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);

        DPL_Ast_Type *result_type = NULL;
        if (_dplp_peek_token(dpl).kind == TOKEN_COLON)
        {
            _dplp_expect_token(dpl, TOKEN_COLON);
            result_type = _dplp_parse_type(dpl);
        }

        _dplp_expect_token(dpl, TOKEN_COLON_EQUAL);

        DPL_Ast_Node *body = _dplp_parse_expression(dpl);

        DPL_Ast_Node *function = _dpla_create_node(&dpl->tree, AST_NODE_FUNCTION, keyword, body->last);
        function->as.function.keyword = keyword;
        function->as.function.name = name;

        function->as.function.signature.argument_count = da_size(arguments);
        if (da_some(arguments))
        {
            function->as.function.signature.arguments = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_FunctionArgument) * da_size(arguments));
            memcpy(function->as.function.signature.arguments, arguments, sizeof(DPL_Ast_FunctionArgument) * da_size(arguments));
            da_free(arguments);
        }
        function->as.function.signature.type = result_type;

        function->as.function.body = body;

        return function;
    }
    else if (keyword_candidate.kind == TOKEN_KEYWORD_TYPE)
    {
        DPL_Token keyword = _dplp_next_token(dpl);
        DPL_Token name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);

        DPL_Token assignment = _dplp_expect_token(dpl, TOKEN_COLON_EQUAL);

        DPL_Ast_Type *type = _dplp_parse_type(dpl);
        if (_dplp_peek_token(dpl).kind == TOKEN_COLON)
        {
            _dplp_expect_token(dpl, TOKEN_COLON);
            type = _dplp_parse_type(dpl);
        }

        DPL_Ast_Node *declaration = _dpla_create_node(&dpl->tree, AST_NODE_DECLARATION, keyword, type->last);
        declaration->as.declaration.keyword = keyword;
        declaration->as.declaration.name = name;
        declaration->as.declaration.type = type;
        declaration->as.declaration.assignment = assignment;
        declaration->as.declaration.initialization = NULL;
        return declaration;
    }

    return _dplp_parse_expression(dpl);
}
void _dplp_move_nodelist(DPL *dpl, da_array(DPL_Ast_Node *) list, size_t *target_count, DPL_Ast_Node ***target_items)
{
    *target_count = da_size(list);
    if (da_some(list))
    {
        *target_items = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_Node *) * da_size(list));
        memcpy(*target_items, list, sizeof(DPL_Ast_Node *) * da_size(list));
        da_free(list);
    }
}

da_array(DPL_Ast_Node *) _dplp_parse_expressions(DPL *dpl, DPL_TokenKind delimiter, DPL_TokenKind closing, bool allow_declarations)
{
    da_array(DPL_Ast_Node *) list = 0;
    if (allow_declarations)
    {
        da_add(list, _dplp_parse_declaration(dpl));
    }
    else
    {
        da_add(list, _dplp_parse_expression(dpl));
    }

    DPL_Token delimiter_candidate = _dplp_peek_token(dpl);
    while (delimiter_candidate.kind == delimiter)
    {
        _dplp_next_token(dpl);
        if (_dplp_peek_token(dpl).kind == closing)
        {
            break;
        }

        if (allow_declarations)
        {
            da_add(list, _dplp_parse_declaration(dpl));
        }
        else
        {
            da_add(list, _dplp_parse_expression(dpl));
        }
        delimiter_candidate = _dplp_peek_token(dpl);
    }

    return list;
}

DPL_Ast_Node *_dplp_parse_primary(DPL *dpl)
{
    DPL_Token token = _dplp_next_token(dpl);
    switch (token.kind)
    {
    case TOKEN_NUMBER:
    case TOKEN_STRING:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
        return _dpla_create_literal(&dpl->tree, token);
    case TOKEN_OPEN_PAREN:
    {
        DPL_Ast_Node *node = _dplp_parse_expression(dpl);
        node->first = token;
        node->last = _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);
        return node;
    }
    break;
    case TOKEN_OPEN_BRACE:
    {
        DPL_Ast_Node *node = _dplp_parse_scope(dpl, token, TOKEN_CLOSE_BRACE);
        return node;
    }
    case TOKEN_IDENTIFIER:
    {
        if (_dplp_peek_token(dpl).kind == TOKEN_OPEN_PAREN)
        {
            DPL_Token name = token;
            /*DPL_Token open_paren =*/_dplp_expect_token(dpl, TOKEN_OPEN_PAREN);

            da_array(DPL_Ast_Node *) arguments = 0;
            if (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_PAREN)
            {
                arguments = _dplp_parse_expressions(dpl, TOKEN_COMMA, TOKEN_CLOSE_PAREN, false);
            }

            DPL_Token closing_paren = _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);

            DPL_Ast_Node *node = _dpla_create_node(&dpl->tree, AST_NODE_FUNCTIONCALL, name, closing_paren);
            node->as.function_call.name = name;
            _dplp_move_nodelist(dpl, arguments, &node->as.function_call.argument_count, &node->as.function_call.arguments);
            return node;
        }

        DPL_Ast_Node *node = _dpla_create_node(&dpl->tree, AST_NODE_SYMBOL, token, token);
        node->as.symbol = token;
        return node;
    }
    case TOKEN_OPEN_BRACKET:
    {
        bool first = true;
        da_array(DPL_Ast_Node *) tmp_fields = NULL;
        while (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_BRACKET)
        {
            if (!first)
            {
                _dplp_expect_token(dpl, TOKEN_COMMA);
            }

            DPL_Ast_Node *field = _dplp_parse_expression(dpl);
            da_add(tmp_fields, field);

            first = false;
        }

        DPL_Ast_Node *object_literal = _dpla_create_node(&dpl->tree, AST_NODE_OBJECT_LITERAL, token, _dplp_next_token(dpl));
        _dplp_move_nodelist(dpl, tmp_fields, &object_literal->as.object_literal.field_count,
                            &object_literal->as.object_literal.fields);

        return object_literal;
    }
    break;
    case TOKEN_STRING_INTERPOLATION:
    {
        da_array(DPL_Ast_Node *) tmp_parts = NULL;

        while (token.kind == TOKEN_STRING_INTERPOLATION)
        {
            if (token.text.count > 3)
            {
                da_add(tmp_parts, _dpla_create_literal(&dpl->tree, token));
            }

            DPL_Ast_Node *expression = _dplp_parse_expression(dpl);
            da_add(tmp_parts, expression);

            token = _dplp_next_token(dpl);
        }

        _dplp_check_token(dpl, token, TOKEN_STRING);
        if (token.text.count > 2)
        {
            da_add(tmp_parts, _dpla_create_literal(&dpl->tree, token));
        }

        DPL_Ast_Node *node = _dpla_create_node(&dpl->tree, AST_NODE_INTERPOLATION,
                                               da_first(tmp_parts)->first, da_last(tmp_parts)->last);
        _dplp_move_nodelist(dpl, tmp_parts, &node->as.interpolation.expression_count, &node->as.interpolation.expressions);

        return node;
    }
    break;
    default:
    {
        DPL_TOKEN_ERROR(dpl->lexer.source, token, "Unexpected %s.", dpl_lexer_token_kind_name(token.kind));
    }
    }
}

DPL_Ast_Node *_dplp_parse_dot(DPL *dpl)
{
    DPL_Ast_Node *expression = _dplp_parse_primary(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_DOT)
    {
        _dplp_next_token(dpl);
        DPL_Ast_Node *new_expression = _dplp_parse_primary(dpl);

        if (new_expression->kind == AST_NODE_SYMBOL)
        {
            DPL_Ast_Node *field_access = _dpla_create_node(&dpl->tree, AST_NODE_FIELD_ACCESS, expression->first, new_expression->last);
            field_access->as.field_access.expression = expression;
            field_access->as.field_access.field = new_expression;
            expression = field_access;
        }
        else if (new_expression->kind == AST_NODE_FUNCTIONCALL)
        {
            DPL_Ast_FunctionCall *fc = &new_expression->as.function_call;
            fc->arguments = arena_realloc(&dpl->tree.memory, fc->arguments,
                                          fc->argument_count * sizeof(DPL_Ast_Node *),
                                          (fc->argument_count + 1) * sizeof(DPL_Ast_Node *));
            fc->argument_count++;

            for (size_t i = fc->argument_count - 1; i > 0; --i)
            {
                fc->arguments[i] = fc->arguments[i - 1];
            }

            fc->arguments[0] = expression;
            expression = new_expression;
        }
        else
        {
            DPL_AST_ERROR(dpl->lexer.source, new_expression, "Right-hand operand of operator `.` must be either a symbol or a function call.");
        }

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node *_dplp_parse_unary(DPL *dpl)
{
    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    if (operator_candidate.kind == TOKEN_MINUS || operator_candidate.kind == TOKEN_BANG || operator_candidate.kind == TOKEN_DOT_DOT)
    {
        DPL_Token operator= _dplp_next_token(dpl);
        DPL_Ast_Node *operand = _dplp_parse_unary(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_UNARY, operator, operand->last);
        new_expression->as.unary.operator= operator;
        new_expression->as.unary.operand = operand;
        return new_expression;
    }

    return _dplp_parse_dot(dpl);
}

DPL_Ast_Node *_dplp_parse_multiplicative(DPL *dpl)
{
    DPL_Ast_Node *expression = _dplp_parse_unary(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_STAR || operator_candidate.kind == TOKEN_SLASH)
    {
        _dplp_next_token(dpl);
        DPL_Ast_Node *rhs = _dplp_parse_unary(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node *_dplp_parse_additive(DPL *dpl)
{
    DPL_Ast_Node *expression = _dplp_parse_multiplicative(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_PLUS || operator_candidate.kind == TOKEN_MINUS)
    {
        _dplp_next_token(dpl);
        DPL_Ast_Node *rhs = _dplp_parse_multiplicative(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node *_dplp_parse_comparison(DPL *dpl)
{
    DPL_Ast_Node *expression = _dplp_parse_additive(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_LESS || operator_candidate.kind == TOKEN_LESS_EQUAL || operator_candidate.kind == TOKEN_GREATER || operator_candidate.kind == TOKEN_GREATER_EQUAL)
    {
        _dplp_next_token(dpl);
        DPL_Ast_Node *rhs = _dplp_parse_additive(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node *_dplp_parse_equality(DPL *dpl)
{
    DPL_Ast_Node *expression = _dplp_parse_comparison(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_EQUAL_EQUAL || operator_candidate.kind == TOKEN_BANG_EQUAL)
    {
        _dplp_next_token(dpl);
        DPL_Ast_Node *rhs = _dplp_parse_comparison(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node *_dplp_parse_and(DPL *dpl)
{
    DPL_Ast_Node *expression = _dplp_parse_equality(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_AND_AND)
    {
        _dplp_next_token(dpl);
        DPL_Ast_Node *rhs = _dplp_parse_equality(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node *_dplp_parse_or(DPL *dpl)
{
    DPL_Ast_Node *expression = _dplp_parse_and(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_PIPE_PIPE)
    {
        _dplp_next_token(dpl);
        DPL_Ast_Node *rhs = _dplp_parse_and(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node *_dplp_parse_conditional_or_loop(DPL *dpl)
{
    DPL_Token keyword_candidate = _dplp_peek_token(dpl);

    if (keyword_candidate.kind == TOKEN_KEYWORD_IF)
    {
        _dplp_next_token(dpl);

        _dplp_expect_token(dpl, TOKEN_OPEN_PAREN);
        DPL_Ast_Node *condition = _dplp_parse_expression(dpl);
        _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);
        DPL_Ast_Node *then_clause = _dplp_parse_expression(dpl);
        _dplp_expect_token(dpl, TOKEN_KEYWORD_ELSE);
        DPL_Ast_Node *else_clause = _dplp_parse_expression(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_CONDITIONAL, keyword_candidate, else_clause->last);
        new_expression->as.conditional.condition = condition;
        new_expression->as.conditional.then_clause = then_clause;
        new_expression->as.conditional.else_clause = else_clause;
        return new_expression;
    }
    else if (keyword_candidate.kind == TOKEN_KEYWORD_WHILE)
    {
        _dplp_next_token(dpl);

        _dplp_expect_token(dpl, TOKEN_OPEN_PAREN);
        DPL_Ast_Node *condition = _dplp_parse_expression(dpl);
        _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);
        DPL_Ast_Node *body = _dplp_parse_expression(dpl);

        DPL_Ast_Node *new_expression = _dpla_create_node(&dpl->tree, AST_NODE_WHILE_LOOP, keyword_candidate, body->last);
        new_expression->as.while_loop.condition = condition;
        new_expression->as.while_loop.body = body;
        return new_expression;
    }

    return _dplp_parse_or(dpl);
}

DPL_Ast_Node *_dplp_parse_assignment(DPL *dpl)
{
    DPL_Ast_Node *target = _dplp_parse_conditional_or_loop(dpl);

    if (_dplp_peek_token(dpl).kind == TOKEN_COLON_EQUAL)
    {
        if (target->kind == AST_NODE_FIELD_ACCESS)
        {
            DPL_AST_ERROR(dpl->lexer.source, target, "Object fields cannot be assigned directly. Instead, you can compose a new object from the old one.");
        }
        else if (target->kind != AST_NODE_SYMBOL)
        {
            DPL_AST_ERROR(dpl->lexer.source, target, "`%s` is not a valid assignment target.",
                          _dpla_node_kind_name(target->kind));
        }

        DPL_Token assignment = _dplp_next_token(dpl);
        DPL_Ast_Node *expression = _dplp_parse_assignment(dpl);

        DPL_Ast_Node *node = _dpla_create_node(&dpl->tree, AST_NODE_ASSIGNMENT, target->first, expression->last);
        node->as.assignment.target = target;
        node->as.assignment.assignment = assignment;
        node->as.assignment.expression = expression;
        return node;
    }

    return target;
}

DPL_Ast_Node *_dplp_parse_expression(DPL *dpl)
{
    return _dplp_parse_assignment(dpl);
}

DPL_Ast_Node *_dplp_parse_scope(DPL *dpl, DPL_Token opening_token, DPL_TokenKind closing_token_kind)
{
    DPL_Token closing_token_candidate = _dplp_peek_token(dpl);
    if (closing_token_candidate.kind == closing_token_kind)
    {
        DPL_TOKEN_ERROR(dpl->lexer.source, closing_token_candidate,
                        "Unexpected %s. Expected at least one expression in scope.",
                        dpl_lexer_token_kind_name(closing_token_kind));
    }

    da_array(DPL_Ast_Node *) expressions = _dplp_parse_expressions(dpl, TOKEN_SEMICOLON, closing_token_kind, true);
    if (opening_token.kind == TOKEN_NONE)
    {
        opening_token = expressions[0]->first;
    }

    DPL_Token closing_token = _dplp_expect_token(dpl, closing_token_kind);

    DPL_Ast_Node *node = _dpla_create_node(&dpl->tree, AST_NODE_SCOPE, opening_token, closing_token);
    _dplp_move_nodelist(dpl, expressions, &node->as.scope.expression_count, &node->as.scope.expressions);
    return node;
}

void _dplp_parse(DPL *dpl)
{
    dpl->tree.root = _dplp_parse_scope(dpl, dpl->lexer.first_token, TOKEN_EOF);
}

// COMPILATION PROCESS

void dpl_compile(DPL *dpl, DPL_Program *program)
{
    _dplp_parse(dpl);
    if (dpl->debug)
    {
        _dpla_print(dpl->tree.root, 0);
        printf("\n");
    }

    DPL_Binding binding = (DPL_Binding){
        .memory = &dpl->bound_tree.memory,
        .source = dpl->lexer.source,
        .symbols = &dpl->symbols,
        .user_functions = 0,
    };
    dpl->bound_tree.root = dpl_bind_node(&binding, dpl->tree.root);

    if (dpl->debug)
    {
        for (size_t i = 0; i < da_size(binding.user_functions); ++i)
        {
            DPL_Binding_UserFunction *uf = &binding.user_functions[i];
            printf("### " SV_Fmt " (arity: %zu) ###\n", SV_Arg(uf->function->name), uf->arity);
            dpl_bind_print(&binding, uf->body, 0);
            printf("\n");
        }

        printf("### program ###\n");
        dpl_bind_print(&binding, dpl->bound_tree.root, 0);
        printf("\n");
    }

    DPL_Generator generator = {
        .user_functions = binding.user_functions,
    };
    for (size_t i = 0; i < da_size(generator.user_functions); ++i)
    {
        DPL_Binding_UserFunction *uf = &generator.user_functions[i];
        uf->begin_ip = da_size(program->code);
        dpl_generate(&generator, uf->body, program);
        dplp_write_return(program);
    }

    program->entry = da_size(program->code);
    dpl_generate(&generator, dpl->bound_tree.root, program);
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

    da_free(binding.user_functions);
}