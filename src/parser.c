#include <dpl/parser.h>
#include <error.h>

const char *AST_NODE_KIND_NAMES[COUNT_AST_NODE_KINDS] = {

    [AST_NODE_LITERAL] = "AST_NODE_LITERAL",
    [AST_NODE_OBJECT_LITERAL] = "AST_NODE_OBJECT_LITERAL",
    [AST_NODE_UNARY] = "AST_NODE_UNARY",
    [AST_NODE_BINARY] = "AST_NODE_BINARY",
    [AST_NODE_FUNCTIONCALL] = "AST_NODE_FUNCTIONCALL",
    [AST_NODE_SCOPE] = "AST_NODE_SCOPE",
    [AST_NODE_DECLARATION] = "AST_NODE_DECLARATION",
    [AST_NODE_SYMBOL] = "AST_NODE_SYMBOL",
    [AST_NODE_ASSIGNMENT] = "AST_NODE_ASSIGNMENT",
    [AST_NODE_FUNCTION] = "AST_NODE_FUNCTION",
    [AST_NODE_CONDITIONAL] = "AST_NODE_CONDITIONAL",
    [AST_NODE_WHILE_LOOP] = "AST_NODE_WHILE_LOOP",
    [AST_NODE_FIELD_ACCESS] = "AST_NODE_FIELD_ACCESS",
    [AST_NODE_INTERPOLATION] = "AST_NODE_INTERPOLATION",
};

static_assert(COUNT_AST_NODE_KINDS == 14,
              "Count of ast node kinds has changed, please update bound node kind names map.");

const char *dpl_parse_nodekind_name(DPL_AstNodeKind kind)
{
    if (kind < 0 || kind >= COUNT_AST_NODE_KINDS)
    {
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
    return AST_NODE_KIND_NAMES[kind];
}

// AST

DPL_Ast_Node *dpl_parse_allocate_node(DPL_Parser *parser, DPL_AstNodeKind kind, DPL_Token first, DPL_Token last)
{
    DPL_Ast_Node *node = arena_alloc(parser->memory, sizeof(DPL_Ast_Node));
    node->kind = kind;
    node->first = first;
    node->last = last;
    return node;
}

void dpl_parse_move_nodelist(DPL_Parser *parser, da_array(DPL_Ast_Node *) list, size_t *target_count, DPL_Ast_Node ***target_items)
{
    *target_count = da_size(list);
    if (da_some(list))
    {
        *target_items = arena_alloc(parser->memory, sizeof(DPL_Ast_Node *) * da_size(list));
        memcpy(*target_items, list, sizeof(DPL_Ast_Node *) * da_size(list));
        da_free(list);
    }
}

DPL_Ast_Node *dpl_parse_allocate_literal(DPL_Parser *parser, DPL_Token token)
{
    DPL_Ast_Node *node = dpl_parse_allocate_node(parser, AST_NODE_LITERAL, token, token);
    node->as.literal.value = token;
    return node;
}

void dpl_parse_build_type_name(DPL_Ast_Type *ast_type, Nob_String_Builder *sb)
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
            dpl_parse_build_type_name(field.type, sb);
        }
        nob_sb_append_cstr(sb, "]");
    }
    break;
    default:
        DW_UNIMPLEMENTED_MSG("Cannot print ast type %d.", ast_type->kind);
    }
}

const char *dpl_parse_type_name(DPL_Ast_Type *ast_type)
{
    static char result[256];
    if (!ast_type)
    {
        return dpl_lexer_token_kind_name(TOKEN_NONE);
    }
    Nob_String_Builder sb = {0};
    dpl_parse_build_type_name(ast_type, &sb);
    nob_sb_append_null(&sb);

    strncpy(result, sb.items, NOB_ARRAY_LEN(result));
    return result;
}

void dpl_parse_print_indent(size_t level)
{
    for (size_t i = 0; i < level; ++i)
    {
        printf("  ");
    }
}

void dpl_parse_print(DPL_Ast_Node *node, size_t level)
{
    dpl_parse_print_indent(level);

    if (!node)
    {
        printf("<nil>\n");
        return;
    }

    printf("%s", dpl_parse_nodekind_name(node->kind));

    switch (node->kind)
    {
    case AST_NODE_UNARY:
    {
        printf(" [%s]\n", dpl_lexer_token_kind_name(node->as.unary.operator.kind));
        dpl_parse_print(node->as.unary.operand, level + 1);
        break;
    }
    case AST_NODE_BINARY:
    {
        printf(" [%s]\n", dpl_lexer_token_kind_name(node->as.binary.operator.kind));
        dpl_parse_print(node->as.binary.left, level + 1);
        dpl_parse_print(node->as.binary.right, level + 1);
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
            dpl_parse_print_indent(level + 1);
            printf("<field #%zu>\n", i);
            dpl_parse_print(object_literal.fields[i], level + 2);
        }
        break;
    }
    break;
    case AST_NODE_FIELD_ACCESS:
    {
        DPL_Ast_FieldAccess field_access = node->as.field_access;
        printf("\n");

        dpl_parse_print_indent(level + 1);
        printf("<expression>\n");
        dpl_parse_print(field_access.expression, level + 2);

        dpl_parse_print_indent(level + 1);
        printf("<field>\n");
        dpl_parse_print(field_access.field, level + 2);
    }
    break;
    case AST_NODE_FUNCTIONCALL:
    {
        DPL_Ast_FunctionCall call = node->as.function_call;
        printf(" [%s: " SV_Fmt "]\n", dpl_lexer_token_kind_name(call.name.kind), SV_Arg(call.name.text));
        for (size_t i = 0; i < call.argument_count; ++i)
        {
            dpl_parse_print_indent(level + 1);
            printf("<arg #%zu>\n", i);
            dpl_parse_print(call.arguments[i], level + 2);
        }
        break;
    }
    case AST_NODE_SCOPE:
    {
        DPL_Ast_Scope scope = node->as.scope;
        printf("\n");
        for (size_t i = 0; i < scope.expression_count; ++i)
        {
            dpl_parse_print_indent(level + 1);
            printf("<expr #%zu>\n", i);
            dpl_parse_print(scope.expressions[i], level + 2);
        }
    }
    break;
    case AST_NODE_DECLARATION:
    {
        DPL_Ast_Declaration declaration = node->as.declaration;
        printf(" [%s " SV_Fmt ": %s]\n", dpl_lexer_token_kind_name(declaration.keyword.kind), SV_Arg(declaration.name.text), dpl_parse_type_name(declaration.type));
        dpl_parse_print_indent(level + 1);
        printf("<initialization>\n");
        dpl_parse_print(declaration.initialization, level + 2);
    }
    break;
    case AST_NODE_ASSIGNMENT:
    {
        DPL_Ast_Assignment assigment = node->as.assignment;
        printf("\n");

        dpl_parse_print_indent(level + 1);
        printf("<target>\n");
        dpl_parse_print(assigment.target, level + 2);

        dpl_parse_print_indent(level + 1);
        printf("<exression>\n");
        dpl_parse_print(assigment.expression, level + 2);
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
            printf(SV_Fmt ": %s", SV_Arg(arg.name.text), dpl_parse_type_name(arg.type));
        }
        printf("): %s]\n", dpl_parse_type_name(function.signature.type));
        dpl_parse_print_indent(level + 1);
        printf("<body>\n");
        dpl_parse_print(function.body, level + 2);
    }
    break;
    case AST_NODE_CONDITIONAL:
    {
        DPL_Ast_Conditional conditional = node->as.conditional;
        printf("\n");

        dpl_parse_print_indent(level + 1);
        printf("<condition>\n");
        dpl_parse_print(conditional.condition, level + 2);

        dpl_parse_print_indent(level + 1);
        printf("<then_clause>\n");
        dpl_parse_print(conditional.then_clause, level + 2);

        dpl_parse_print_indent(level + 1);
        printf("<else_clause>\n");
        dpl_parse_print(conditional.else_clause, level + 2);
    }
    break;
    case AST_NODE_WHILE_LOOP:
    {
        DPL_Ast_WhileLoop while_loop = node->as.while_loop;
        printf("\n");

        dpl_parse_print_indent(level + 1);
        printf("<condition>\n");
        dpl_parse_print(while_loop.condition, level + 2);

        dpl_parse_print_indent(level + 1);
        printf("<body>\n");
        dpl_parse_print(while_loop.body, level + 2);
    }
    break;
    case AST_NODE_INTERPOLATION:
    {
        DPL_Ast_Interpolation interpolation = node->as.interpolation;
        printf("\n");
        for (size_t i = 0; i < interpolation.expression_count; ++i)
        {
            dpl_parse_print_indent(level + 1);
            printf("<expr #%zu>\n", i);
            dpl_parse_print(interpolation.expressions[i], level + 2);
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

void dpl_parse_skip_whitespace(DPL_Parser *parser)
{
    while (dpl_lexer_peek_token(parser->lexer).kind == TOKEN_WHITESPACE || dpl_lexer_peek_token(parser->lexer).kind == TOKEN_COMMENT)
    {
        dpl_lexer_next_token(parser->lexer);
    }
}

DPL_Token dpl_parse_next_token(DPL_Parser *parser)
{
    dpl_parse_skip_whitespace(parser);
    return dpl_lexer_next_token(parser->lexer);
}

DPL_Token dpl_parse_peek_token(DPL_Parser *parser)
{
    dpl_parse_skip_whitespace(parser);
    return dpl_lexer_peek_token(parser->lexer);
}

void dpl_parse_check_token(DPL_Parser *parser, DPL_Token token, DPL_TokenKind kind)
{
    if (token.kind != kind)
    {
        DPL_TOKEN_ERROR(parser->lexer->source, token, "Unexpected %s, expected %s.",
                        dpl_lexer_token_kind_name(token.kind), dpl_lexer_token_kind_name(kind));
    }
}

DPL_Token dpl_parse_expect_token(DPL_Parser *parser, DPL_TokenKind kind)
{
    DPL_Token next_token = dpl_parse_next_token(parser);
    dpl_parse_check_token(parser, next_token, kind);
    return next_token;
}

DPL_Ast_Node *dpl_parse_expression(DPL_Parser *parser);
DPL_Ast_Node *dpl_parse_scope(DPL_Parser *parser, DPL_Token opening_token, DPL_TokenKind closing_token_kind);

int dpl_parse_compare_object_type_fields(void const *a, void const *b)
{
    return nob_sv_cmp(((DPL_Ast_TypeField *)a)->name.text, ((DPL_Ast_TypeField *)b)->name.text);
}

DPL_Ast_Type *dpl_parse_type(DPL_Parser *parser)
{
    DPL_Token type_begin = dpl_parse_peek_token(parser);
    switch (type_begin.kind)
    {
    case TOKEN_IDENTIFIER:
    {
        dpl_parse_next_token(parser);

        DPL_Ast_Type *name_type = arena_alloc(parser->memory, sizeof(DPL_Ast_Type));
        name_type->kind = TYPE_BASE;
        name_type->first = type_begin;
        name_type->last = type_begin;
        name_type->as.name = type_begin;
        return name_type;
    }
    case TOKEN_OPEN_BRACKET:
    {
        dpl_parse_next_token(parser);

        da_array(DPL_Ast_TypeField) tmp_fields = NULL;
        while (dpl_parse_peek_token(parser).kind != TOKEN_CLOSE_BRACKET)
        {
            if (da_size(tmp_fields) > 0)
            {
                dpl_parse_expect_token(parser, TOKEN_COMMA);
            }
            DPL_Token name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);
            dpl_parse_expect_token(parser, TOKEN_COLON);
            DPL_Ast_Type *type = dpl_parse_type(parser);
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
                    DPL_AST_ERROR_WITH_NOTE(parser->lexer->source,
                                            &tmp_fields[i], "Previous declaration was here.",
                                            &field, "Duplicate field `" SV_Fmt "` in object type.", SV_Arg(name.text));
                }
            }
            da_add(tmp_fields, field);
        }
        qsort(tmp_fields, da_size(tmp_fields), sizeof(*tmp_fields), dpl_parse_compare_object_type_fields);

        DPL_Ast_TypeField *fields = NULL;
        if (da_some(tmp_fields))
        {
            fields = arena_alloc(parser->memory, sizeof(DPL_Ast_TypeField) * da_size(tmp_fields));
            memcpy(fields, tmp_fields, sizeof(DPL_Ast_TypeField) * da_size(tmp_fields));
        }

        DPL_Ast_Type *object_type = arena_alloc(parser->memory, sizeof(DPL_Ast_Type));
        object_type->kind = TYPE_OBJECT;
        object_type->first = type_begin;
        object_type->last = dpl_parse_peek_token(parser);
        object_type->as.object.field_count = da_size(tmp_fields);
        object_type->as.object.fields = fields;

        da_free(tmp_fields);
        dpl_parse_next_token(parser);

        return object_type;
    }
    default:
        DPL_TOKEN_ERROR(parser->lexer->source, type_begin, "Invalid %s in type reference, expected %s.", dpl_lexer_token_kind_name(type_begin.kind),
                        dpl_lexer_token_kind_name(TOKEN_IDENTIFIER));
    }
}

DPL_Ast_Node *dpl_parse_declaration(DPL_Parser *parser)
{
    DPL_Token keyword_candidate = dpl_parse_peek_token(parser);
    if (keyword_candidate.kind == TOKEN_KEYWORD_CONSTANT || keyword_candidate.kind == TOKEN_KEYWORD_VAR)
    {
        DPL_Token keyword = dpl_parse_next_token(parser);
        DPL_Token name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);

        DPL_Ast_Type *type = NULL;
        if (dpl_parse_peek_token(parser).kind == TOKEN_COLON)
        {
            dpl_parse_expect_token(parser, TOKEN_COLON);
            type = dpl_parse_type(parser);
        }

        DPL_Token assignment = dpl_parse_expect_token(parser, TOKEN_COLON_EQUAL);

        DPL_Ast_Node *initialization = dpl_parse_expression(parser);

        DPL_Ast_Node *declaration = dpl_parse_allocate_node(parser, AST_NODE_DECLARATION, keyword, initialization->last);
        declaration->as.declaration.keyword = keyword;
        declaration->as.declaration.name = name;
        declaration->as.declaration.type = type;
        declaration->as.declaration.assignment = assignment;
        declaration->as.declaration.initialization = initialization;
        return declaration;
    }
    else if (keyword_candidate.kind == TOKEN_KEYWORD_FUNCTION)
    {
        DPL_Token keyword = dpl_parse_next_token(parser);
        DPL_Token name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);

        da_array(DPL_Ast_FunctionArgument) arguments = 0;
        dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);
        if (dpl_parse_peek_token(parser).kind != TOKEN_CLOSE_PAREN)
        {
            while (true)
            {
                DPL_Ast_FunctionArgument argument = {0};
                argument.name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);
                dpl_parse_expect_token(parser, TOKEN_COLON);
                argument.type = dpl_parse_type(parser);

                da_add(arguments, argument);

                if (dpl_parse_peek_token(parser).kind != TOKEN_COMMA)
                {
                    break;
                }
                else
                {
                    dpl_parse_next_token(parser);
                }
            }
        }
        dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);

        DPL_Ast_Type *result_type = NULL;
        if (dpl_parse_peek_token(parser).kind == TOKEN_COLON)
        {
            dpl_parse_expect_token(parser, TOKEN_COLON);
            result_type = dpl_parse_type(parser);
        }

        dpl_parse_expect_token(parser, TOKEN_COLON_EQUAL);

        DPL_Ast_Node *body = dpl_parse_expression(parser);

        DPL_Ast_Node *function = dpl_parse_allocate_node(parser, AST_NODE_FUNCTION, keyword, body->last);
        function->as.function.keyword = keyword;
        function->as.function.name = name;

        function->as.function.signature.argument_count = da_size(arguments);
        if (da_some(arguments))
        {
            function->as.function.signature.arguments = arena_alloc(parser->memory, sizeof(DPL_Ast_FunctionArgument) * da_size(arguments));
            memcpy(function->as.function.signature.arguments, arguments, sizeof(DPL_Ast_FunctionArgument) * da_size(arguments));
            da_free(arguments);
        }
        function->as.function.signature.type = result_type;

        function->as.function.body = body;

        return function;
    }
    else if (keyword_candidate.kind == TOKEN_KEYWORD_TYPE)
    {
        DPL_Token keyword = dpl_parse_next_token(parser);
        DPL_Token name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);

        DPL_Token assignment = dpl_parse_expect_token(parser, TOKEN_COLON_EQUAL);

        DPL_Ast_Type *type = dpl_parse_type(parser);
        if (dpl_parse_peek_token(parser).kind == TOKEN_COLON)
        {
            dpl_parse_expect_token(parser, TOKEN_COLON);
            type = dpl_parse_type(parser);
        }

        DPL_Ast_Node *declaration = dpl_parse_allocate_node(parser, AST_NODE_DECLARATION, keyword, type->last);
        declaration->as.declaration.keyword = keyword;
        declaration->as.declaration.name = name;
        declaration->as.declaration.type = type;
        declaration->as.declaration.assignment = assignment;
        declaration->as.declaration.initialization = NULL;
        return declaration;
    }

    return dpl_parse_expression(parser);
}

da_array(DPL_Ast_Node *) dpl_parse_expressions(DPL_Parser *parser, DPL_TokenKind delimiter, DPL_TokenKind closing, bool allow_declarations)
{
    da_array(DPL_Ast_Node *) list = 0;
    if (allow_declarations)
    {
        da_add(list, dpl_parse_declaration(parser));
    }
    else
    {
        da_add(list, dpl_parse_expression(parser));
    }

    DPL_Token delimiter_candidate = dpl_parse_peek_token(parser);
    while (delimiter_candidate.kind == delimiter)
    {
        dpl_parse_next_token(parser);
        if (dpl_parse_peek_token(parser).kind == closing)
        {
            break;
        }

        if (allow_declarations)
        {
            da_add(list, dpl_parse_declaration(parser));
        }
        else
        {
            da_add(list, dpl_parse_expression(parser));
        }
        delimiter_candidate = dpl_parse_peek_token(parser);
    }

    return list;
}

DPL_Ast_Node *dpl_parse_primary(DPL_Parser *parser)
{
    DPL_Token token = dpl_parse_next_token(parser);
    switch (token.kind)
    {
    case TOKEN_NUMBER:
    case TOKEN_STRING:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
        return dpl_parse_allocate_literal(parser, token);
    case TOKEN_OPEN_PAREN:
    {
        DPL_Ast_Node *node = dpl_parse_expression(parser);
        node->first = token;
        node->last = dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);
        return node;
    }
    break;
    case TOKEN_OPEN_BRACE:
    {
        DPL_Ast_Node *node = dpl_parse_scope(parser, token, TOKEN_CLOSE_BRACE);
        return node;
    }
    case TOKEN_IDENTIFIER:
    {
        if (dpl_parse_peek_token(parser).kind == TOKEN_OPEN_PAREN)
        {
            DPL_Token name = token;
            /*DPL_Token open_paren =*/dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);

            da_array(DPL_Ast_Node *) arguments = 0;
            if (dpl_parse_peek_token(parser).kind != TOKEN_CLOSE_PAREN)
            {
                arguments = dpl_parse_expressions(parser, TOKEN_COMMA, TOKEN_CLOSE_PAREN, false);
            }

            DPL_Token closing_paren = dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);

            DPL_Ast_Node *node = dpl_parse_allocate_node(parser, AST_NODE_FUNCTIONCALL, name, closing_paren);
            node->as.function_call.name = name;
            dpl_parse_move_nodelist(parser, arguments, &node->as.function_call.argument_count, &node->as.function_call.arguments);
            return node;
        }

        DPL_Ast_Node *node = dpl_parse_allocate_node(parser, AST_NODE_SYMBOL, token, token);
        node->as.symbol = token;
        return node;
    }
    case TOKEN_OPEN_BRACKET:
    {
        bool first = true;
        da_array(DPL_Ast_Node *) tmp_fields = NULL;
        while (dpl_parse_peek_token(parser).kind != TOKEN_CLOSE_BRACKET)
        {
            if (!first)
            {
                dpl_parse_expect_token(parser, TOKEN_COMMA);
            }

            DPL_Ast_Node *field = dpl_parse_expression(parser);
            da_add(tmp_fields, field);

            first = false;
        }

        DPL_Ast_Node *object_literal = dpl_parse_allocate_node(parser, AST_NODE_OBJECT_LITERAL, token, dpl_parse_next_token(parser));
        dpl_parse_move_nodelist(parser, tmp_fields, &object_literal->as.object_literal.field_count,
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
                da_add(tmp_parts, dpl_parse_allocate_literal(parser, token));
            }

            DPL_Ast_Node *expression = dpl_parse_expression(parser);
            da_add(tmp_parts, expression);

            token = dpl_parse_next_token(parser);
        }

        dpl_parse_check_token(parser, token, TOKEN_STRING);
        if (token.text.count > 2)
        {
            da_add(tmp_parts, dpl_parse_allocate_literal(parser, token));
        }

        DPL_Ast_Node *node = dpl_parse_allocate_node(parser, AST_NODE_INTERPOLATION,
                                                     da_first(tmp_parts)->first, da_last(tmp_parts)->last);
        dpl_parse_move_nodelist(parser, tmp_parts, &node->as.interpolation.expression_count, &node->as.interpolation.expressions);

        return node;
    }
    break;
    default:
    {
        DPL_TOKEN_ERROR(parser->lexer->source, token, "Unexpected %s.", dpl_lexer_token_kind_name(token.kind));
    }
    }
}

DPL_Ast_Node *dpl_parse_dot(DPL_Parser *parser)
{
    DPL_Ast_Node *expression = dpl_parse_primary(parser);

    DPL_Token operator_candidate = dpl_parse_peek_token(parser);
    while (operator_candidate.kind == TOKEN_DOT)
    {
        dpl_parse_next_token(parser);
        DPL_Ast_Node *new_expression = dpl_parse_primary(parser);

        if (new_expression->kind == AST_NODE_SYMBOL)
        {
            DPL_Ast_Node *field_access = dpl_parse_allocate_node(parser, AST_NODE_FIELD_ACCESS, expression->first, new_expression->last);
            field_access->as.field_access.expression = expression;
            field_access->as.field_access.field = new_expression;
            expression = field_access;
        }
        else if (new_expression->kind == AST_NODE_FUNCTIONCALL)
        {
            DPL_Ast_FunctionCall *fc = &new_expression->as.function_call;
            fc->arguments = arena_realloc(parser->memory, fc->arguments,
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
            DPL_AST_ERROR(parser->lexer->source, new_expression, "Right-hand operand of operator `.` must be either a symbol or a function call.");
        }

        operator_candidate = dpl_parse_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node *dpl_parse_unary(DPL_Parser *parser)
{
    DPL_Token operator_candidate = dpl_parse_peek_token(parser);
    if (operator_candidate.kind == TOKEN_MINUS || operator_candidate.kind == TOKEN_BANG || operator_candidate.kind == TOKEN_DOT_DOT)
    {
        DPL_Token operator= dpl_parse_next_token(parser);
        DPL_Ast_Node *operand = dpl_parse_unary(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_UNARY, operator, operand->last);
        new_expression->as.unary.operator= operator;
        new_expression->as.unary.operand = operand;
        return new_expression;
    }

    return dpl_parse_dot(parser);
}

DPL_Ast_Node *dpl_parse_multiplicative(DPL_Parser *parser)
{
    DPL_Ast_Node *expression = dpl_parse_unary(parser);

    DPL_Token operator_candidate = dpl_parse_peek_token(parser);
    while (operator_candidate.kind == TOKEN_STAR || operator_candidate.kind == TOKEN_SLASH)
    {
        dpl_parse_next_token(parser);
        DPL_Ast_Node *rhs = dpl_parse_unary(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = dpl_parse_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node *dpl_parse_additive(DPL_Parser *parser)
{
    DPL_Ast_Node *expression = dpl_parse_multiplicative(parser);

    DPL_Token operator_candidate = dpl_parse_peek_token(parser);
    while (operator_candidate.kind == TOKEN_PLUS || operator_candidate.kind == TOKEN_MINUS)
    {
        dpl_parse_next_token(parser);
        DPL_Ast_Node *rhs = dpl_parse_multiplicative(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = dpl_parse_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node *dpl_parse_comparison(DPL_Parser *parser)
{
    DPL_Ast_Node *expression = dpl_parse_additive(parser);

    DPL_Token operator_candidate = dpl_parse_peek_token(parser);
    while (operator_candidate.kind == TOKEN_LESS || operator_candidate.kind == TOKEN_LESS_EQUAL || operator_candidate.kind == TOKEN_GREATER || operator_candidate.kind == TOKEN_GREATER_EQUAL)
    {
        dpl_parse_next_token(parser);
        DPL_Ast_Node *rhs = dpl_parse_additive(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = dpl_parse_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node *dpl_parse_equality(DPL_Parser *parser)
{
    DPL_Ast_Node *expression = dpl_parse_comparison(parser);

    DPL_Token operator_candidate = dpl_parse_peek_token(parser);
    while (operator_candidate.kind == TOKEN_EQUAL_EQUAL || operator_candidate.kind == TOKEN_BANG_EQUAL)
    {
        dpl_parse_next_token(parser);
        DPL_Ast_Node *rhs = dpl_parse_comparison(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = dpl_parse_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node *dpl_parse_and(DPL_Parser *parser)
{
    DPL_Ast_Node *expression = dpl_parse_equality(parser);

    DPL_Token operator_candidate = dpl_parse_peek_token(parser);
    while (operator_candidate.kind == TOKEN_AND_AND)
    {
        dpl_parse_next_token(parser);
        DPL_Ast_Node *rhs = dpl_parse_equality(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = dpl_parse_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node *dpl_parse_or(DPL_Parser *parser)
{
    DPL_Ast_Node *expression = dpl_parse_and(parser);

    DPL_Token operator_candidate = dpl_parse_peek_token(parser);
    while (operator_candidate.kind == TOKEN_PIPE_PIPE)
    {
        dpl_parse_next_token(parser);
        DPL_Ast_Node *rhs = dpl_parse_and(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_BINARY, expression->first, rhs->last);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator= operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = dpl_parse_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node *dpl_parse_conditional_or_loop(DPL_Parser *parser)
{
    DPL_Token keyword_candidate = dpl_parse_peek_token(parser);

    if (keyword_candidate.kind == TOKEN_KEYWORD_IF)
    {
        dpl_parse_next_token(parser);

        dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);
        DPL_Ast_Node *condition = dpl_parse_expression(parser);
        dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);
        DPL_Ast_Node *then_clause = dpl_parse_expression(parser);
        dpl_parse_expect_token(parser, TOKEN_KEYWORD_ELSE);
        DPL_Ast_Node *else_clause = dpl_parse_expression(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_CONDITIONAL, keyword_candidate, else_clause->last);
        new_expression->as.conditional.condition = condition;
        new_expression->as.conditional.then_clause = then_clause;
        new_expression->as.conditional.else_clause = else_clause;
        return new_expression;
    }
    else if (keyword_candidate.kind == TOKEN_KEYWORD_WHILE)
    {
        dpl_parse_next_token(parser);

        dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);
        DPL_Ast_Node *condition = dpl_parse_expression(parser);
        dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);
        DPL_Ast_Node *body = dpl_parse_expression(parser);

        DPL_Ast_Node *new_expression = dpl_parse_allocate_node(parser, AST_NODE_WHILE_LOOP, keyword_candidate, body->last);
        new_expression->as.while_loop.condition = condition;
        new_expression->as.while_loop.body = body;
        return new_expression;
    }

    return dpl_parse_or(parser);
}

DPL_Ast_Node *dpl_parse_assignment(DPL_Parser *parser)
{
    DPL_Ast_Node *target = dpl_parse_conditional_or_loop(parser);

    if (dpl_parse_peek_token(parser).kind == TOKEN_COLON_EQUAL)
    {
        if (target->kind == AST_NODE_FIELD_ACCESS)
        {
            DPL_AST_ERROR(parser->lexer->source, target, "Object fields cannot be assigned directly. Instead, you can compose a new object from the old one.");
        }
        else if (target->kind != AST_NODE_SYMBOL)
        {
            DPL_AST_ERROR(parser->lexer->source, target, "`%s` is not a valid assignment target.",
                          dpl_parse_nodekind_name(target->kind));
        }

        DPL_Token assignment = dpl_parse_next_token(parser);
        DPL_Ast_Node *expression = dpl_parse_assignment(parser);

        DPL_Ast_Node *node = dpl_parse_allocate_node(parser, AST_NODE_ASSIGNMENT, target->first, expression->last);
        node->as.assignment.target = target;
        node->as.assignment.assignment = assignment;
        node->as.assignment.expression = expression;
        return node;
    }

    return target;
}

DPL_Ast_Node *dpl_parse_expression(DPL_Parser *parser)
{
    return dpl_parse_assignment(parser);
}

DPL_Ast_Node *dpl_parse_scope(DPL_Parser *parser, DPL_Token opening_token, DPL_TokenKind closing_token_kind)
{
    DPL_Token closing_token_candidate = dpl_parse_peek_token(parser);
    if (closing_token_candidate.kind == closing_token_kind)
    {
        DPL_TOKEN_ERROR(parser->lexer->source, closing_token_candidate,
                        "Unexpected %s. Expected at least one expression in scope.",
                        dpl_lexer_token_kind_name(closing_token_kind));
    }

    da_array(DPL_Ast_Node *) expressions = dpl_parse_expressions(parser, TOKEN_SEMICOLON, closing_token_kind, true);
    if (opening_token.kind == TOKEN_NONE)
    {
        opening_token = expressions[0]->first;
    }

    DPL_Token closing_token = dpl_parse_expect_token(parser, closing_token_kind);

    DPL_Ast_Node *node = dpl_parse_allocate_node(parser, AST_NODE_SCOPE, opening_token, closing_token);
    dpl_parse_move_nodelist(parser, expressions, &node->as.scope.expression_count, &node->as.scope.expressions);
    return node;
}

DPL_Ast_Node *dpl_parse(DPL_Parser *parser)
{
    return dpl_parse_scope(parser, parser->lexer->first_token, TOKEN_EOF);
}
