#include <assert.h>

#include <arena.h>
#include <dw_array.h>
#include <error.h>

#include <dpl/binding.h>
#include <dpl/lexer.h>

// UTILS

const char *BOUND_NODE_KIND_NAMES[COUNT_BOUND_NODE_KINDS] = {
    [BOUND_NODE_VALUE] = "BOUND_NODE_VALUE",
    [BOUND_NODE_OBJECT] = "BOUND_NODE_OBJECT",
    [BOUND_NODE_FUNCTIONCALL] = "BOUND_NODE_FUNCTIONCALL",
    [BOUND_NODE_SCOPE] = "BOUND_NODE_SCOPE",
    [BOUND_NODE_VARREF] = "BOUND_NODE_VARREF",
    [BOUND_NODE_ARGREF] = "BOUND_NODE_ARGREF",
    [BOUND_NODE_ASSIGNMENT] = "BOUND_NODE_ASSIGNMENT",
    [BOUND_NODE_CONDITIONAL] = "BOUND_NODE_CONDITIONAL",
    [BOUND_NODE_LOGICAL_OPERATOR] = "BOUND_NODE_LOGICAL_OPERATOR",
    [BOUND_NODE_WHILE_LOOP] = "BOUND_NODE_WHILE_LOOP",
    [BOUND_NODE_LOAD_FIELD] = "BOUND_NODE_LOAD_FIELD",
    [BOUND_NODE_INTERPOLATION] = "BOUND_NODE_INTERPOLATION",
};

static_assert(COUNT_BOUND_NODE_KINDS == 12,
              "Count of bound node kinds has changed, please update bound node kind names map.");

const char *dpl_bind_nodekind_name(DPL_BoundNodeKind kind)
{
    if (kind < 0 || kind >= COUNT_BOUND_NODE_KINDS)
    {
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
    return BOUND_NODE_KIND_NAMES[kind];
}

static DPL_Symbol *dpl_bind_resolve_type_alias(DPL_Symbol *type)
{
    while (type && type->as.type.kind == TYPE_ALIAS)
    {
        type = type->as.type.as.alias;
    }
    return type;
}

static bool dpl_bind_type_assignable(DPL_Symbol *from, DPL_Symbol *to)
{
    return dpl_bind_resolve_type_alias(from) == dpl_bind_resolve_type_alias(to);
}

static void dpl_bind_build_type_name(DPL_Ast_Type *ast_type, Nob_String_Builder *sb)
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
            dpl_bind_build_type_name(field.type, sb);
        }
        nob_sb_append_cstr(sb, "]");
    }
    break;
    default:
        DW_UNIMPLEMENTED_MSG("Cannot print ast type %d.", ast_type->kind);
    }
}

static const char *dpl_bind_type_name(DPL_Ast_Type *ast_type)
{
    static char result[256];
    if (!ast_type)
    {
        return dpl_lexer_token_kind_name(TOKEN_NONE);
    }
    Nob_String_Builder sb = {0};
    dpl_bind_build_type_name(ast_type, &sb);
    nob_sb_append_null(&sb);

    strncpy(result, sb.items, NOB_ARRAY_LEN(result));
    return result;
}

static DPL_Symbol *dpl_bind_type(DPL_Binding *binding, DPL_Ast_Type *ast_type)
{
    const char *type_name = dpl_bind_type_name(ast_type);

    DPL_Symbol *s = dpl_symbols_find_kind_cstr(binding->symbols, type_name, SYMBOL_TYPE);
    if (s)
    {
        return s;
    }

    DPL_Symbol *cached_type = dpl_symbols_find_cstr(binding->symbols, type_name);
    if (cached_type)
    {
        return cached_type;
    }

    if (ast_type->kind == TYPE_OBJECT)
    {
        DPL_Ast_TypeObject object_type = ast_type->as.object;

        DPL_Symbol *new_object_type = dpl_symbols_push_type_object_cstr(binding->symbols, type_name, object_type.field_count);
        DPL_Symbol_Type_ObjectField *fields = new_object_type->as.type.as.object.fields;

        for (size_t i = 0; i < object_type.field_count; ++i)
        {
            DPL_Ast_TypeField ast_field = object_type.fields[i];
            DPL_Symbol *bound_field_type = dpl_bind_type(binding, ast_field.type);
            if (!bound_field_type)
            {
                DPL_AST_ERROR(binding->source, ast_field.type, "Unable to bind type `%s` for field `" SV_Fmt "`.",
                              dpl_bind_type_name(ast_field.type), SV_Arg(ast_field.name.text));
            }
            fields[i].name = ast_field.name.text;
            fields[i].type = bound_field_type;
        }

        return new_object_type;
    }

    return NULL;
}

static void dpl_bind_check_assignment(DPL_Binding *binding, const char *what, DPL_Ast_Node *node, DPL_Symbol *expression_type)
{
    DPL_Ast_Declaration *decl = &node->as.declaration;
    if (dpl_symbols_is_type_base(expression_type, TYPE_BASE_NONE))
    {
        DPL_AST_ERROR(binding->source, decl->initialization,
                      "Expressions of type `" SV_Fmt "` cannot be assigned.", SV_Arg(expression_type->name));
    }

    if (decl->type)
    {
        DPL_Symbol *declared_type = dpl_bind_type(binding, decl->type);
        if (!declared_type)
        {
            DPL_AST_ERROR(binding->source, decl->type,
                          "Unknown type `%s` in declaration of %s `" SV_Fmt "`.",
                          dpl_bind_type_name(decl->type), what, SV_Arg(decl->name.text));
        }

        if (!dpl_bind_type_assignable(expression_type, declared_type))
        {
            DPL_AST_ERROR(binding->source, node,
                          "Cannot assign expression of type `" SV_Fmt "` to %s `" SV_Fmt "` of type `" SV_Fmt "`.",
                          SV_Arg(expression_type->name), what, SV_Arg(decl->name.text), SV_Arg(declared_type->name));
        }
    }
}

// Creation

static DPL_Bound_Node *dpl_bind_allocate_node(DPL_Binding *binding, DPL_BoundNodeKind kind, DPL_Symbol *type)
{
    DPL_Bound_Node *bound_node = arena_alloc(binding->memory, sizeof(DPL_Bound_Node));
    bound_node->kind = kind;
    bound_node->type = type;
    return bound_node;
}

DPL_Bound_Node *dpl_bind_create_scope(DPL_Binding *binding, size_t expression_count, DPL_Bound_Node **expressions)
{
    if (expression_count == 0)
    {
        return NULL;
    }

    DPL_Bound_Node *scope = dpl_bind_allocate_node(binding, BOUND_NODE_SCOPE, expressions[expression_count - 1]->type);
    scope->as.scope.expressions_count = expression_count;
    scope->as.scope.expressions = arena_alloc(binding->memory, sizeof(DPL_Bound_Node *) * expression_count);
    memcpy(scope->as.scope.expressions, expressions, sizeof(DPL_Bound_Node *) * expression_count);

    return scope;
}

DPL_Bound_Node *dpl_bind_create_scope_move(DPL_Binding *binding, da_array(DPL_Bound_Node *) expressions)
{
    DPL_Bound_Node *scope = dpl_bind_create_scope(binding, da_size(expressions), expressions);
    da_free(expressions);
    return scope;
}

DPL_Bound_Node *dpl_bind_create_assignment(DPL_Binding *binding, DPL_Symbol *var, DPL_Bound_Node *expression)
{
    DPL_Bound_Node *assignment = dpl_bind_allocate_node(binding, BOUND_NODE_ASSIGNMENT, expression->type);
    assignment->as.assignment.scope_index = var->as.var.scope_index;
    assignment->as.assignment.expression = expression;
    return assignment;
}

DPL_Bound_Node *dpl_bind_create_varref(DPL_Binding *binding, DPL_Symbol *var)
{
    DPL_Bound_Node *varref = dpl_bind_allocate_node(binding, BOUND_NODE_VARREF, var->as.var.type);
    varref->as.varref = var->as.var.scope_index;
    return varref;
}

DPL_Bound_Node *dpl_bind_create_load_field(DPL_Binding *binding, DPL_Bound_Node *expression, size_t field_index)
{
    DPL_Symbol *resolved_type = dpl_bind_resolve_type_alias(expression->type);

    DPL_Bound_Node *load_field = dpl_bind_allocate_node(binding, BOUND_NODE_LOAD_FIELD, resolved_type->as.type.as.object.fields[field_index].type);
    load_field->as.load_field.expression = expression;
    load_field->as.load_field.field_index = field_index;
    return load_field;
}

DPL_Bound_Node *dpl_bind_create_while_loop(DPL_Binding *binding, DPL_Bound_Node *condition, DPL_Bound_Node *body)
{
    // TODO: At the moment, loops do not produce values and therefore are of type `none`.
    //       This should change in the future, where they can yield optional values or
    //       arrays.
    DPL_Bound_Node *while_loop = dpl_bind_allocate_node(binding, BOUND_NODE_WHILE_LOOP, dpl_symbols_find_type_none(binding->symbols));
    while_loop->as.while_loop.condition = condition;
    while_loop->as.while_loop.body = body;
    return while_loop;
}

static void dpl_bind_move_nodelist(DPL_Binding *binding, da_array(DPL_Bound_Node *) list, size_t *target_count, DPL_Bound_Node ***target_items)
{
    *target_count = da_size(list);
    if (da_some(list))
    {
        *target_items = arena_alloc(binding->memory, sizeof(DPL_Bound_Node *) * da_size(list));
        memcpy(*target_items, list, sizeof(DPL_Bound_Node *) * da_size(list));
        da_free(list);
    }
}

static void dpl_bind_check_function_used(DPL_Binding *binding, DPL_Symbol *symbol)
{
    DPL_Symbol_Function *f = &symbol->as.function;
    if (f->kind == FUNCTION_USER && !f->as.user_function.used)
    {
        f->as.user_function.used = true;
        f->as.user_function.user_handle = da_size(binding->user_functions);

        DPL_Binding_UserFunction user_function = {
            .function = symbol,
            .arity = f->signature.argument_count,
            .begin_ip = 0,
            .body = (DPL_Bound_Node *)f->as.user_function.body,
        };
        da_add(binding->user_functions, user_function);
    }
}

// Constant folding

static DPL_Symbol_Constant dpl_bind_fold_constant(DPL_Binding *binding, DPL_Ast_Node *node);

static Nob_String_View dpl_bind_unescape_string(DPL_Binding *binding, Nob_String_View escaped_string,
                                                size_t prefix_count, size_t postfix_count)
{
    // unescape source string literal
    char *unescaped_string = arena_alloc(
        binding->memory,
        // -(prefix_count + postfix_count) for delimiters; +1 for terminating null byte
        sizeof(char) * (escaped_string.count - (prefix_count + postfix_count) + 1));

    const char *source_pos = escaped_string.data + prefix_count;
    const char *source_end = escaped_string.data + (escaped_string.count - postfix_count - 1);
    char *target_pos = unescaped_string;

    while (source_pos <= source_end)
    {
        if (*source_pos == '\\')
        {
            source_pos++;
            switch (*source_pos)
            {
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
        }
        else
        {
            *target_pos = *source_pos;
        }

        source_pos++;
        target_pos++;
    }

    *target_pos = '\0';
    return nob_sv_from_cstr(unescaped_string);
}

static DPL_Symbol_Constant dpl_bind_fold_constant_literal(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Token value = node->as.literal.value;
    switch (value.kind)
    {
    case TOKEN_NUMBER:
        return (DPL_Symbol_Constant){
            .type = dpl_symbols_find_type_number(binding->symbols),
            .as.number = atof(nob_temp_sv_to_cstr(value.text)),
        };
    case TOKEN_STRING:
        return (DPL_Symbol_Constant){
            .type = dpl_symbols_find_type_string(binding->symbols),
            .as.string = dpl_bind_unescape_string(binding, value.text, 1, 1),
        };
    case TOKEN_STRING_INTERPOLATION:
        return (DPL_Symbol_Constant){
            .type = dpl_symbols_find_type_string(binding->symbols),
            .as.string = dpl_bind_unescape_string(binding, value.text, 1, 2),
        };
    case TOKEN_TRUE:
        return (DPL_Symbol_Constant){
            .type = dpl_symbols_find_type_boolean(binding->symbols),
            .as.boolean = true,
        };
    case TOKEN_FALSE:
        return (DPL_Symbol_Constant){
            .type = dpl_symbols_find_type_boolean(binding->symbols),
            .as.boolean = false,
        };
    default:
        DPL_TOKEN_ERROR(binding->source, value, "Cannot fold %s.", dpl_lexer_token_kind_name(value.kind));
    }
}

static DPL_Symbol_Constant dpl_bind_fold_constant_binary(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Token operator= node->as.binary.operator;
    DPL_Symbol_Constant lhs_value = dpl_bind_fold_constant(binding, node->as.binary.left);
    DPL_Symbol_Constant rhs_value = dpl_bind_fold_constant(binding, node->as.binary.right);

    switch (operator.kind)
    {
    case TOKEN_PLUS:
    {
        if (dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_NUMBER) && dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_NUMBER))
        {
            return (DPL_Symbol_Constant){
                .type = lhs_value.type,
                .as.number = lhs_value.as.number + rhs_value.as.number,
            };
        }
        if (dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_STRING) && dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_STRING))
        {
            return (DPL_Symbol_Constant){
                .type = lhs_value.type,
                .as.string = nob_sv_from_cstr(nob_temp_sprintf(SV_Fmt SV_Fmt, SV_Arg(lhs_value.as.string), SV_Arg(rhs_value.as.string))),
            };
        }

        DPL_TOKEN_ERROR(binding->source, operator,
                        "Cannot fold constant for binary operator %s: Both operands must be either of type `number` or `string`.",
                        dpl_lexer_token_kind_name(operator.kind));
    }
    case TOKEN_MINUS:
    {
        if (!dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_NUMBER))
        {
            DPL_AST_ERROR(binding->source, node->as.binary.left,
                          "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                          dpl_lexer_token_kind_name(operator.kind));
        }
        if (!dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_NUMBER))
        {
            DPL_AST_ERROR(binding->source, node->as.binary.right,
                          "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                          dpl_lexer_token_kind_name(operator.kind));
        }

        return (DPL_Symbol_Constant){
            .type = lhs_value.type,
            .as.number = lhs_value.as.number - rhs_value.as.number,
        };
    }
    break;
    case TOKEN_STAR:
    {
        if (!dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_NUMBER))
        {
            DPL_AST_ERROR(binding->source, node->as.binary.left,
                          "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                          dpl_lexer_token_kind_name(operator.kind));
        }
        if (!dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_NUMBER))
        {
            DPL_AST_ERROR(binding->source, node->as.binary.right,
                          "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                          dpl_lexer_token_kind_name(operator.kind));
        }

        return (DPL_Symbol_Constant){
            .type = lhs_value.type,
            .as.number = lhs_value.as.number * rhs_value.as.number,
        };
    }
    break;
    case TOKEN_SLASH:
    {
        if (!dpl_symbols_is_type_base(lhs_value.type, TYPE_BASE_NUMBER))
        {
            DPL_AST_ERROR(binding->source, node->as.binary.left,
                          "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                          dpl_lexer_token_kind_name(operator.kind));
        }
        if (!dpl_symbols_is_type_base(rhs_value.type, TYPE_BASE_NUMBER))
        {
            DPL_AST_ERROR(binding->source, node->as.binary.right,
                          "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                          dpl_lexer_token_kind_name(operator.kind));
        }

        return (DPL_Symbol_Constant){
            .type = lhs_value.type,
            .as.number = lhs_value.as.number / rhs_value.as.number,
        };
    }
    break;
    default:
        DPL_TOKEN_ERROR(binding->source, operator, "Cannot fold constant for binary operator %s.",
                        dpl_lexer_token_kind_name(operator.kind));
    }
}

static DPL_Symbol_Constant dpl_bind_fold_constant_symbol(DPL_Binding *binding, DPL_Ast_Node *node)
{
    Nob_String_View symbol_name = node->as.symbol.text;
    DPL_Symbol *symbol = dpl_symbols_find(binding->symbols, symbol_name);
    if (!symbol)
    {
        DPL_TOKEN_ERROR(binding->source, node->as.symbol,
                        "Cannot fold constant: unknown symbol `" SV_Fmt "`.",
                        SV_Arg(symbol_name));
    }

    if (symbol->kind != SYMBOL_CONSTANT)
    {
        DPL_TOKEN_ERROR(binding->source, node->as.symbol,
                        "Cannot fold constant: symbol `" SV_Fmt "` does not resolve to a constant value.",
                        SV_Arg(symbol_name));
    }

    return symbol->as.constant;
}

static DPL_Symbol_Constant dpl_bind_fold_constant(DPL_Binding *binding, DPL_Ast_Node *node)
{
    switch (node->kind)
    {
    case AST_NODE_LITERAL:
        return dpl_bind_fold_constant_literal(binding, node);
    case AST_NODE_BINARY:
        return dpl_bind_fold_constant_binary(binding, node);
    case AST_NODE_SYMBOL:
        return dpl_bind_fold_constant_symbol(binding, node);
    default:
        DPL_AST_ERROR(binding->source, node,
                      "Cannot fold constant expression of type `%s`.\n",
                      dpl_parse_nodekind_name(node->kind));
    }
}

// Bind ast nodes

static DPL_Bound_Node *dpl_bind_unary_function_call(DPL_Binding *binding, DPL_Bound_Node *operand, const char *function_name)
{
    DPL_Symbol *function_symbol = dpl_symbols_find_function1_cstr(binding->symbols, function_name, operand->type);

    if (function_symbol)
    {
        dpl_bind_check_function_used(binding, function_symbol);

        DPL_Bound_Node *bound_node = dpl_bind_allocate_node(binding, BOUND_NODE_FUNCTIONCALL,
                                                            function_symbol->as.function.signature.returns);
        bound_node->as.function_call.function = function_symbol;

        da_array(DPL_Bound_Node *) temp_arguments = 0;
        da_add(temp_arguments, operand);
        dpl_bind_move_nodelist(binding, temp_arguments, &bound_node->as.function_call.arguments_count, &bound_node->as.function_call.arguments);
        return bound_node;
    }

    return NULL;
}

static DPL_Bound_Node *dpl_bind_unary(DPL_Binding *binding, DPL_Ast_Node *node, const char *function_name)
{
    DPL_Bound_Node *operand = dpl_bind_node(binding, node->as.unary.operand);
    if (!operand)
    {
        DPL_AST_ERROR(binding->source, node, "Cannot bind operand of unary expression.");
    }

    DPL_Bound_Node *bound_unary = dpl_bind_unary_function_call(binding, operand, function_name);
    if (!bound_unary)
    {
        DPL_Token operator_token = node->as.unary.operator;
        DPL_AST_ERROR(binding->source, node, "Cannot resolve function \"%s(" SV_Fmt ")\" for unary operator \"" SV_Fmt "\".",
                      function_name, SV_Arg(operand->type->name), SV_Arg(operator_token.text));
    }

    return bound_unary;
}

static DPL_Bound_Node *dpl_bind_binary(DPL_Binding *binding, DPL_Ast_Node *node, const char *function_name)
{
    DPL_Bound_Node *lhs = dpl_bind_node(binding, node->as.binary.left);
    if (!lhs)
    {
        DPL_AST_ERROR(binding->source, node, "Cannot bind left-hand side of binary expression.");
    }
    DPL_Bound_Node *rhs = dpl_bind_node(binding, node->as.binary.right);
    if (!rhs)
    {
        DPL_AST_ERROR(binding->source, node, "Cannot bind right-hand side of binary expression.");
    }

    DPL_Symbol *function_symbol = dpl_symbols_find_function2_cstr(binding->symbols, function_name, lhs->type, rhs->type);
    if (function_symbol)
    {
        dpl_bind_check_function_used(binding, function_symbol);

        DPL_Bound_Node *bound_node = dpl_bind_allocate_node(binding, BOUND_NODE_FUNCTIONCALL,
                                                            function_symbol->as.function.signature.returns);
        bound_node->as.function_call.function = function_symbol;

        da_array(DPL_Bound_Node *) temp_arguments = 0;
        da_add(temp_arguments, lhs);
        da_add(temp_arguments, rhs);
        dpl_bind_move_nodelist(binding, temp_arguments, &bound_node->as.function_call.arguments_count, &bound_node->as.function_call.arguments);
        return bound_node;
    }

    DPL_Token operator_token = node->as.binary.operator;
    DPL_AST_ERROR(binding->source, node, "Cannot resolve function \"%s(" SV_Fmt ", " SV_Fmt ")\" for binary operator \"" SV_Fmt "\".",
                  function_name, SV_Arg(lhs->type->name), SV_Arg(rhs->type->name), SV_Arg(operator_token.text));
}

static DPL_Bound_Node *dpl_bind_function_call(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Bound_Node *bound_node = dpl_bind_allocate_node(binding, BOUND_NODE_FUNCTIONCALL, NULL);

    da_array(DPL_Symbol *) argument_types = NULL;
    da_array(DPL_Bound_Node *) temp_arguments = NULL;

    DPL_Ast_FunctionCall fc = node->as.function_call;
    for (size_t i = 0; i < fc.argument_count; ++i)
    {
        DPL_Bound_Node *bound_argument = dpl_bind_node(binding, fc.arguments[i]);
        if (!bound_argument)
        {
            DPL_AST_ERROR(binding->source, fc.arguments[i], "Cannot bind argument #%zu of function call.", i);
        }
        da_add(temp_arguments, bound_argument);
        da_add(argument_types, bound_argument->type);
    }
    dpl_bind_move_nodelist(binding, temp_arguments, &bound_node->as.function_call.arguments_count,
                           &bound_node->as.function_call.arguments);

    DPL_Symbol *function_symbol = dpl_symbols_find_function(binding->symbols, fc.name.text, da_size(argument_types), argument_types);
    if (function_symbol)
    {
        dpl_bind_check_function_used(binding, function_symbol);
        bound_node->as.function_call.function = function_symbol;
        bound_node->type = function_symbol->as.function.signature.returns;
        return bound_node;
    }

    Nob_String_Builder signature_builder = {0};
    {
        nob_sb_append_sv(&signature_builder, fc.name.text);
        nob_sb_append_cstr(&signature_builder, "(");
        DPL_Bound_Node **arguments = bound_node->as.function_call.arguments;
        size_t arguments_count = bound_node->as.function_call.arguments_count;
        for (size_t i = 0; i < arguments_count; ++i)
        {
            if (i > 0)
            {
                nob_sb_append_cstr(&signature_builder, ", ");
            }
            nob_sb_append_sv(&signature_builder, arguments[i]->type->name);
        }
        nob_sb_append_cstr(&signature_builder, ")");
        nob_sb_append_null(&signature_builder);
    }
    DPL_AST_ERROR(binding->source, node, "Cannot resolve function `%s`.", signature_builder.items);
}

static DPL_Bound_Node *dpl_bind_scope(DPL_Binding *binding, DPL_Ast_Node *node)
{

    DPL_Ast_Scope scope = node->as.scope;

    da_array(DPL_Bound_Node *) bound_expressions = 0;
    dpl_symbols_push_boundary_cstr(binding->symbols, NULL, BOUNDARY_SCOPE);
    for (size_t i = 0; i < scope.expression_count; ++i)
    {
        DPL_Bound_Node *bound_expression = dpl_bind_node(binding, scope.expressions[i]);
        if (!bound_expression)
        {
            continue;
        }

        da_add(bound_expressions, bound_expression);
    }
    dpl_symbols_pop_boundary(binding->symbols);

    return dpl_bind_create_scope_move(binding, bound_expressions);
}

static int dpl_bind_object_literal_compare_fields(void const *a, void const *b)
{
    return nob_sv_cmp(((DPL_Bound_ObjectField *)a)->name, ((DPL_Bound_ObjectField *)b)->name);
}

int dpl_bind_object_literal_compare_query(void const *a, void const *b)
{
    return nob_sv_cmp(((DPL_Symbol_Type_ObjectField *)a)->name, ((DPL_Symbol_Type_ObjectField *)b)->name);
}

void dpl_bind_object_literal_add_field(DPL_Binding *binding, da_array(DPL_Bound_ObjectField) * tmp_bound_fields,
                                       DPL_Symbol_Type_ObjectQuery *type_query, Nob_String_View field_name, DPL_Bound_Node *field_expression)
{
    DW_UNUSED(binding);

    for (size_t i = 0; i < da_size(*tmp_bound_fields); ++i)
    {
        if (nob_sv_eq((*tmp_bound_fields)[i].name, field_name))
        {
            (*tmp_bound_fields)[i].expression = field_expression;
            return;
        }
    }

    DPL_Bound_ObjectField bound_field = {
        .name = field_name,
        .expression = field_expression,
    };
    da_add(*tmp_bound_fields, bound_field);
    qsort(*tmp_bound_fields,
          da_size(*tmp_bound_fields), sizeof(**tmp_bound_fields),
          dpl_bind_object_literal_compare_fields);

    DPL_Symbol_Type_ObjectField query_field = {
        .name = bound_field.name,
        .type = bound_field.expression->type,
    };
    da_add(*type_query, query_field);
    qsort(*type_query,
          da_size(*type_query), sizeof(**type_query),
          dpl_bind_object_literal_compare_query);
}

DPL_Bound_Node *dpl_bind_object_literal(DPL_Binding *binding, DPL_Ast_Node *node)
{
    dpl_symbols_push_boundary_cstr(binding->symbols, NULL, BOUNDARY_SCOPE);

    DPL_Ast_ObjectLiteral object_literal = node->as.object_literal;
    da_array(DPL_Bound_ObjectField) tmp_bound_fields = NULL;
    DPL_Symbol_Type_ObjectQuery type_query = NULL;
    da_array(DPL_Bound_Node *) temporaries = NULL;
    for (size_t i = 0; i < object_literal.field_count; ++i)
    {
        DPL_Ast_Node *field = object_literal.fields[i];
        if (field->kind == AST_NODE_ASSIGNMENT && field->as.assignment.target->kind == AST_NODE_SYMBOL)
        {
            DPL_Ast_Assignment *assignment = &object_literal.fields[i]->as.assignment;
            dpl_bind_object_literal_add_field(
                binding, &tmp_bound_fields, &type_query,
                assignment->target->as.symbol.text,
                dpl_bind_node(binding, assignment->expression));
        }
        else if (field->kind == AST_NODE_UNARY && field->as.unary.operator.kind == TOKEN_DOT_DOT)
        {
            DPL_Bound_Node *bound_temporary = dpl_bind_node(binding, field->as.unary.operand);
            bound_temporary->persistent = true;
            if (bound_temporary->type->as.type.kind != TYPE_OBJECT)
            {
                DPL_AST_ERROR(binding->source, field, "Only object expressions can be spread for composing objects.");
            }
            da_add(temporaries, bound_temporary);

            DPL_Symbol *var = dpl_symbols_push_var(binding->symbols, SV_NULL, bound_temporary->type);

            DPL_Symbol_Type_Object bound_object_type = bound_temporary->type->as.type.as.object;
            for (size_t i = 0; i < bound_object_type.field_count; ++i)
            {
                dpl_bind_object_literal_add_field(
                    binding,
                    &tmp_bound_fields,
                    &type_query,
                    bound_object_type.fields[i].name,
                    dpl_bind_create_load_field(
                        binding,
                        dpl_bind_create_varref(binding, var),
                        i));
            }
        }
        else if (field->kind == AST_NODE_SYMBOL)
        {
            dpl_bind_object_literal_add_field(
                binding, &tmp_bound_fields, &type_query,
                field->as.symbol.text,
                dpl_bind_node(binding, field));
        }
        else
        {
            DPL_AST_ERROR(binding->source, field,
                          "Cannot use a %s in an object expression.",
                          dpl_parse_nodekind_name(field->kind));
        }
    }

    DPL_Symbol *bound_type = dpl_symbols_find_type_object_query(binding->symbols, type_query);
    if (!bound_type)
    {
        Nob_String_Builder type_name_builder = {0};
        nob_sb_append_cstr(&type_name_builder, "[");
        for (size_t i = 0; i < da_size(type_query); ++i)
        {
            if (i > 0)
            {
                nob_sb_append_cstr(&type_name_builder, ", ");
            }
            nob_sb_append_sv(&type_name_builder, type_query[i].name);
            nob_sb_append_cstr(&type_name_builder, ": ");
            nob_sb_append_sv(&type_name_builder, type_query[i].type->name);
        }
        nob_sb_append_cstr(&type_name_builder, "]");
        nob_sb_append_null(&type_name_builder);

        bound_type = dpl_symbols_push_type_object_cstr(binding->symbols, type_name_builder.items, da_size(type_query));
        memcpy(bound_type->as.type.as.object.fields, type_query, sizeof(DPL_Symbol_Type_ObjectField) * da_size(type_query));

        nob_sb_free(type_name_builder);
    }
    da_free(type_query);

    DPL_Bound_ObjectField *bound_fields = arena_alloc(binding->memory, sizeof(DPL_Bound_ObjectField) * da_size(tmp_bound_fields));
    memcpy(bound_fields, tmp_bound_fields, sizeof(DPL_Bound_ObjectField) * da_size(tmp_bound_fields));

    DPL_Bound_Node *bound_node = dpl_bind_allocate_node(binding, BOUND_NODE_OBJECT, bound_type);
    bound_node->as.object.field_count = da_size(tmp_bound_fields);
    bound_node->as.object.fields = bound_fields;

    da_free(tmp_bound_fields);

    dpl_symbols_pop_boundary(binding->symbols);

    if (da_some(temporaries))
    {
        da_add(temporaries, bound_node);
        return dpl_bind_create_scope_move(binding, temporaries);
    }

    return bound_node;
}

static DPL_Bound_Node *dpl_bind_literal(DPL_Binding *binding, DPL_Ast_Node *node)
{
    switch (node->as.literal.value.kind)
    {
    case TOKEN_NUMBER:
    case TOKEN_STRING:
    case TOKEN_STRING_INTERPOLATION:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    {

        DPL_Symbol_Constant value = dpl_bind_fold_constant(binding, node);
        DPL_Bound_Node *bound_node = dpl_bind_allocate_node(binding, BOUND_NODE_VALUE, value.type);
        bound_node->as.value = value;
        return bound_node;
    }
    default:
        DPL_AST_ERROR(binding->source, node, "Cannot resolve literal type for %s.",
                      dpl_lexer_token_kind_name(node->as.literal.value.kind));
    }
}

static DPL_Bound_Node *dpl_bind_field_access(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Bound_Node *bound_expression = dpl_bind_node(binding, node->as.field_access.expression);

    DPL_Symbol *expression_type = bound_expression->type;
    if (expression_type->as.type.kind == TYPE_ALIAS)
    {
        expression_type = expression_type->as.type.as.alias;
    }

    if (expression_type->as.type.kind != TYPE_OBJECT)
    {
        DPL_AST_ERROR(binding->source, node->as.field_access.expression,
                      "Can access fields only for object types." SV_Fmt,
                      SV_Arg(bound_expression->type->name));
    }

    DPL_Token field_name = node->as.field_access.field->as.symbol;
    DPL_Symbol_Type_Object object_type = expression_type->as.type.as.object;

    bool field_found = false;
    size_t field_index = 0;
    for (size_t i = 0; i < object_type.field_count; ++i)
    {
        if (nob_sv_eq(object_type.fields[i].name, field_name.text))
        {
            field_found = true;
            field_index = i;
            break;
        }
    }

    if (!field_found)
    {
        DPL_AST_ERROR(binding->source, node, "Objects of type `" SV_Fmt "`, have no field `" SV_Fmt "`.",
                      SV_Arg(bound_expression->type->name), SV_Arg(field_name.text));
    }

    return dpl_bind_create_load_field(binding, bound_expression, field_index);
}

static DPL_Bound_Node *dpl_bind_unary_operator(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Token operator= node->as.unary.operator;
    switch (operator.kind)
    {
    case TOKEN_MINUS:
        return dpl_bind_unary(binding, node, "negate");
    case TOKEN_BANG:
        return dpl_bind_unary(binding, node, "not");
    default:
        break;
    }

    DPL_AST_ERROR(binding->source, node,
                  "Cannot resolve function for unary operator \"" SV_Fmt "\".",
                  SV_Arg(operator.text));
}

DPL_Bound_Node *dpl_bind_binary_operator(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Token operator= node->as.binary.operator;
    switch (operator.kind)
    {
    case TOKEN_PLUS:
        return dpl_bind_binary(binding, node, "add");
    case TOKEN_MINUS:
        return dpl_bind_binary(binding, node, "subtract");
    case TOKEN_STAR:
        return dpl_bind_binary(binding, node, "multiply");
    case TOKEN_SLASH:
        return dpl_bind_binary(binding, node, "divide");
    case TOKEN_LESS:
        return dpl_bind_binary(binding, node, "less");
    case TOKEN_LESS_EQUAL:
        return dpl_bind_binary(binding, node, "lessEqual");
    case TOKEN_GREATER:
        return dpl_bind_binary(binding, node, "greater");
    case TOKEN_GREATER_EQUAL:
        return dpl_bind_binary(binding, node, "greaterEqual");
    case TOKEN_EQUAL_EQUAL:
        return dpl_bind_binary(binding, node, "equal");
    case TOKEN_BANG_EQUAL:
        return dpl_bind_binary(binding, node, "notEqual");
    case TOKEN_AND_AND:
    case TOKEN_PIPE_PIPE:
    {
        DPL_Bound_Node *lhs = dpl_bind_node(binding, node->as.binary.left);
        if (!lhs)
        {
            DPL_AST_ERROR(binding->source, node, "Cannot bind left-hand side of binary expression.");
        }
        DPL_Bound_Node *rhs = dpl_bind_node(binding, node->as.binary.right);
        if (!rhs)
        {
            DPL_AST_ERROR(binding->source, node, "Cannot bind right-hand side of binary expression.");
        }

        DPL_Bound_Node *bound_node = dpl_bind_allocate_node(binding, BOUND_NODE_LOGICAL_OPERATOR, dpl_symbols_find_type_boolean(binding->symbols));
        bound_node->as.logical_operator.operator= node->as.binary.operator;
        bound_node->as.logical_operator.lhs = lhs;
        bound_node->as.logical_operator.rhs = rhs;
        return bound_node;
    }
    default:
        break;
    }

    DPL_AST_ERROR(binding->source, node, "Cannot resolve function for binary operator %s.",
                  dpl_lexer_token_kind_name(operator.kind));
}

DPL_Bound_Node *dpl_bind_declaration(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Ast_Declaration *decl = &node->as.declaration;

    switch (decl->keyword.kind)
    {
    case TOKEN_KEYWORD_CONSTANT:
    {
        DPL_Symbol *s = dpl_symbols_push(binding->symbols, SYMBOL_CONSTANT, decl->name.text);
        s->as.constant = dpl_bind_fold_constant(binding, decl->initialization);
        dpl_bind_check_assignment(binding, "constant", node, s->as.constant.type);

        return NULL;
    }
    case TOKEN_KEYWORD_VAR:
    {
        DPL_Bound_Node *expression = dpl_bind_node(binding, decl->initialization);
        expression->persistent = true;
        dpl_bind_check_assignment(binding, "variable", node, expression->type);

        dpl_symbols_push_var(binding->symbols, decl->name.text, expression->type);

        return expression;
    }
    case TOKEN_KEYWORD_TYPE:
    {
        dpl_symbols_push_type_alias(binding->symbols, decl->name.text, dpl_bind_type(binding, decl->type));
        return NULL;
    }
    default:
        DW_UNIMPLEMENTED_MSG("Cannot bind declaration with `%s`.",
                             dpl_lexer_token_kind_name(decl->keyword.kind));
    }
}

DPL_Bound_Node *dpl_bind_symbol(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Symbol *symbol = dpl_symbols_find(binding->symbols, node->as.symbol.text);
    if (!symbol)
    {
        DPL_AST_ERROR(binding->source, node, "Cannot resolve symbol `" SV_Fmt "` in current scope.",
                      SV_Arg(node->as.symbol.text));
    }

    switch (symbol->kind)
    {
    case SYMBOL_CONSTANT:
    {
        DPL_Bound_Node *node = dpl_bind_allocate_node(binding, BOUND_NODE_VALUE, symbol->as.constant.type);
        node->as.value = symbol->as.constant;
        return node;
    }
    case SYMBOL_VAR:
        return dpl_bind_create_varref(binding, symbol);
    case SYMBOL_ARGUMENT:
    {
        DPL_Bound_Node *node = dpl_bind_allocate_node(binding, BOUND_NODE_ARGREF, symbol->as.argument.type);
        node->as.argref = symbol->as.argument.scope_index;
        return node;
    }
    default:
        DPL_AST_ERROR(binding->source, node, "Cannot resolve symbols of type `%s`.",
                      dpl_symbols_kind_name(symbol->kind));
    }
}

DPL_Bound_Node *dpl_bind_assignment(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Ast_Assignment *assignment = &node->as.assignment;

    if (assignment->target->kind != AST_NODE_SYMBOL)
    {
        DPL_AST_ERROR(binding->source, node, "Cannot assign to target of type `%s`.",
                      dpl_parse_nodekind_name(assignment->target->kind));
    }

    Nob_String_View symbol_name = assignment->target->as.symbol.text;
    DPL_Symbol *symbol = dpl_symbols_find(binding->symbols, symbol_name);
    if (!symbol)
    {
        DPL_AST_ERROR(binding->source, assignment->target, "Cannot find symbol `" SV_Fmt "`.",
                      SV_Arg(symbol_name));
    }
    if (symbol->kind != SYMBOL_VAR)
    {
        DPL_AST_ERROR(binding->source, assignment->target, "Cannot assign to %s `" SV_Fmt "`.",
                      dpl_symbols_kind_name(symbol->kind), SV_Arg(symbol_name));
    }

    DPL_Bound_Node *bound_expression = dpl_bind_node(binding, assignment->expression);

    if (symbol->as.var.type != bound_expression->type)
    {
        DPL_AST_ERROR(binding->source, node, "Cannot assign expression of type `" SV_Fmt "` to variable `" SV_Fmt "` of type `" SV_Fmt "`.",
                      SV_Arg(bound_expression->type->name), SV_Arg(symbol->name), SV_Arg(symbol->as.var.type->name));
    }

    return dpl_bind_create_assignment(binding, symbol, bound_expression);
}

DPL_Bound_Node *dpl_bind_function(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Ast_Function *function = &node->as.function;

    DPL_Symbol *function_symbol = dpl_symbols_push_function_user(binding->symbols, function->name.text, function->signature.argument_count);

    for (size_t i = 0; i < function->signature.argument_count; ++i)
    {
        DPL_Ast_FunctionArgument arg = function->signature.arguments[i];
        DPL_Symbol *arg_type = dpl_bind_resolve_type_alias(dpl_bind_type(binding, arg.type));
        if (!arg_type)
        {
            DPL_AST_ERROR(binding->source, arg.type,
                          "Cannot resolve type `%s` for argument `" SV_Fmt "` in current scope.",
                          dpl_bind_type_name(arg.type), SV_Arg(arg.name.text));
        }

        function_symbol->as.function.signature.arguments[i] = arg_type;
    }

    dpl_symbols_push_boundary_cstr(binding->symbols, NULL, BOUNDARY_FUNCTION);

    for (size_t i = 0; i < function->signature.argument_count; ++i)
    {
        dpl_symbols_push_argument(binding->symbols, function->signature.arguments[i].name.text,
                                  function_symbol->as.function.signature.arguments[i]);
    }

    DPL_Bound_Node *bound_body = dpl_bind_node(binding, function->body);

    if (function->signature.type)
    {
        DPL_Symbol *return_type = dpl_bind_resolve_type_alias(dpl_bind_type(binding, function->signature.type));
        if (!return_type)
        {
            DPL_AST_ERROR(binding->source, function->signature.type,
                          "Cannot resolve return type `%s` in current scope.",
                          dpl_bind_type_name(function->signature.type));
        }

        if (return_type != bound_body->type)
        {
            DPL_AST_ERROR(binding->source, node,
                          "Declared return type `%s` of function `" SV_Fmt "` is not compatible with body expression type `" SV_Fmt "`.",
                          dpl_bind_type_name(function->signature.type),
                          SV_Arg(function->name.text),
                          SV_Arg(bound_body->type->name));
        }

        function_symbol->as.function.signature.returns = return_type;
    }
    else
    {
        function_symbol->as.function.signature.returns = bound_body->type;
    }

    function_symbol->as.function.as.user_function.body = bound_body;

    dpl_symbols_pop_boundary(binding->symbols);
    return NULL;
}

DPL_Bound_Node *dpl_bind_conditional(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Ast_Conditional *conditional = &node->as.conditional;

    DPL_Bound_Node *bound_condition = dpl_bind_node(binding, conditional->condition);
    if (!dpl_symbols_is_type_base(bound_condition->type, TYPE_BASE_BOOLEAN))
    {
        DPL_Symbol *boolean_type = dpl_symbols_find_type_boolean(binding->symbols);

        DPL_AST_ERROR(binding->source, conditional->condition,
                      "Condition operand type `" SV_Fmt "` does not match type `" SV_Fmt "`.",
                      SV_Arg(bound_condition->type->name), SV_Arg(boolean_type->name));
    }

    DPL_Bound_Node *bound_then_clause = dpl_bind_node(binding, conditional->then_clause);
    DPL_Bound_Node *bound_else_clause = dpl_bind_node(binding, conditional->else_clause);
    if (bound_then_clause->type != bound_else_clause->type)
    {
        DPL_AST_ERROR(binding->source, node, "Types `" SV_Fmt "` and `" SV_Fmt "` do not match in the conditional expression clauses.",
                      SV_Arg(bound_then_clause->type->name), SV_Arg(bound_else_clause->type->name));
    }

    DPL_Bound_Node *bound_node = dpl_bind_allocate_node(binding, BOUND_NODE_CONDITIONAL, bound_then_clause->type);
    bound_node->as.conditional.condition = bound_condition;
    bound_node->as.conditional.then_clause = bound_then_clause;
    bound_node->as.conditional.else_clause = bound_else_clause;
    return bound_node;
}

DPL_Bound_Node *dpl_bind_while_loop(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Ast_WhileLoop *while_loop = &node->as.while_loop;

    DPL_Bound_Node *bound_condition = dpl_bind_node(binding, while_loop->condition);
    if (!dpl_symbols_is_type_base(bound_condition->type, TYPE_BASE_BOOLEAN))
    {
        DPL_Symbol *boolean_type = dpl_symbols_find_type_boolean(binding->symbols);

        DPL_AST_ERROR(binding->source, while_loop->condition,
                      "Condition operand type `" SV_Fmt "` does not match type `" SV_Fmt "`.",
                      SV_Arg(bound_condition->type->name), SV_Arg(boolean_type->name));
    }

    return dpl_bind_create_while_loop(
        binding,
        bound_condition,
        dpl_bind_node(binding, while_loop->body));
}

DPL_Bound_Node *dpl_bind_for_loop(DPL_Binding *binding, DPL_Ast_Node *node)
{

    DPL_Ast_ForLoop *for_loop = &node->as.for_loop;

    dpl_symbols_push_boundary_cstr(binding->symbols, NULL, BOUNDARY_SCOPE);

    DPL_Bound_Node *bound_iterator_initializer = dpl_bind_node(binding, for_loop->iterator_initializer);
    DPL_Symbol *iterator_type = bound_iterator_initializer->type;

    DPL_Symbol *resolved_iterator_type = dpl_bind_resolve_type_alias(iterator_type);
    if (resolved_iterator_type->as.type.kind != TYPE_OBJECT)
    {
        DPL_AST_ERROR(binding->source, for_loop->iterator_initializer,
                      "Only objects can be used as iterators in for loops.");
    }

    DPL_Symbol_Type_Object *object_type = &resolved_iterator_type->as.type.as.object;
    bool have_finished = false;
    DPL_Symbol *value_type = NULL;
    size_t finished_index;
    size_t current_index;
    for (size_t i = 0; i < object_type->field_count; ++i)
    {
        if (nob_sv_eq(object_type->fields[i].name, nob_sv_from_cstr("finished")) && dpl_symbols_is_type_base(object_type->fields[i].type, TYPE_BASE_BOOLEAN))
        {
            have_finished = true;
            finished_index = i;
        }
        else if (nob_sv_eq(object_type->fields[i].name, nob_sv_from_cstr("current")))
        {
            value_type = object_type->fields[i].type;
            current_index = i;
        }
    }
    if (!have_finished || !value_type)
    {
        DPL_AST_ERROR(binding->source, for_loop->iterator_initializer,
                      "Iterator in for loop needs a field `current` and a field `finished` of type `Boolean`.");
    }

    DPL_Symbol *next_function = dpl_symbols_find_function1_cstr(binding->symbols, "next", iterator_type);
    if (!next_function || (next_function->as.function.signature.returns != iterator_type))
    {
        DPL_AST_ERROR(binding->source, for_loop->iterator_initializer,
                      "Iterator in for loop needs a function `next(" SV_Fmt "): " SV_Fmt "`.",
                      SV_Arg(iterator_type->name),
                      SV_Arg(iterator_type->name));
    }

    DPL_Symbol *iterator_var = dpl_symbols_push_var(binding->symbols, SV_NULL, iterator_type);
    DPL_Bound_Node *init_assignment = bound_iterator_initializer;
    init_assignment->persistent = true;

    DPL_Bound_Node *while_condition = dpl_bind_unary_function_call(
        binding,
        dpl_bind_create_load_field(
            binding,
            dpl_bind_create_varref(binding, iterator_var),
            finished_index),
        "not");

    dpl_symbols_push_boundary_cstr(binding->symbols, NULL, BOUNDARY_SCOPE);

    dpl_symbols_push_var(binding->symbols, for_loop->variable_name.text, value_type);
    DPL_Bound_Node *current_assignment = dpl_bind_create_load_field(
        binding,
        dpl_bind_create_varref(binding, iterator_var),
        current_index);
    current_assignment->persistent = true;

    DPL_Bound_Node *inner_body = dpl_bind_node(binding, for_loop->body);

    DPL_Bound_Node *next_assignment = dpl_bind_create_assignment(
        binding,
        iterator_var,
        dpl_bind_unary_function_call(
            binding,
            dpl_bind_create_varref(binding, iterator_var),
            "next"));

    dpl_symbols_pop_boundary(binding->symbols);

    DPL_Bound_Node *while_loop = dpl_bind_create_while_loop(
        binding,
        while_condition,
        dpl_bind_create_scope(binding, DPL_BOUND_NODES(current_assignment, inner_body, next_assignment)));

    DPL_Bound_Node *scope = dpl_bind_create_scope(binding, DPL_BOUND_NODES(init_assignment, while_loop));

    dpl_symbols_pop_boundary(binding->symbols);

    return scope;
}

DPL_Bound_Node *dpl_bind_interpolation(DPL_Binding *binding, DPL_Ast_Node *node)
{
    DPL_Ast_Interpolation *interpolation = &node->as.interpolation;

    da_array(DPL_Bound_Node *) bound_expressions = NULL;
    for (size_t i = 0; i < interpolation->expression_count; ++i)
    {
        DPL_Bound_Node *bound_expression = dpl_bind_node(binding, interpolation->expressions[i]);

        if (!dpl_symbols_is_type_base(bound_expression->type, TYPE_BASE_STRING))
        {
            DPL_Bound_Node *bound_expression_to_string = dpl_bind_unary_function_call(binding, bound_expression, "toString");
            if (!bound_expression_to_string)
            {
                DPL_AST_ERROR(binding->source, node, "Cannot resolve function `toString(" SV_Fmt ")` for string interpolation.",
                              SV_Arg(bound_expression->type->name));
            }

            bound_expression = bound_expression_to_string;
        }

        da_add(bound_expressions, bound_expression);
    }

    DPL_Bound_Node *bound_node = dpl_bind_allocate_node(binding, BOUND_NODE_INTERPOLATION, dpl_symbols_find_type_string(binding->symbols));
    dpl_bind_move_nodelist(binding, bound_expressions, &bound_node->as.interpolation.expressions_count,
                           &bound_node->as.interpolation.expressions);
    return bound_node;
}

DPL_Bound_Node *dpl_bind_node(DPL_Binding *binding, DPL_Ast_Node *node)
{
    switch (node->kind)
    {
    case AST_NODE_LITERAL:
        return dpl_bind_literal(binding, node);
    case AST_NODE_OBJECT_LITERAL:
        return dpl_bind_object_literal(binding, node);
    case AST_NODE_FIELD_ACCESS:
        return dpl_bind_field_access(binding, node);
    case AST_NODE_UNARY:
        return dpl_bind_unary_operator(binding, node);
    case AST_NODE_BINARY:
        return dpl_bind_binary_operator(binding, node);
    case AST_NODE_FUNCTIONCALL:
        return dpl_bind_function_call(binding, node);
    case AST_NODE_SCOPE:
        return dpl_bind_scope(binding, node);
    case AST_NODE_DECLARATION:
        return dpl_bind_declaration(binding, node);
    case AST_NODE_SYMBOL:
        return dpl_bind_symbol(binding, node);
    case AST_NODE_ASSIGNMENT:
        return dpl_bind_assignment(binding, node);
    case AST_NODE_FUNCTION:
        return dpl_bind_function(binding, node);
    case AST_NODE_CONDITIONAL:
        return dpl_bind_conditional(binding, node);
    case AST_NODE_WHILE_LOOP:
        return dpl_bind_while_loop(binding, node);
    case AST_NODE_FOR_LOOP:
        return dpl_bind_for_loop(binding, node);
    case AST_NODE_INTERPOLATION:
        return dpl_bind_interpolation(binding, node);
    default:
        DPL_AST_ERROR(binding->source, node, "Cannot bind %s expressions.",
                      dpl_parse_nodekind_name(node->kind));
    }
}

void dpl_bind_print(DPL_Binding *binding, DPL_Bound_Node *node, size_t level)
{
    for (size_t i = 0; i < level; ++i)
    {
        printf("  ");
    }

    if (!node)
    {
        printf("<nil>\n");
        return;
    }

    if (node->persistent)
    {
        printf("*");
    }

    if (node->type)
    {
        printf("[" SV_Fmt "] ", SV_Arg(node->type->name));
    }
    else
    {
        printf("[<unknown>] ");
    }

    switch (node->kind)
    {
    case BOUND_NODE_FUNCTIONCALL:
    {
        DPL_Symbol *function = node->as.function_call.function;
        printf(SV_Fmt "(\n", SV_Arg(function->name));

        for (size_t i = 0; i < node->as.function_call.arguments_count; ++i)
        {
            dpl_bind_print(binding, node->as.function_call.arguments[i], level + 1);
        }

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_VALUE:
    {
        printf("Value `");
        if (dpl_symbols_is_type_base(node->as.value.type, TYPE_BASE_NUMBER))
        {
            printf("%f", node->as.value.as.number);
        }
        else if (dpl_symbols_is_type_base(node->as.value.type, TYPE_BASE_STRING))
        {
            Nob_String_View value = node->as.value.as.string;
            dplp_print_escaped_string(value.data, value.count);
        }
        else if (dpl_symbols_is_type_base(node->as.value.type, TYPE_BASE_BOOLEAN))
        {
            printf("%s", node->as.value.as.boolean ? "true" : "false");
        }
        else
        {
            printf("<unknown>");
        }
        printf("`\n");
    }
    break;
    case BOUND_NODE_OBJECT:
    {
        printf("$object(\n");

        DPL_Bound_Object object = node->as.object;
        for (size_t field_index = 0; field_index < object.field_count; ++field_index)
        {
            for (size_t i = 0; i < level + 1; ++i)
            {
                printf("  ");
            }
            printf(SV_Fmt ":\n", SV_Arg(object.fields[field_index].name));
            dpl_bind_print(binding, object.fields[field_index].expression, level + 2);
        }

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }

        printf(")\n");
    }
    break;
    case BOUND_NODE_SCOPE:
    {
        printf("$scope(\n");

        for (size_t i = 0; i < node->as.scope.expressions_count; ++i)
        {
            dpl_bind_print(binding, node->as.scope.expressions[i], level + 1);
        }

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_VARREF:
    {
        printf("$varref(scope_index = %zu)\n", node->as.varref);
    }
    break;
    case BOUND_NODE_ARGREF:
    {
        printf("$argref(scope_index = %zu)\n", node->as.argref);
    }
    break;
    case BOUND_NODE_ASSIGNMENT:
    {
        printf("$assignment(\n");

        for (size_t i = 0; i < level + 1; ++i)
        {
            printf("  ");
        }
        printf("scope_index %zu\n", node->as.assignment.scope_index);

        dpl_bind_print(binding, node->as.assignment.expression, level + 1);

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_CONDITIONAL:
    {
        printf("$conditional(\n");
        dpl_bind_print(binding, node->as.conditional.condition, level + 1);
        dpl_bind_print(binding, node->as.conditional.then_clause, level + 1);
        dpl_bind_print(binding, node->as.conditional.else_clause, level + 1);

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_LOGICAL_OPERATOR:
    {
        printf("$logical_operator(\n");
        for (size_t i = 0; i < level + 1; ++i)
        {
            printf("  ");
        }
        printf("%s\n", dpl_lexer_token_kind_name(node->as.logical_operator.operator.kind));
        dpl_bind_print(binding, node->as.logical_operator.lhs, level + 1);
        dpl_bind_print(binding, node->as.logical_operator.rhs, level + 1);

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_WHILE_LOOP:
    {
        printf("$while(\n");
        dpl_bind_print(binding, node->as.while_loop.condition, level + 1);
        dpl_bind_print(binding, node->as.while_loop.body, level + 1);

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_LOAD_FIELD:
    {
        printf("$load_field( #%zu\n", node->as.load_field.field_index);
        dpl_bind_print(binding, node->as.load_field.expression, level + 1);

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    case BOUND_NODE_INTERPOLATION:
    {
        printf("$interpolation(\n");

        for (size_t i = 0; i < node->as.interpolation.expressions_count; ++i)
        {
            dpl_bind_print(binding, node->as.interpolation.expressions[i], level + 1);
        }

        for (size_t i = 0; i < level; ++i)
        {
            printf("  ");
        }
        printf(")\n");
    }
    break;
    default:
        DW_UNIMPLEMENTED_MSG("%s", dpl_bind_nodekind_name(node->kind));
    }
}
