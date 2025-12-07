#include <error.h>
#include <nobx.h>

#include <dpl/parser.h>

const char* AST_NODE_KIND_NAMES[COUNT_AST_NODE_KINDS] = {

    [AST_NODE_LITERAL] = "AST_NODE_LITERAL",
    [AST_NODE_OBJECT_LITERAL] = "AST_NODE_OBJECT_LITERAL",
    [AST_NODE_ARRAY_LITERAL] = "AST_NODE_ARRAY_LITERAL",
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
    [AST_NODE_FOR_LOOP] = "AST_NODE_FOR_LOOP",
    [AST_NODE_FIELD_ACCESS] = "AST_NODE_FIELD_ACCESS",
    [AST_NODE_INTERPOLATION] = "AST_NODE_INTERPOLATION",
};

static_assert(COUNT_AST_NODE_KINDS == 16,
              "Count of ast node kinds has changed, please update ast node kind names map.");

const char* dpl_parse_nodekind_name(DPL_AstNodeKind kind)
{
    if (kind < 0 || kind >= COUNT_AST_NODE_KINDS)
    {
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
    return AST_NODE_KIND_NAMES[kind];
}

// AST

DPL_Ast_Node* dpl_parse_allocate_node(DPL_Parser* parser, DPL_AstNodeKind kind, DPL_Token first, DPL_Token last)
{
    DPL_Ast_Node* node = arena_alloc(parser->memory, sizeof(DPL_Ast_Node));
    node->kind = kind;
    node->first = first;
    node->last = last;
    return node;
}

void dpl_parse_move_nodelist(DPL_Parser* parser, DPL_Ast_Nodes list, size_t* target_count, DPL_Ast_Node*** target_items)
{
    *target_count = list.count;
    if (list.count > 0)
    {
        *target_items = arena_alloc(parser->memory, sizeof(DPL_Ast_Node*) * list.count);
        memcpy(*target_items, list.items, sizeof(DPL_Ast_Node*) * list.count);
        nob_da_free(list);
    }
}

void dpl_parse_build_type_name(DPL_Ast_Type* ast_type, Nob_String_Builder* sb)
{
    switch (ast_type->kind)
    {
    case TYPE_BASE:
        nob_sb_append_sv(sb, ast_type->as.name.text);
        break;
    case TYPE_OBJECT:
        {
            nob_sb_append_cstr(sb, "$[");
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
    case TYPE_ARRAY:
        {
            nob_sb_append_cstr(sb, "[");
            dpl_parse_build_type_name(ast_type->as.array.element_type, sb);
            nob_sb_append_cstr(sb, "]");
        }
        break;
    default:
        DW_UNIMPLEMENTED_MSG("Cannot print ast type %d.", ast_type->kind);
    }
}

const char* dpl_parse_type_name(DPL_Ast_Type* ast_type)
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

void dpl_parse_print(DPL_Ast_Node* node, size_t level)
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
            printf(" [%s: " SV_Fmt "]\n", dpl_lexer_token_kind_name(node->as.literal.value.kind),
                   SV_Arg(node->as.literal.value.text));
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
    case AST_NODE_ARRAY_LITERAL:
        {
            DPL_Ast_ArrayLiteral array_literal = node->as.array_literal;
            printf("\n");
            for (size_t i = 0; i < array_literal.element_count; ++i)
            {
                dpl_parse_print_indent(level + 1);
                printf("<element #%zu>\n", i);
                dpl_parse_print(array_literal.elements[i], level + 2);
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
            printf(" [%s " SV_Fmt ": %s]\n", dpl_lexer_token_kind_name(declaration.keyword.kind),
                   SV_Arg(declaration.name.text), dpl_parse_type_name(declaration.type));
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
    case AST_NODE_FOR_LOOP:
        {
            DPL_Ast_ForLoop for_loop = node->as.for_loop;
            printf("\n");

            dpl_parse_print_indent(level + 1);
            printf("<variable_name: " SV_Fmt ">\n", SV_Arg(for_loop.variable_name.text));

            dpl_parse_print_indent(level + 1);
            printf("<iterator_initializer>\n");
            dpl_parse_print(for_loop.iterator_initializer, level + 2);

            dpl_parse_print_indent(level + 1);
            printf("<body>\n");
            dpl_parse_print(for_loop.body, level + 2);
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

// ALLOCATION

DPL_Ast_Node* dpl_parse_allocate_literal(DPL_Parser* parser, DPL_Token token)
{
    DPL_Ast_Node* node = dpl_parse_allocate_node(parser, AST_NODE_LITERAL, token, token);
    node->as.literal.value = token;
    return node;
}

// PARSER

void dpl_parse_skip_whitespace(DPL_Parser* parser)
{
    while (dpl_lexer_peek_token(parser->lexer).kind == TOKEN_WHITESPACE || dpl_lexer_peek_token(parser->lexer).kind ==
        TOKEN_COMMENT)
    {
        dpl_lexer_next_token(parser->lexer);
    }
}

DPL_Token dpl_parse_next_token(DPL_Parser* parser)
{
    dpl_parse_skip_whitespace(parser);
    return dpl_lexer_next_token(parser->lexer);
}

DPL_Token dpl_parse_peek_token(DPL_Parser* parser)
{
    dpl_parse_skip_whitespace(parser);
    return dpl_lexer_peek_token(parser->lexer);
}

void dpl_parse_check_token(DPL_Parser* parser, DPL_Token token, DPL_TokenKind kind)
{
    if (token.kind != kind)
    {
        DPL_TOKEN_ERROR(parser->lexer->source, token, "Unexpected %s, expected %s.",
                        dpl_lexer_token_kind_name(token.kind), dpl_lexer_token_kind_name(kind));
    }
}

DPL_Token dpl_parse_expect_token(DPL_Parser* parser, DPL_TokenKind kind)
{
    DPL_Token next_token = dpl_parse_next_token(parser);
    dpl_parse_check_token(parser, next_token, kind);
    return next_token;
}

// TYPES

int dpl_parse_compare_object_type_fields(void const* a, void const* b)
{
    return nob_sv_cmp(((DPL_Ast_TypeField*)a)->name.text, ((DPL_Ast_TypeField*)b)->name.text);
}

DPL_Ast_Type* dpl_parse_type(DPL_Parser* parser)
{
    DPL_Token type_begin = dpl_parse_peek_token(parser);
    switch (type_begin.kind)
    {
    case TOKEN_IDENTIFIER:
        {
            dpl_parse_next_token(parser);

            DPL_Ast_Type* name_type = arena_alloc(parser->memory, sizeof(DPL_Ast_Type));
            name_type->kind = TYPE_BASE;
            name_type->first = type_begin;
            name_type->last = type_begin;
            name_type->as.name = type_begin;
            return name_type;
        }
    case TOKEN_OPEN_DOLLAR_BRACKET:
        {
            dpl_parse_next_token(parser);

            DPL_Ast_TypeFields tmp_fields = {0};
            while (dpl_parse_peek_token(parser).kind != TOKEN_CLOSE_BRACKET)
            {
                if (tmp_fields.count > 0)
                {
                    dpl_parse_expect_token(parser, TOKEN_COMMA);
                }
                DPL_Token name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);
                dpl_parse_expect_token(parser, TOKEN_COLON);
                DPL_Ast_Type* type = dpl_parse_type(parser);
                DPL_Ast_TypeField field = {
                    .name = name,
                    .first = name,
                    .last = type->last,
                    .type = type,
                };

                for (size_t i = 0; i < tmp_fields.count; ++i)
                {
                    if (nob_sv_eq(tmp_fields.items[i].name.text, name.text))
                    {
                        DPL_AST_ERROR_WITH_NOTE(parser->lexer->source,
                                                &tmp_fields.items[i], "Previous declaration was here.",
                                                &field, "Duplicate field `" SV_Fmt "` in object type.",
                                                SV_Arg(name.text));
                    }
                }
                nob_da_append(&tmp_fields, field);
            }
            qsort(tmp_fields.items, tmp_fields.count, sizeof(*tmp_fields.items), dpl_parse_compare_object_type_fields);

            DPL_Ast_TypeField* fields = NULL;
            if (tmp_fields.count > 0)
            {
                fields = arena_alloc(parser->memory, sizeof(DPL_Ast_TypeField) * tmp_fields.count);
                memcpy(fields, tmp_fields.items, sizeof(DPL_Ast_TypeField) * tmp_fields.count);
            }

            DPL_Ast_Type* object_type = arena_alloc(parser->memory, sizeof(DPL_Ast_Type));
            object_type->kind = TYPE_OBJECT;
            object_type->first = type_begin;
            object_type->last = dpl_parse_peek_token(parser);
            object_type->as.object.field_count = tmp_fields.count;
            object_type->as.object.fields = fields;

            nob_da_free(tmp_fields);
            dpl_parse_next_token(parser);

            return object_type;
        }
    case TOKEN_OPEN_BRACKET:
        {
            dpl_parse_next_token(parser);

            DPL_Ast_Type* element_type = dpl_parse_type(parser);
            DPL_Token close_bracket = dpl_parse_expect_token(parser, TOKEN_CLOSE_BRACKET);

            DPL_Ast_Type* object_type = arena_alloc(parser->memory, sizeof(DPL_Ast_Type));
            object_type->kind = TYPE_ARRAY;
            object_type->first = type_begin;
            object_type->last = close_bracket;
            object_type->as.array.element_type = element_type;

            return object_type;
        }
    default:
        DPL_TOKEN_ERROR(parser->lexer->source, type_begin, "Invalid %s in type reference, expected %s.",
                        dpl_lexer_token_kind_name(type_begin.kind),
                        dpl_lexer_token_kind_name(TOKEN_IDENTIFIER));
    }
}

// RULES

typedef enum
{
    DPL_PARSER_PREC_NONE,
    DPL_PARSER_PREC_DECLARATION, // var / const / function
    DPL_PARSER_PREC_ASSIGNMENT, // :=
    DPL_PARSER_PREC_OR, // or
    DPL_PARSER_PREC_AND, // and
    DPL_PARSER_PREC_EQUALITY, // == !=
    DPL_PARSER_PREC_COMPARISON, // < > <= >=
    DPL_PARSER_PREC_ADDITIVE, // + -
    DPL_PARSER_PREC_MULTIPLICATIVE, // * /
    DPL_PARSER_PREC_RANGE, // ..
    DPL_PARSER_PREC_UNARY, // ! -
    DPL_PARSER_PREC_CALL, // . ()
    DPL_PARSER_PREC_PRIMARY
} DPL_Parser_Precedence;

typedef DPL_Ast_Node* (*DPL_Parser_Prefix_Callback)(DPL_Parser* parser);
typedef DPL_Ast_Node* (*DPL_Parser_Infix_Callback)(DPL_Parser* parser, DPL_Ast_Node*);

typedef struct
{
    DPL_Parser_Prefix_Callback prefix;
    DPL_Parser_Infix_Callback infix;
    DPL_Parser_Precedence precedence;
} DPL_Parser_Rule;

static DPL_Parser_Rule* dpl_parse_get_rule(const DPL_TokenKind kind);

DPL_Ast_Nodes dpl_parse_expressions(DPL_Parser* parser, DPL_TokenKind delimiter, DPL_TokenKind closing,
                                    const DPL_Parser_Precedence precedence);
DPL_Ast_Node* dpl_parse_scope_internal(DPL_Parser* parser, DPL_Token opening_token, DPL_TokenKind closing_token_kind);

static DPL_Ast_Node* dpl_parse_precedence(DPL_Parser* parser, const DPL_Parser_Precedence precedence)
{
    const DPL_Token token = dpl_parse_peek_token(parser);

    const DPL_Parser_Rule* rule = dpl_parse_get_rule(token.kind);
    if (rule->prefix == NULL || (rule->precedence != DPL_PARSER_PREC_NONE && rule->precedence < precedence))
    {
        DPL_TOKEN_ERROR(parser->lexer->source, token,
                        "Expected expression, got %s.", dpl_lexer_token_kind_name(token.kind));
    }

    DPL_Ast_Node* result = rule->prefix(parser);

    while (precedence <= dpl_parse_get_rule(dpl_parse_peek_token(parser).kind)->precedence)
    {
        const DPL_Parser_Infix_Callback infix_rule = dpl_parse_get_rule(dpl_parse_peek_token(parser).kind)->infix;
        result = infix_rule(parser, result);
    }

    return result;
}

static DPL_Ast_Node* dpl_parse_literal(DPL_Parser* parser)
{
    const DPL_Token token = dpl_parse_next_token(parser);
    return dpl_parse_allocate_literal(parser, token);
}

static DPL_Ast_Node* dpl_parse_string_interpolation(DPL_Parser* parser)
{
    DPL_Ast_Nodes tmp_parts = {0};

    DPL_Token token = dpl_parse_next_token(parser);
    while (token.kind == TOKEN_STRING_INTERPOLATION)
    {
        if (token.text.count > 3)
        {
            nob_da_append(&tmp_parts, dpl_parse_allocate_literal(parser, token));
        }

        DPL_Ast_Node* expression = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);
        nob_da_append(&tmp_parts, expression);

        token = dpl_parse_next_token(parser);
    }

    dpl_parse_check_token(parser, token, TOKEN_STRING);
    if (token.text.count > 2)
    {
        nob_da_append(&tmp_parts, dpl_parse_allocate_literal(parser, token));
    }

    DPL_Ast_Node* node = dpl_parse_allocate_node(parser, AST_NODE_INTERPOLATION,
                                                 tmp_parts.items[0]->first, tmp_parts.items[tmp_parts.count - 1]->last);
    dpl_parse_move_nodelist(parser, tmp_parts, &node->as.interpolation.expression_count,
                            &node->as.interpolation.expressions);

    return node;
}

static DPL_Ast_Node* dpl_parse_unary(DPL_Parser* parser)
{
    const DPL_Token operator = dpl_parse_next_token(parser);
    DPL_Ast_Node* operand = dpl_parse_precedence(parser, DPL_PARSER_PREC_UNARY);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_UNARY, operator, operand->last);
    result->as.unary.operator = operator;
    result->as.unary.operand = operand;
    return result;
}

static DPL_Ast_Node* dpl_parse_grouping(DPL_Parser* parser)
{
    const DPL_Token open_paren = dpl_parse_next_token(parser);
    DPL_Ast_Node* result = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);
    result->first = open_paren;
    result->last = dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);
    return result;
}

static DPL_Ast_Node* dpl_parse_binary(DPL_Parser* parser, DPL_Ast_Node* lhs)
{
    const DPL_Token operator = dpl_parse_next_token(parser);

    const DPL_Parser_Rule* rule = dpl_parse_get_rule(operator.kind);
    DPL_Ast_Node* rhs = dpl_parse_precedence(parser, rule->precedence + 1);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_BINARY, lhs->first, rhs->last);
    result->as.binary.left = lhs;
    result->as.binary.operator = operator;
    result->as.binary.right = rhs;
    return result;
}

static DPL_Ast_Node* dpl_parse_assignment(DPL_Parser* parser, DPL_Ast_Node* lhs)
{
    const DPL_Token assignment = dpl_parse_next_token(parser);

    if (lhs->kind == AST_NODE_FIELD_ACCESS)
    {
        DPL_AST_ERROR(parser->lexer->source, lhs,
                      "Object fields cannot be assigned directly. Instead, you can compose a new object from the old one.")
        ;
    }

    if (lhs->kind != AST_NODE_SYMBOL)
    {
        DPL_AST_ERROR(parser->lexer->source, lhs, "`%s` is not a valid assignment target.",
                      dpl_parse_nodekind_name(lhs->kind));
    }

    DPL_Ast_Node* rhs = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_ASSIGNMENT, lhs->first, rhs->last);
    result->as.assignment.target = lhs;
    result->as.assignment.assignment = assignment;
    result->as.assignment.expression = rhs;
    return result;
}

static DPL_Ast_Node* dpl_parse_scope(DPL_Parser* parser)
{
    const DPL_Token open_brace = dpl_parse_next_token(parser);
    return dpl_parse_scope_internal(parser, open_brace, TOKEN_CLOSE_BRACE);
}

static DPL_Ast_Node* dpl_parse_identifier(DPL_Parser* parser)
{
    const DPL_Token identifier = dpl_parse_next_token(parser);
    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_SYMBOL, identifier, identifier);
    result->as.symbol = identifier;
    return result;
}

static DPL_Ast_Node* dpl_parse_function_call(DPL_Parser* parser, DPL_Ast_Node* lhs)
{
    if (lhs->kind != AST_NODE_SYMBOL)
    {
        DPL_AST_ERROR(parser->lexer->source, lhs, "Only symbols can be called as functions.");
    }
    const DPL_Token name = lhs->as.symbol;

    dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);

    DPL_Ast_Nodes arguments = {0};
    if (dpl_parse_peek_token(parser).kind != TOKEN_CLOSE_PAREN)
    {
        arguments = dpl_parse_expressions(parser, TOKEN_COMMA, TOKEN_CLOSE_PAREN, DPL_PARSER_PREC_ASSIGNMENT);
    }

    const DPL_Token closing_paren = dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);

    DPL_Ast_Node* node = dpl_parse_allocate_node(parser, AST_NODE_FUNCTIONCALL, name, closing_paren);
    node->as.function_call.name = name;
    dpl_parse_move_nodelist(parser, arguments, &node->as.function_call.argument_count,
                            &node->as.function_call.arguments);
    return node;
}

static DPL_Ast_Node* dpl_parse_object_literal(DPL_Parser* parser)
{
    const DPL_Token open_dollar_bracket = dpl_parse_next_token(parser);

    const DPL_Ast_Nodes fields = dpl_parse_expressions(parser, TOKEN_COMMA, TOKEN_CLOSE_BRACKET,
                                                       DPL_PARSER_PREC_ASSIGNMENT);

    DPL_Ast_Node* object_literal = dpl_parse_allocate_node(parser, AST_NODE_OBJECT_LITERAL, open_dollar_bracket,
                                                           dpl_parse_next_token(parser));
    dpl_parse_move_nodelist(parser, fields, &object_literal->as.object_literal.field_count,
                            &object_literal->as.object_literal.fields);

    return object_literal;
}

static DPL_Ast_Node* dpl_parse_array_literal(DPL_Parser* parser)
{
    const DPL_Token open_bracket = dpl_parse_next_token(parser);

    DPL_Ast_Nodes elements = {0};
    if (dpl_parse_peek_token(parser).kind != TOKEN_CLOSE_BRACKET)
    {
        elements = dpl_parse_expressions(parser, TOKEN_COMMA, TOKEN_CLOSE_BRACKET, DPL_PARSER_PREC_ASSIGNMENT);
    }

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_ARRAY_LITERAL, open_bracket,
                                                          dpl_parse_next_token(parser));
    dpl_parse_move_nodelist(parser, elements, &result->as.array_literal.element_count,
                            &result->as.array_literal.elements);
    return result;
}

DPL_Ast_Node* dpl_parse_dot_access(DPL_Parser* parser, DPL_Ast_Node* lhs)
{
    dpl_parse_next_token(parser);
    DPL_Ast_Node* rhs = dpl_parse_precedence(parser, DPL_PARSER_PREC_CALL);

    if (rhs->kind == AST_NODE_SYMBOL)
    {
        DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_FIELD_ACCESS, lhs->first, rhs->last);
        result->as.field_access.expression = lhs;
        result->as.field_access.field = rhs;
        return result;
    }

    if (rhs->kind == AST_NODE_FUNCTIONCALL)
    {
        DPL_Ast_FunctionCall* fc = &rhs->as.function_call;
        fc->arguments = arena_realloc(parser->memory, fc->arguments,
                                      fc->argument_count * sizeof(DPL_Ast_Node*),
                                      (fc->argument_count + 1) * sizeof(DPL_Ast_Node*));
        fc->argument_count++;

        for (size_t i = fc->argument_count - 1; i > 0; --i)
        {
            fc->arguments[i] = fc->arguments[i - 1];
        }

        fc->arguments[0] = lhs;
        return rhs;
    }

    DPL_AST_ERROR(parser->lexer->source, rhs,
                  "Right-hand operand of operator `.` must be either a symbol or a function call.");
}

DPL_Ast_Node* dpl_parse_element_access(DPL_Parser* parser, DPL_Ast_Node* lhs)
{
    const DPL_Token operator = dpl_parse_next_token(parser);

    DPL_Ast_Node* rhs = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);
    const DPL_Token closing_bracket = dpl_parse_expect_token(parser, TOKEN_CLOSE_BRACKET);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_BINARY, lhs->first, closing_bracket);
    result->as.binary.operator = operator;
    result->as.binary.left = lhs;
    result->as.binary.right = rhs;
    return result;
}

DPL_Ast_Node* dpl_parse_if(DPL_Parser* parser)
{
    const DPL_Token keyword = dpl_parse_next_token(parser);

    dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);
    DPL_Ast_Node* condition = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);
    dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);
    DPL_Ast_Node* then_clause = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);
    dpl_parse_expect_token(parser, TOKEN_KEYWORD_ELSE);
    DPL_Ast_Node* else_clause = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_CONDITIONAL, keyword, else_clause->last);
    result->as.conditional.condition = condition;
    result->as.conditional.then_clause = then_clause;
    result->as.conditional.else_clause = else_clause;
    return result;
}

DPL_Ast_Node* dpl_parse_while(DPL_Parser* parser)
{
    const DPL_Token keyword = dpl_parse_next_token(parser);

    dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);
    DPL_Ast_Node* condition = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);
    dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);
    DPL_Ast_Node* body = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_WHILE_LOOP, keyword, body->last);
    result->as.while_loop.condition = condition;
    result->as.while_loop.body = body;
    return result;
}

DPL_Ast_Node* dpl_parse_for(DPL_Parser* parser)
{
    const DPL_Token keyword = dpl_parse_next_token(parser);

    dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);
    dpl_parse_expect_token(parser, TOKEN_KEYWORD_VAR);
    DPL_Token variable_name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);
    dpl_parse_expect_token(parser, TOKEN_KEYWORD_IN);
    DPL_Ast_Node* iterator_initializer = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);
    dpl_parse_expect_token(parser, TOKEN_CLOSE_PAREN);
    DPL_Ast_Node* body = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_FOR_LOOP, keyword, body->last);
    result->as.for_loop.variable_name = variable_name;
    result->as.for_loop.iterator_initializer = iterator_initializer;
    result->as.for_loop.body = body;
    return result;
}

DPL_Ast_Node* dpl_parse_var_const_declaration(DPL_Parser* parser)
{
    const DPL_Token keyword = dpl_parse_next_token(parser);
    const DPL_Token name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);

    DPL_Ast_Type* type = NULL;
    if (dpl_parse_peek_token(parser).kind == TOKEN_COLON)
    {
        dpl_parse_expect_token(parser, TOKEN_COLON);
        type = dpl_parse_type(parser);
    }

    const DPL_Token assignment = dpl_parse_expect_token(parser, TOKEN_COLON_EQUAL);

    DPL_Ast_Node* initialization = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_DECLARATION, keyword, initialization->last);
    result->as.declaration.keyword = keyword;
    result->as.declaration.name = name;
    result->as.declaration.type = type;
    result->as.declaration.assignment = assignment;
    result->as.declaration.initialization = initialization;
    return result;
}

DPL_Ast_Node* dpl_parse_function_declaration(DPL_Parser* parser)
{
    const DPL_Token keyword = dpl_parse_next_token(parser);
    const DPL_Token name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);

    DPL_Ast_FunctionArguments arguments = {0};
    dpl_parse_expect_token(parser, TOKEN_OPEN_PAREN);
    if (dpl_parse_peek_token(parser).kind != TOKEN_CLOSE_PAREN)
    {
        while (true)
        {
            DPL_Ast_FunctionArgument argument = {0};
            argument.name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);
            dpl_parse_expect_token(parser, TOKEN_COLON);
            argument.type = dpl_parse_type(parser);

            nob_da_append(&arguments, argument);

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

    DPL_Ast_Type* result_type = NULL;
    if (dpl_parse_peek_token(parser).kind == TOKEN_COLON)
    {
        dpl_parse_expect_token(parser, TOKEN_COLON);
        result_type = dpl_parse_type(parser);
    }

    dpl_parse_expect_token(parser, TOKEN_COLON_EQUAL);

    DPL_Ast_Node* body = dpl_parse_precedence(parser, DPL_PARSER_PREC_ASSIGNMENT);

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_FUNCTION, keyword, body->last);
    result->as.function.keyword = keyword;
    result->as.function.name = name;

    result->as.function.signature.argument_count = arguments.count;
    if (arguments.count > 0)
    {
        result->as.function.signature.arguments = arena_alloc(parser->memory,
                                                              sizeof(DPL_Ast_FunctionArgument) * arguments.count);
        memcpy(result->as.function.signature.arguments, arguments.items,
               sizeof(DPL_Ast_FunctionArgument) * arguments.count);
        nob_da_free(arguments);
    }
    result->as.function.signature.type = result_type;

    result->as.function.body = body;

    return result;
}

DPL_Ast_Node* dpl_parse_type_declaration(DPL_Parser* parser)
{
    const DPL_Token keyword = dpl_parse_next_token(parser);
    const DPL_Token name = dpl_parse_expect_token(parser, TOKEN_IDENTIFIER);
    const DPL_Token assignment = dpl_parse_expect_token(parser, TOKEN_COLON_EQUAL);

    DPL_Ast_Type* type = dpl_parse_type(parser);
    if (dpl_parse_peek_token(parser).kind == TOKEN_COLON)
    {
        dpl_parse_expect_token(parser, TOKEN_COLON);
        type = dpl_parse_type(parser);
    }

    DPL_Ast_Node* result = dpl_parse_allocate_node(parser, AST_NODE_DECLARATION, keyword, type->last);
    result->as.declaration.keyword = keyword;
    result->as.declaration.name = name;
    result->as.declaration.type = type;
    result->as.declaration.assignment = assignment;
    result->as.declaration.initialization = NULL;
    return result;
}

static DPL_Parser_Rule DPL_PARSER_RULES[COUNT_TOKEN_KINDS] = {
    [TOKEN_NONE] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_WHITESPACE] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_COMMENT] = {NULL, NULL, DPL_PARSER_PREC_NONE},

    [TOKEN_PLUS] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_ADDITIVE},
    [TOKEN_MINUS] = {dpl_parse_unary, dpl_parse_binary, DPL_PARSER_PREC_ADDITIVE},
    [TOKEN_STAR] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_MULTIPLICATIVE},
    [TOKEN_SLASH] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_ADDITIVE},

    [TOKEN_LESS] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_COMPARISON},
    [TOKEN_GREATER] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_COMPARISON},
    [TOKEN_EQUAL_EQUAL] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_EQUALITY},
    [TOKEN_BANG] = {dpl_parse_unary, NULL, DPL_PARSER_PREC_UNARY},
    [TOKEN_BANG_EQUAL] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_EQUALITY},
    [TOKEN_AND_AND] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_AND},
    [TOKEN_PIPE_PIPE] = {NULL, dpl_parse_binary, DPL_PARSER_PREC_OR},

    [TOKEN_DOT] = {NULL, dpl_parse_dot_access, DPL_PARSER_PREC_CALL},
    [TOKEN_DOT_DOT] = {dpl_parse_unary, dpl_parse_binary, DPL_PARSER_PREC_RANGE},
    [TOKEN_COLON] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_COLON_EQUAL] = {NULL, dpl_parse_assignment, DPL_PARSER_PREC_ASSIGNMENT},

    [TOKEN_OPEN_PAREN] = {dpl_parse_grouping, dpl_parse_function_call, DPL_PARSER_PREC_CALL},
    [TOKEN_CLOSE_PAREN] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_OPEN_BRACE] = {dpl_parse_scope, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_CLOSE_BRACE] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_OPEN_BRACKET] = {dpl_parse_array_literal, dpl_parse_element_access, DPL_PARSER_PREC_CALL},
    [TOKEN_CLOSE_BRACKET] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_OPEN_DOLLAR_BRACKET] = {dpl_parse_object_literal, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_SEMICOLON] = {NULL, NULL, DPL_PARSER_PREC_NONE},

    [TOKEN_NUMBER] = {dpl_parse_literal, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_IDENTIFIER] = {dpl_parse_identifier, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_STRING] = {dpl_parse_literal, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_STRING_INTERPOLATION] = {dpl_parse_string_interpolation, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_TRUE] = {dpl_parse_literal, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_FALSE] = {dpl_parse_literal, NULL, DPL_PARSER_PREC_NONE},

    [TOKEN_KEYWORD_CONSTANT] = {dpl_parse_var_const_declaration, NULL, DPL_PARSER_PREC_DECLARATION},
    [TOKEN_KEYWORD_FUNCTION] = {dpl_parse_function_declaration, NULL, DPL_PARSER_PREC_DECLARATION},
    [TOKEN_KEYWORD_VAR] = {dpl_parse_var_const_declaration, NULL, DPL_PARSER_PREC_DECLARATION},
    [TOKEN_KEYWORD_IF] = {dpl_parse_if, NULL, DPL_PARSER_PREC_ASSIGNMENT},
    [TOKEN_KEYWORD_ELSE] = {NULL, NULL, DPL_PARSER_PREC_NONE},
    [TOKEN_KEYWORD_WHILE] = {dpl_parse_while, NULL, DPL_PARSER_PREC_ASSIGNMENT},
    [TOKEN_KEYWORD_TYPE] = {dpl_parse_type_declaration, NULL, DPL_PARSER_PREC_DECLARATION},
    [TOKEN_KEYWORD_FOR] = {dpl_parse_for, NULL, DPL_PARSER_PREC_ASSIGNMENT},
    [TOKEN_KEYWORD_IN] = {NULL, NULL, DPL_PARSER_PREC_NONE},
};

static_assert(COUNT_TOKEN_KINDS == 45,
              "Count of ast node kinds has changed, please update ast node kind names map.");

static DPL_Parser_Rule* dpl_parse_get_rule(const DPL_TokenKind kind)
{
    return &DPL_PARSER_RULES[kind];
}

DPL_Ast_Nodes dpl_parse_expressions(DPL_Parser* parser, DPL_TokenKind delimiter, DPL_TokenKind closing,
                                    const DPL_Parser_Precedence precedence)
{
    DPL_Ast_Nodes list = {0};
    nob_da_append(&list, dpl_parse_precedence(parser, precedence));

    DPL_Token delimiter_candidate = dpl_parse_peek_token(parser);
    while (delimiter_candidate.kind == delimiter)
    {
        dpl_parse_next_token(parser);
        if (dpl_parse_peek_token(parser).kind == closing)
        {
            break;
        }

        nob_da_append(&list, dpl_parse_precedence(parser, precedence));
        delimiter_candidate = dpl_parse_peek_token(parser);
    }

    return list;
}

DPL_Ast_Node* dpl_parse_scope_internal(DPL_Parser* parser, DPL_Token opening_token, const DPL_TokenKind closing_token_kind)
{
    const DPL_Token closing_token_candidate = dpl_parse_peek_token(parser);
    if (closing_token_candidate.kind == closing_token_kind)
    {
        DPL_TOKEN_ERROR(parser->lexer->source, closing_token_candidate,
                        "Unexpected %s. Expected at least one expression in scope.",
                        dpl_lexer_token_kind_name(closing_token_kind));
    }

    const DPL_Ast_Nodes expressions = dpl_parse_expressions(parser, TOKEN_SEMICOLON, closing_token_kind,
                                                      DPL_PARSER_PREC_DECLARATION);
    if (opening_token.kind == TOKEN_NONE)
    {
        opening_token = expressions.items[0]->first;
    }

    const DPL_Token closing_token = dpl_parse_expect_token(parser, closing_token_kind);

    DPL_Ast_Node* node = dpl_parse_allocate_node(parser, AST_NODE_SCOPE, opening_token, closing_token);
    dpl_parse_move_nodelist(parser, expressions, &node->as.scope.expression_count, &node->as.scope.expressions);
    return node;
}

DPL_Ast_Node* dpl_parse(DPL_Parser* parser)
{
    return dpl_parse_scope_internal(parser, parser->lexer->first_token, TOKEN_EOF);
}
