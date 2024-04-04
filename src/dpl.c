#include "dpl.h"

// Forward declarations needed for initialization
DPL_Type_Handle _dplt_register(DPL* types, DPL_Type type);
DPL_Type_Handle _dplt_register_by_name(DPL* types, Nob_String_View name);
void _dplt_add_handle(DPL_Type_Handles* handles, DPL_Type_Handle handle);
void _dplt_print(FILE* out, DPL* dpl, DPL_Type* type);

DPL_Function_Handle _dplf_register(DPL* dpl, Nob_String_View name,
                                   DPL_Type_Handle type);
void _dplf_print(FILE* out, DPL* dpl, DPL_Function* function);

void dpl_init(DPL *dpl)
{
    // CATALOGS

    /// TYPES
    dpl->types.number_handle = _dplt_register_by_name(dpl, nob_sv_from_cstr("number"));

    {
        DPL_Type unary = {0};
        unary.name = nob_sv_from_cstr("unary");
        unary.kind = TYPE_FUNCTION;
        _dplt_add_handle(&unary.as.function.arguments, dpl->types.number_handle);
        unary.as.function.returns = dpl->types.number_handle;
        dpl->types.unary_handle = _dplt_register(dpl, unary);
    }

    {
        DPL_Type binary = {0};
        binary.name = nob_sv_from_cstr("binary");
        binary.kind = TYPE_FUNCTION;
        _dplt_add_handle(&binary.as.function.arguments, dpl->types.number_handle);
        _dplt_add_handle(&binary.as.function.arguments, dpl->types.number_handle);
        binary.as.function.returns = dpl->types.number_handle;
        dpl->types.binary_handle = _dplt_register(dpl, binary);
    }

    if (dpl->debug)
    {
        for (size_t i = 0; i < dpl->types.count; ++i) {
            printf("* %zu: ", dpl->types.items[i].handle);
            _dplt_print(stdout, dpl, &dpl->types.items[i]);
            printf("\n");
        }
        printf("\n");
    }

    /// FUNCTIONS
    _dplf_register(dpl, nob_sv_from_cstr("negate"), dpl->types.unary_handle);

    _dplf_register(dpl, nob_sv_from_cstr("add"), dpl->types.binary_handle);
    _dplf_register(dpl, nob_sv_from_cstr("subtract"), dpl->types.binary_handle);
    _dplf_register(dpl, nob_sv_from_cstr("multiply"), dpl->types.binary_handle);
    _dplf_register(dpl, nob_sv_from_cstr("divide"), dpl->types.binary_handle);

    if (dpl->debug)
    {
        for (size_t i = 0; i < dpl->functions.count; ++i) {
            printf("* %zu: ", dpl->functions.items[i].handle);
            _dplf_print(stdout, dpl, &dpl->functions.items[i]);
            printf("\n");
        }
        printf("\n");
    }

    // lexer initialization
    dpl->current_line = dpl->source.data;
}

void dpl_free(DPL *dpl)
{
    // catalog freeing
    nob_da_free(dpl->types);
    nob_da_free(dpl->functions);

    // parser freeing
    arena_free(&dpl->tree.memory);

    // calltree freeing
    arena_free(&dpl->calltree.memory);
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

DPL_Type_Handle _dplt_add(DPL* dpl, DPL_Type type) {
    size_t index = dpl->types.count;
    nob_da_append(&dpl->types, type);

    return index;
}

DPL_Type_Handle _dplt_register_by_name(DPL* dpl, Nob_String_View name)
{
    DPL_Type type = {
        .name = name,
        .handle = dpl->types.count + 1,
        .hash = _dplt_hash(name),
    };
    _dplt_add(dpl, type);
    return type.handle;
}

DPL_Type_Handle _dplt_register(DPL* dpl, DPL_Type type)
{
    type.handle = dpl->types.count + 1;
    type.hash = _dplt_hash(type.name);
    _dplt_add(dpl, type);
    return type.handle;
}

DPL_Type* _dplt_find_by_handle(DPL *dpl, DPL_Type_Handle handle)
{
    for (size_t i = 0;  i < dpl->types.count; ++i) {
        if (dpl->types.items[i].handle == handle) {
            return &dpl->types.items[i];
        }
    }
    return 0;
}

DPL_Type* _dplt_find_by_name(DPL *dpl, Nob_String_View name)
{
    for (size_t i = 0;  i < dpl->types.count; ++i) {
        if (nob_sv_eq(dpl->types.items[i].name, name)) {
            return &dpl->types.items[i];
        }
    }
    return 0;
}

void _dplt_add_handle(DPL_Type_Handles* handles, DPL_Type_Handle handle)
{
    nob_da_append(handles, handle);
}

bool _dplt_add_handle_by_name(DPL_Type_Handles* handles, DPL* dpl, Nob_String_View name)
{
    DPL_Type* type = _dplt_find_by_name(dpl, name);
    if (!type) {
        return false;
    }

    _dplt_add_handle(handles, type->handle);
    return true;
}

void _dplt_print_meta_function(FILE* out, DPL* dpl, DPL_Type* type)
{
    fprintf(out, "(");
    for (size_t i = 0; i < type->as.function.arguments.count; ++i) {
        if (i > 0) {
            fprintf(out, ", ");
        }
        _dplt_print(out, dpl,
                    _dplt_find_by_handle(dpl, type->as.function.arguments.items[i]));
    }
    fprintf(out, ")");

    fprintf(out, ": ");
    _dplt_print(out, dpl, _dplt_find_by_handle(dpl, type->as.function.returns));
}

void _dplt_print(FILE* out, DPL* dpl, DPL_Type* type)
{
    if (!type) {
        fprintf(out, "<unknown type>");
        return;
    }

    fprintf(out, SV_Fmt, SV_Arg(type->name));

    switch (type->kind) {
    case TYPE_FUNCTION:
        fprintf(out, "[");
        _dplt_print_meta_function(out, dpl, type);
        fprintf(out, "]");
        break;
    default:
        break;
    }
}

/// FUNCTIONS

DPL_Function_Handle _dplf_register(DPL* dpl, Nob_String_View name,
                                   DPL_Type_Handle type)
{
    DPL_Function function = {
        .handle = dpl->functions.count + 1,
        .name = name,
        .type = type,
    };
    nob_da_append(&dpl->functions, function);

    return function.handle;
}

DPL_Function* _dplf_find_by_handle(DPL *dpl, DPL_Function_Handle handle)
{
    for (size_t i = 0;  i < dpl->functions.count; ++i) {
        if (dpl->functions.items[i].handle == handle) {
            return &dpl->functions.items[i];
        }
    }
    return 0;
}

DPL_Function* _dplf_find_by_signature1(DPL *dpl,
                                       Nob_String_View name, DPL_Type_Handle arg0)
{
    for (size_t i = 0; i < dpl->functions.count; ++i) {
        DPL_Function* function = &dpl->functions.items[i];
        if (nob_sv_eq(function->name, name)) {
            DPL_Type* function_type = _dplt_find_by_handle(dpl, function->type);
            if (function_type->kind == TYPE_FUNCTION
                    && function_type->as.function.arguments.count == 1
                    && function_type->as.function.arguments.items[0] == arg0
               ) {
                return function;
            }
        }
    }

    return 0;
}

DPL_Function* _dplf_find_by_signature2(DPL *dpl,
                                       Nob_String_View name, DPL_Type_Handle arg0, DPL_Type_Handle arg1)
{
    for (size_t i = 0; i < dpl->functions.count; ++i) {
        DPL_Function* function = &dpl->functions.items[i];
        if (nob_sv_eq(function->name, name)) {
            DPL_Type* function_type = _dplt_find_by_handle(dpl, function->type);
            if (function_type->kind == TYPE_FUNCTION
                    && function_type->as.function.arguments.count == 2
                    && function_type->as.function.arguments.items[0] == arg0
                    && function_type->as.function.arguments.items[1] == arg1
               ) {
                return function;
            }
        }
    }

    return 0;
}

void _dplf_print(FILE* out, DPL* dpl, DPL_Function* function) {
    if (!function) {
        fprintf(out, "<unknown function>");
        return;
    }

    fprintf(out, SV_Fmt, SV_Arg(function->name));
    _dplt_print_meta_function(out, dpl, _dplt_find_by_handle(dpl, function->type));
}

// LEXER

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
    return (DPL_Token) {
        .kind = kind,
        .location = dpl->token_start_location,
        .text = nob_sv_from_parts(
                    dpl->source.data + dpl->token_start,
                    dpl->position - dpl->token_start
                ),
    };
}

DPL_Token _dpll_build_empty_token(DPL* dpl, DPL_TokenKind kind)
{
    return (DPL_Token) {
        .kind = kind,
        .location = dpl->token_start_location,
        .text = SV_NULL,
    };
}

void _dpll_print_highlighted(FILE *f, DPL* dpl, const char* line_start, size_t column, size_t length)
{
    Nob_String_View line = nob_sv_from_parts(line_start, 0);
    while ((line.count < (size_t)((dpl->source.data + dpl->source.count) - line_start))
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

void _dpll_error(DPL* dpl, const char *fmt, ...) __attribute__ ((noreturn));
void _dpll_error(DPL* dpl, const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, LOC_Fmt ": ERROR: ", LOC_Arg(dpl->token_start_location));

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    _dpll_print_highlighted(stderr, dpl, dpl->current_line, dpl->token_start,
                            dpl->position - dpl->token_start);

    exit(1);
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
    case '(':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_OPEN_PAREN);
    case ')':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_CLOSE_PAREN);
    }

    if (isdigit(_dpll_current(dpl)))
    {
        while (!_dpll_is_eof(dpl) && isdigit(_dpll_current(dpl)))
            _dpll_advance(dpl);

        return _dpll_build_token(dpl, TOKEN_NUMBER);
    }

    if (isalpha(_dpll_current(dpl)))
    {
        while (!_dpll_is_eof(dpl) && isalnum(_dpll_current(dpl)))
            _dpll_advance(dpl);

        return _dpll_build_token(dpl, TOKEN_IDENTIFIER);
    }

    _dpll_error(dpl, "Invalid character '%c'.\n", _dpll_current(dpl));
    _dpll_advance(dpl);
}

DPL_Token _dpll_peek_token(DPL *dpl) {
    if (dpl->peek_token.kind == TOKEN_NONE) {
        dpl->peek_token = _dpll_next_token(dpl);
    }
    return dpl->peek_token;
}

const char* _dpll_token_kind_name(DPL_TokenKind kind)
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

void _dpll_print_token(DPL* dpl, DPL_Token token)
{
    printf(LOC_Fmt ": %s (" SV_Fmt ")\n",
           LOC_Arg(token.location),
           _dpll_token_kind_name(token.kind),
           SV_Arg(token.text));

    _dpll_print_highlighted(stdout, dpl, token.location.line_start,
                            token.location.column, token.text.count);
}

void _dpll_print_token_location(FILE* out, DPL* dpl, DPL_Token token)
{
    _dpll_print_highlighted(out, dpl, token.location.line_start,
                            token.location.column, token.text.count);
}

// AST

DPL_Ast_Node* _dpla_create_node(DPL_Ast_Tree* tree, DPL_AstNodeKind kind)
{
    DPL_Ast_Node* node = arena_alloc(&tree->memory, sizeof(DPL_Ast_Node));
    node->kind = kind;
    return node;
}

const char* _dpla_node_kind_name(DPL_AstNodeKind kind) {
    switch (kind) {
    case AST_NODE_LITERAL:
        return "AST_NODE_LITERAL";
    case AST_NODE_UNARY:
        return "AST_NODE_UNARY";
    case AST_NODE_BINARY:
        return "AST_NODE_BINARY";
    }

    assert(false && "unreachable: _dpla_node_kind_name");
    return "";
}

void _dpla_print(DPL_Ast_Node* node, size_t level) {
    for (size_t i = 0; i < level; ++i) {
        printf("  ");
    }

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
    case AST_NODE_LITERAL: {
        printf(" [%s: "SV_Fmt"]\n", _dpll_token_kind_name(node->as.literal.value.kind), SV_Arg(node->as.literal.value.text));
        break;
    }
    default: {
        printf("\n");
        break;
    }
    }
}

// PARSER

void _dplp_error_unexpected_token(DPL* dpl, DPL_Token token) __attribute__ ((noreturn));
void _dplp_error_unexpected_token(DPL* dpl, DPL_Token token) {
    fprintf(stderr, LOC_Fmt ": Unexpected token <%s>:\n", LOC_Arg(token.location), _dpll_token_kind_name(token.kind));
    _dpll_print_token_location(stderr, dpl, token);
    exit(1);
}

void _dplp_error_unexpected_token_expected(DPL* dpl, DPL_Token token, DPL_TokenKind expected_kind) __attribute__ ((noreturn));
void _dplp_error_unexpected_token_expected(DPL* dpl, DPL_Token token, DPL_TokenKind expected_kind) {
    fprintf(stderr, LOC_Fmt ": Unexpected token <%s>, expected <%s>:\n", LOC_Arg(token.location),
            _dpll_token_kind_name(token.kind), _dpll_token_kind_name(expected_kind));
    _dpll_print_token_location(stderr, dpl, token);
    exit(1);
}

void _dplp_skip_whitespace(DPL *dpl) {
    while (_dpll_peek_token(dpl).kind == TOKEN_WHITESPACE) {
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

DPL_Token _dplp_expect_token(DPL *dpl, DPL_TokenKind kind) {
    DPL_Token next_token = _dplp_next_token(dpl);
    if (next_token.kind != kind) {
        _dplp_error_unexpected_token_expected(dpl, next_token, kind);
    }
    return next_token;
}

DPL_Ast_Node* _dplp_parse_expression(DPL* dpl);

DPL_Ast_Node* _dplp_parse_primary(DPL* dpl)
{
    DPL_Token token = _dplp_next_token(dpl);
    switch (token.kind) {
    case TOKEN_NUMBER:
        /*case TOKEN_IDENTIFIER:*/
    {
        DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_LITERAL);
        node->as.literal.value = token;
        return node;
    }
    break;
    case TOKEN_OPEN_PAREN: {
        DPL_Ast_Node* node = _dplp_parse_expression(dpl);
        _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);
        return node;
    }
    break;
    default: {
        _dplp_error_unexpected_token(dpl, token);
    }
    }
}

DPL_Ast_Node* _dplp_parser_unary(DPL* dpl)
{
    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    if (operator_candidate.kind == TOKEN_MINUS) {
        _dplp_next_token(dpl);
        DPL_Ast_Node* operand = _dplp_parser_unary(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_UNARY);
        new_expression->as.unary.operator = operator_candidate;
        new_expression->as.unary.operand = operand;
        return new_expression;
    }

    return _dplp_parse_primary(dpl);
}

DPL_Ast_Node* _dplp_parse_multiplicative(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parser_unary(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_STAR || operator_candidate.kind == TOKEN_SLASH) {
        _dplp_next_token(dpl);
        DPL_Ast_Node* rhs = _dplp_parser_unary(dpl);

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY);
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

        DPL_Ast_Node* new_expression = _dpla_create_node(&dpl->tree, AST_NODE_BINARY);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_expression(DPL* dpl)
{
    return _dplp_parse_additive(dpl);
}


void _dplp_parse(DPL* dpl)
{
    dpl->tree.root = _dplp_parse_expression(dpl);
    _dplp_expect_token(dpl, TOKEN_EOF);
}

// CALLTREE

DPL_CallTree_Node* _dplc_bind_node(DPL* dpl, DPL_Ast_Node* node);

DPL_CallTree_Node* _dplc_bind_unary(DPL* dpl, DPL_Ast_Node* node, const char* function_name)
{
    DPL_CallTree_Node* operand = _dplc_bind_node(dpl, node->as.unary.operand);

    DPL_Function* function = _dplf_find_by_signature1(
                                 dpl,
                                 nob_sv_from_cstr(function_name),
                                 operand->type_handle);
    if (function) {
        DPL_Type* function_type = _dplt_find_by_handle(dpl, function->type);

        if (function_type) {
            DPL_CallTree_Node* calltree_node = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
            calltree_node->kind = CALLTREE_NODE_FUNCTION;
            calltree_node->type_handle = function_type->as.function.returns;

            calltree_node->as.function.function_handle = function->handle;
            nob_da_append(&calltree_node->as.function.arguments, operand);

            return calltree_node;
        }
    }

    DPL_Type* operand_type = _dplt_find_by_handle(dpl, operand->type_handle);

    DPL_Token operator_token = node->as.unary.operator;
    fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function \"%s("SV_Fmt")\" for unary operator \""SV_Fmt"\".\n",
            LOC_Arg(operator_token.location), function_name, SV_Arg(operand_type->name), SV_Arg(operator_token.text));
    _dpll_print_token_location(stderr, dpl, operator_token);

    exit(1);
}


DPL_CallTree_Node* _dplc_bind_binary(DPL* dpl, DPL_Ast_Node* node, const char* function_name)
{
    DPL_CallTree_Node* lhs = _dplc_bind_node(dpl, node->as.binary.left);
    DPL_CallTree_Node* rhs = _dplc_bind_node(dpl, node->as.binary.right);

    DPL_Function* function = _dplf_find_by_signature2(
                                 dpl,
                                 nob_sv_from_cstr(function_name),
                                 lhs->type_handle, rhs->type_handle);
    if (function) {
        DPL_Type* function_type = _dplt_find_by_handle(dpl, function->type);

        if (function_type) {
            DPL_CallTree_Node* calltree_node = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
            calltree_node->kind = CALLTREE_NODE_FUNCTION;
            calltree_node->type_handle = function_type->as.function.returns;

            calltree_node->as.function.function_handle = function->handle;
            nob_da_append(&calltree_node->as.function.arguments, lhs);
            nob_da_append(&calltree_node->as.function.arguments, rhs);

            return calltree_node;
        }
    }

    DPL_Type* lhs_type = _dplt_find_by_handle(dpl, lhs->type_handle);
    DPL_Type* rhs_type = _dplt_find_by_handle(dpl, rhs->type_handle);

    DPL_Token operator_token = node->as.binary.operator;
    fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function \"%s("SV_Fmt", "SV_Fmt")\" for binary operator \""SV_Fmt"\".\n",
            LOC_Arg(operator_token.location), function_name, SV_Arg(lhs_type->name), SV_Arg(rhs_type->name), SV_Arg(operator_token.text));
    _dpll_print_token_location(stderr, dpl, operator_token);

    exit(1);
}

DPL_CallTree_Node* _dplc_bind_node(DPL* dpl, DPL_Ast_Node* node)
{
    switch (node->kind)
    {
    case AST_NODE_LITERAL: {
        switch (node->as.literal.value.kind)
        {
        case TOKEN_NUMBER: {
            DPL_CallTree_Node* calltree_node = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
            calltree_node->kind = CALLTREE_NODE_VALUE;
            calltree_node->type_handle = dpl->types.number_handle;
            calltree_node->as.value.ast_node = node;
            return calltree_node;
        }
        break;
        default:
            break;
        }
        break;
    }
    case AST_NODE_UNARY: {
        DPL_Token operator = node->as.unary.operator;
        switch(operator.kind) {
        case TOKEN_MINUS:
            return _dplc_bind_unary(dpl, node, "negate");
        default:
            break;
        }

        fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function for unary operator \""SV_Fmt"\".\n",
                LOC_Arg(operator.location), SV_Arg(operator.text));
        _dpll_print_token_location(stderr, dpl, operator);
        exit(1);

    }
    case AST_NODE_BINARY: {
        DPL_Token operator = node->as.binary.operator;
        switch(operator.kind) {
        case TOKEN_PLUS:
            return _dplc_bind_binary(dpl, node, "add");
        case TOKEN_MINUS:
            return _dplc_bind_binary(dpl, node, "subtract");
        case TOKEN_STAR:
            return _dplc_bind_binary(dpl, node, "multiply");
        case TOKEN_SLASH:
            return _dplc_bind_binary(dpl, node, "divide");
        default:
            break;
        }

        fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function for binary operator \""SV_Fmt"\".\n",
                LOC_Arg(operator.location), SV_Arg(operator.text));
        _dpll_print_token_location(stderr, dpl, operator);
        exit(1);
    }
    break;
    default:
        break;
    }

    fprintf(stderr, "ERROR: Cannot resolve function call tree for AST node of kind \"%s\".\n",
            _dpla_node_kind_name(node->kind));
    exit(1);
}

void _dplc_bind(DPL* dpl)
{
    dpl->calltree.root = _dplc_bind_node(dpl, dpl->tree.root);
}

void _dplc_print(DPL* dpl, DPL_CallTree_Node* node, size_t level) {
    for (size_t i = 0; i < level; ++i) {
        printf("  ");
    }

    if (!node) {
        printf("<nil>\n");
        return;
    }

    DPL_Type* type = _dplt_find_by_handle(dpl, node->type_handle);
    printf("["SV_Fmt"] ", SV_Arg(type->name));

    switch(node->kind) {
    case CALLTREE_NODE_FUNCTION: {
        DPL_Function* function = _dplf_find_by_handle(dpl, node->as.function.function_handle);
        printf(SV_Fmt"(\n", SV_Arg(function->name));

        for (size_t i = 0; i < node->as.function.arguments.count; ++i) {
            _dplc_print(dpl, node->as.function.arguments.items[i], level + 1);
        }

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");

    }
    break;
    case CALLTREE_NODE_VALUE: {
        printf("Value \""SV_Fmt"\"\n", SV_Arg(node->as.value.ast_node->as.literal.value.text));
    }
    break;
    }
}

// PROGRAM GENERATION

void _dplg_generate(DPL* dpl, DPL_CallTree_Node* node, DPL_Program* program) {
    switch (node->kind)
    {
    case CALLTREE_NODE_VALUE: {
        if (node->type_handle == dpl->types.number_handle) {
            double value = atof(nob_temp_sv_to_cstr(node->as.value.ast_node->as.literal.value.text));
            dplp_write_push_number(program, value);
        } else {
            DPL_Type* type = _dplt_find_by_handle(dpl, node->type_handle);
            fprintf(stderr, "Cannot generate program for value node of type "SV_Fmt".\n", SV_Arg(type->name));
            exit(1);
        }
    }
    break;
    case CALLTREE_NODE_FUNCTION: {
        DPL_Function* function = _dplf_find_by_handle(dpl, node->as.function.function_handle);
        for (size_t i = 0; i < node->as.function.arguments.count; ++i) {
            _dplg_generate(dpl, node->as.function.arguments.items[i], program);
        }

        if (nob_sv_eq(function->name, nob_sv_from_cstr("negate"))) {
            dplp_write_negate(program);
        } else if (nob_sv_eq(function->name, nob_sv_from_cstr("add"))) {
            dplp_write_add(program);
        } else if (nob_sv_eq(function->name, nob_sv_from_cstr("subtract"))) {
            dplp_write_subtract(program);
        } else if (nob_sv_eq(function->name, nob_sv_from_cstr("multiply"))) {
            dplp_write_multiply(program);
        } else if (nob_sv_eq(function->name, nob_sv_from_cstr("divide"))) {
            dplp_write_divide(program);
        } else {
            fprintf(stderr, "Cannot generate program for function "SV_Fmt".\n", SV_Arg(function->name));
            exit(1);
        }
    }
    break;

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

    _dplc_bind(dpl);
    if (dpl->debug)
    {
        _dplc_print(dpl, dpl->calltree.root, 0);
        printf("\n");
    }

    _dplg_generate(dpl, dpl->calltree.root, program);
    if (dpl->debug)
    {
        dplp_print(program);
    }
}