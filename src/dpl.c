#include "dpl.h"

// Forward declarations needed for initialization
void _dpl_add_handle(DPL_Handles* handles, DPL_Handle handle);

DPL_Handle _dplt_register(DPL* types, DPL_Type type);
DPL_Handle _dplt_register_by_name(DPL* types, Nob_String_View name);
void _dplt_print(FILE* out, DPL* dpl, DPL_Type* type);

DPL_Handle _dplf_register(DPL* dpl,
                          Nob_String_View name,
                          DPL_Signature* signature,
                          DPL_Generator_Callback generator_callback,
                          void* generator_user_data);
void _dplf_print(FILE* out, DPL* dpl, DPL_Function* function);

void _dple_register(DPL *dpl, DPL_ExternalFunctions* externals);

void _dplg_generate_inst(DPL_Program* program, void* instruction)
{
    dplp_write(program, (DPL_Instruction_Kind) instruction);
}

void dpl_init(DPL *dpl, DPL_ExternalFunctions* externals)
{
    // CATALOGS

    /// TYPES
    dpl->types.number_handle = _dplt_register_by_name(dpl, nob_sv_from_cstr("number"));
    dpl->types.string_handle = _dplt_register_by_name(dpl, nob_sv_from_cstr("string"));

    /// FUNCTIONS AND GENERATORS

    DPL_Signature unary_number = {0};
    _dpl_add_handle(&unary_number.arguments, dpl->types.number_handle);
    unary_number.returns = dpl->types.number_handle;

    DPL_Signature binary_number = {0};
    _dpl_add_handle(&binary_number.arguments, dpl->types.number_handle);
    _dpl_add_handle(&binary_number.arguments, dpl->types.number_handle);
    binary_number.returns = dpl->types.number_handle;

    DPL_Signature binary_string = {0};
    _dpl_add_handle(&binary_string.arguments, dpl->types.string_handle);
    _dpl_add_handle(&binary_string.arguments, dpl->types.string_handle);
    binary_string.returns = dpl->types.string_handle;


    // unary operators
    _dplf_register(dpl, nob_sv_from_cstr("negate"), &unary_number, _dplg_generate_inst, (void*) INST_NEGATE);

    // binary operators
    _dplf_register(dpl, nob_sv_from_cstr("add"), &binary_number, _dplg_generate_inst, (void*) INST_ADD);
    _dplf_register(dpl, nob_sv_from_cstr("add"), &binary_string, _dplg_generate_inst, (void*) INST_ADD);
    _dplf_register(dpl, nob_sv_from_cstr("subtract"), &binary_number, _dplg_generate_inst, (void*) INST_SUBTRACT);
    _dplf_register(dpl, nob_sv_from_cstr("multiply"), &binary_number, _dplg_generate_inst, (void*) INST_MULTIPLY);
    _dplf_register(dpl, nob_sv_from_cstr("divide"), &binary_number, _dplg_generate_inst, (void*) INST_DIVIDE);

    if (externals != NULL) {
        _dple_register(dpl, externals);
    }

    if (dpl->debug)
    {
        printf("Types:\n");
        for (size_t i = 0; i < dpl->types.count; ++i) {
            printf("* %u: ", dpl->types.items[i].handle);
            _dplt_print(stdout, dpl, &dpl->types.items[i]);
            printf("\n");
        }
        printf("\n");
    }

    if (dpl->debug)
    {
        printf("Functions:\n");
        for (size_t i = 0; i < dpl->functions.count; ++i) {
            printf("* %u: ", dpl->functions.items[i].handle);
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

void _dpl_add_handle(DPL_Handles* handles, DPL_Handle handle)
{
    if (handles->count >= DPL_HANDLES_CAPACITY) {
        fprintf(stderr, "Excceded maximum capacity of handles");
        exit(1);
    }

    handles->items[handles->count] = handle;
    ++handles->count;
}

bool _dpl_handles_equal(DPL_Handles* first, DPL_Handles* second)
{
    if (first->count != second->count) {
        return false;
    }

    for (size_t i = 0; i < first->count; ++i) {
        if (first->items[i] != second->items[i]) {
            return false;
        }
    }

    return true;
}

DPL_Handle _dplt_add(DPL* dpl, DPL_Type type) {
    size_t index = dpl->types.count;
    nob_da_append(&dpl->types, type);

    return index;
}

DPL_Handle _dplt_register_by_name(DPL* dpl, Nob_String_View name)
{
    DPL_Type type = {
        .name = name,
        .handle = dpl->types.count + 1,
        .hash = _dplt_hash(name),
    };
    _dplt_add(dpl, type);
    return type.handle;
}

DPL_Handle _dplt_register(DPL* dpl, DPL_Type type)
{
    type.handle = dpl->types.count + 1;
    type.hash = _dplt_hash(type.name);
    _dplt_add(dpl, type);
    return type.handle;
}

DPL_Type* _dplt_find_by_handle(DPL *dpl, DPL_Handle handle)
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

DPL_Type* _dplt_find_by_signature(DPL* dpl, DPL_Handles* arguments, DPL_Handle returns)
{
    for (size_t i = 0;  i < dpl->types.count; ++i)
    {
        DPL_Type *type = &dpl->types.items[i];
        if (type->kind == TYPE_FUNCTION
                && _dpl_handles_equal(&type->as.function.arguments, arguments)
                && type->as.function.returns == returns)
        {
            return type;
        }
    }

    return NULL;
}

bool _dpl_add_handle_by_name(DPL_Handles* handles, DPL* dpl, Nob_String_View name)
{
    DPL_Type* type = _dplt_find_by_name(dpl, name);
    if (!type) {
        return false;
    }

    _dpl_add_handle(handles, type->handle);
    return true;
}

void _dpl_print_signature(FILE* out, DPL* dpl, DPL_Signature* signature)
{
    fprintf(out, "(");
    for (size_t i = 0; i < signature->arguments.count; ++i) {
        if (i > 0) {
            fprintf(out, ", ");
        }
        _dplt_print(out, dpl,
                    _dplt_find_by_handle(dpl, signature->arguments.items[i]));
    }
    fprintf(out, ")");

    fprintf(out, ": ");
    _dplt_print(out, dpl, _dplt_find_by_handle(dpl, signature->returns));
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
        _dpl_print_signature(out, dpl, &type->as.function);
        fprintf(out, "]");
        break;
    default:
        break;
    }
}

/// FUNCTIONS

DPL_Handle _dplf_register(DPL* dpl,
                          Nob_String_View name,
                          DPL_Signature* signature,
                          DPL_Generator_Callback generator_callback,
                          void* generator_user_data)
{
    DPL_Function function = {
        .handle = dpl->functions.count + 1,
        .name = name,
        .signature = *signature,
        .generator = {
            .callback = generator_callback,
            .user_data = generator_user_data,
        },
    };
    nob_da_append(&dpl->functions, function);

    return function.handle;
}

DPL_Function* _dplf_find_by_handle(DPL *dpl, DPL_Handle handle)
{
    for (size_t i = 0;  i < dpl->functions.count; ++i) {
        if (dpl->functions.items[i].handle == handle) {
            return &dpl->functions.items[i];
        }
    }
    return 0;
}

DPL_Function* _dplf_find_by_signature(DPL *dpl,
                                      Nob_String_View name, DPL_Handles* arguments)
{
    for (size_t i = 0; i < dpl->functions.count; ++i) {
        DPL_Function* function = &dpl->functions.items[i];
        if (nob_sv_eq(function->name, name)) {
            if (_dpl_handles_equal(&function->signature.arguments, arguments)) {
                return function;
            }
        }
    }
    return 0;
}

DPL_Function* _dplf_find_by_signature1(DPL *dpl,
                                       Nob_String_View name, DPL_Handle arg0)
{
    for (size_t i = 0; i < dpl->functions.count; ++i) {
        DPL_Function* function = &dpl->functions.items[i];
        if (nob_sv_eq(function->name, name)) {
            if (function->signature.arguments.count == 1
                    && function->signature.arguments.items[0] == arg0
               ) {
                return function;
            }
        }
    }

    return 0;
}

DPL_Function* _dplf_find_by_signature2(DPL *dpl,
                                       Nob_String_View name, DPL_Handle arg0, DPL_Handle arg1)
{
    for (size_t i = 0; i < dpl->functions.count; ++i) {
        DPL_Function* function = &dpl->functions.items[i];
        if (nob_sv_eq(function->name, name)) {
            if (function->signature.arguments.count == 2
                    && function->signature.arguments.items[0] == arg0
                    && function->signature.arguments.items[1] == arg1
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
    _dpl_print_signature(out, dpl, &function->signature);
}

// EXTERNALS

void _dplg_call_external_callback(DPL_Program* program, void* user_data)
{
    dplp_write_call_external(program, (size_t)user_data);
}

void _dple_register(DPL *dpl, DPL_ExternalFunctions* externals)
{
    for (size_t i = 0; i < externals->count; ++i)
    {
        DPL_ExternalFunction* external = &externals->items[i];

        // find the function type

        DPL_Signature signature = {0};

        DPL_Type* returns = _dplt_find_by_name(dpl, nob_sv_from_cstr(external->return_type));
        if (returns == NULL) {
            fprintf(stderr, "ERROR: Cannot register external function `%s`: return type `%s` cannot be resolved.\n",
                    external->name, external->return_type);
            exit(1);
        }
        signature.returns = returns->handle;

        for (size_t j = 0; j < external->argument_types.count; ++j) {
            DPL_Type* argument = _dplt_find_by_name(dpl, nob_sv_from_cstr(external->argument_types.items[j]));
            if (argument == NULL) {
                fprintf(stderr, "ERROR: Cannot register external function `%s`: argument type `%s` cannot be resolved.\n",
                        external->name, external->argument_types.items[j]);
                exit(1);
            }

            _dpl_add_handle(&signature.arguments, argument->handle);
        }

        _dplf_register(dpl, nob_sv_from_cstr(external->name), &signature,
                       _dplg_call_external_callback, (void*)i);
    }
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

bool _dpll_is_ident_begin(char c) {
    return c == '_' ||  isalpha(c);
}

bool _dpll_is_ident(char c) {
    return c == '_' ||  isalnum(c);
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
        return _dpll_build_token(dpl, TOKEN_DOT);
    case ':':
        _dpll_advance(dpl);
        if (_dpll_current(dpl) == '=') {
            _dpll_advance(dpl);
            return _dpll_build_token(dpl, TOKEN_COLON_EQUAL);
        }
        return _dpll_build_token(dpl, TOKEN_COLON);
    case '(':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_OPEN_PAREN);
    case ')':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_CLOSE_PAREN);
    case '{':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_OPEN_BRACE);
    case '}':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_CLOSE_BRACE);
    case ',':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_COMMA);
    case ';':
        _dpll_advance(dpl);
        return _dpll_build_token(dpl, TOKEN_SEMICOLON);
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
        }

        return t;
    }


    if (_dpll_current(dpl) == '"')
    {
        _dpll_advance(dpl);

        bool in_escape = false;
        while (true) {
            if (_dpll_is_eof(dpl)) {
                _dpll_error(dpl, "Untermintated string literal.\n");
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
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_KEYWORD_CONSTANT:
        return "CONSTANT";

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

    case TOKEN_DOT:
        return "DOT";
    case TOKEN_COLON:
        return "COLON";
    case TOKEN_COLON_EQUAL:
        return "COLON_EQUAL";

    case TOKEN_OPEN_PAREN:
        return "OPEN_PAREN";
    case TOKEN_CLOSE_PAREN:
        return "CLOSE_PAREN";
    case TOKEN_OPEN_BRACE:
        return "OPEN_BRACE";
    case TOKEN_CLOSE_BRACE:
        return "CLOSE_BRACE";
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_SEMICOLON:
        return "SEMICOLON";

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
    case AST_NODE_FUNCTIONCALL:
        return "AST_NODE_FUNCTIONCALL";
    case AST_NODE_SCOPE:
        return "AST_NODE_SCOPE";
    case AST_NODE_DECLARATION:
        return "AST_NODE_DECLARATION";
    case AST_NODE_SYMBOL:
        return "AST_NODE_SYMBOL";
    }

    assert(false && "unreachable: _dpla_node_kind_name");
    return "";
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
        printf(" [%s "SV_Fmt": "SV_Fmt"]\n", _dpll_token_kind_name(declaration.keyword.kind), SV_Arg(declaration.name.text), SV_Arg(declaration.type.text));
        _dpla_print_indent(level + 1);
        printf("<initialization>\n");
        _dpla_print(declaration.initialization, level + 2);
    }
    break;
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
DPL_Ast_Node* _dplp_parse_scope(DPL* dpl, DPL_TokenKind closing_token);

DPL_Ast_Node* _dplp_parse_declaration(DPL* dpl) {
    DPL_Token keyword_candidate = _dplp_peek_token(dpl);
    if (keyword_candidate.kind == TOKEN_KEYWORD_CONSTANT) {
        DPL_Ast_Node* declaration = _dpla_create_node(&dpl->tree, AST_NODE_DECLARATION);
        declaration->as.declaration.keyword = _dplp_next_token(dpl);
        declaration->as.declaration.name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);
        _dplp_expect_token(dpl, TOKEN_COLON);
        declaration->as.declaration.type = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);
        declaration->as.declaration.assignment = _dplp_expect_token(dpl, TOKEN_COLON_EQUAL);
        declaration->as.declaration.initialization = _dplp_parse_expression(dpl);
        return declaration;
    }

    return _dplp_parse_expression(dpl);
}

typedef struct
{
    DPL_Ast_Node** items;
    size_t count;
    size_t capacity;
} _DPL_Ast_NodeList;

_DPL_Ast_NodeList _dplp_parse_expressions(DPL* dpl, DPL_TokenKind delimiter, DPL_TokenKind closing, bool allow_declarations)
{
    _DPL_Ast_NodeList list = {0};
    if (allow_declarations) {
        nob_da_append(&list, _dplp_parse_declaration(dpl));
    } else {
        nob_da_append(&list, _dplp_parse_expression(dpl));
    }

    DPL_Token delimiter_candidate = _dplp_peek_token(dpl);
    while (delimiter_candidate.kind == delimiter) {
        _dplp_next_token(dpl);
        if (_dplp_peek_token(dpl).kind == closing) {
            break;
        }

        if (allow_declarations) {
            nob_da_append(&list, _dplp_parse_declaration(dpl));
        } else {
            nob_da_append(&list, _dplp_parse_expression(dpl));
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
    case TOKEN_OPEN_BRACE: {
        DPL_Ast_Node* node = _dplp_parse_scope(dpl, TOKEN_CLOSE_BRACE);
        return node;
    }
    case TOKEN_IDENTIFIER: {
        if (_dplp_peek_token(dpl).kind == TOKEN_OPEN_PAREN) {
            DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_FUNCTIONCALL);
            node->as.function_call.name = token;

            _dplp_expect_token(dpl, TOKEN_OPEN_PAREN);

            if (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_PAREN) {
                _DPL_Ast_NodeList arguments = _dplp_parse_expressions(dpl, TOKEN_COMMA, TOKEN_CLOSE_PAREN, false);
                if (arguments.count > 0) {
                    node->as.function_call.argument_count = arguments.count;
                    node->as.function_call.arguments = arena_alloc(
                                                           &dpl->tree.memory, sizeof(DPL_Ast_Node*) * arguments.count);
                    memcpy(node->as.function_call.arguments, arguments.items, sizeof(DPL_Ast_Node*) * arguments.count);
                    nob_da_free(arguments);
                }
            }

            _dplp_expect_token(dpl, TOKEN_CLOSE_PAREN);
            return node;
        }

        DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_SYMBOL);
        node->as.symbol = token;
        return node;
    }
    default: {
        _dplp_error_unexpected_token(dpl, token);
    }
    }
}

DPL_Ast_Node *_dplp_parser_dot(DPL* dpl)
{
    DPL_Ast_Node* expression = _dplp_parse_primary(dpl);

    DPL_Token operator_candidate = _dplp_peek_token(dpl);
    while (operator_candidate.kind == TOKEN_DOT) {
        _dplp_next_token(dpl);
        DPL_Ast_Node* new_expression = _dplp_parse_primary(dpl);

        if (new_expression->kind != AST_NODE_FUNCTIONCALL) {
            fprintf(stderr, LOC_Fmt": Right-hand operand of `.` operator must be a function call.\n",
                    LOC_Arg(operator_candidate.location));
            _dpll_print_token_location(stderr, dpl, operator_candidate);
            exit(1);
        }

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

        operator_candidate = _dplp_peek_token(dpl);
    }

    return expression;
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

    return _dplp_parser_dot(dpl);
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

DPL_Ast_Node* _dplp_parse_scope(DPL* dpl, DPL_TokenKind closing_token)
{
    DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_SCOPE);
    node->as.scope.expression_count = 0;

    if (_dplp_peek_token(dpl).kind != closing_token) {

        _DPL_Ast_NodeList expressions = _dplp_parse_expressions(dpl, TOKEN_SEMICOLON, closing_token, true);
        if (expressions.count > 0) {
            node->as.scope.expression_count = expressions.count;
            node->as.scope.expressions = arena_alloc(
                                             &dpl->tree.memory, sizeof(DPL_Ast_Node*) * expressions.count);
            memcpy(node->as.scope.expressions, expressions.items, sizeof(DPL_Ast_Node*) * expressions.count);
            nob_da_free(expressions);
        }
    }
    _dplp_expect_token(dpl, closing_token);

    return node;
}

void _dplp_parse(DPL* dpl)
{
    dpl->tree.root = _dplp_parse_scope(dpl, TOKEN_EOF); //_dplp_parse_expression(dpl);
    //_dplp_expect_token(dpl, TOKEN_EOF);
}

// CALLTREE

void _dplc_symbols_begin_scope(DPL* dpl) {
    DPL_SymbolStack* s = &dpl->symbol_stack;
    nob_da_append(&s->frames, s->symbols.count);
}

void _dplc_symbols_end_scope(DPL* dpl) {
    DPL_SymbolStack* s = &dpl->symbol_stack;
    s->frames.count--;
    s->symbols.count = s->frames.items[s->frames.count];
}

DPL_Symbol* _dplc_symbols_lookup(DPL* dpl, Nob_String_View name) {
    DPL_SymbolStack* s = &dpl->symbol_stack;
    for (size_t i = s->symbols.count; i > 0; --i) {
        if (nob_sv_eq(s->symbols.items[i - 1].name, name)) {
            return &s->symbols.items[i - 1];
        }
    }
    return NULL;
}

DPL_CallTree_Node* _dplc_bind_node(DPL* dpl, DPL_Ast_Node* node);

DPL_CallTree_Node* _dplc_bind_unary(DPL* dpl, DPL_Ast_Node* node, const char* function_name)
{
    DPL_CallTree_Node* operand = _dplc_bind_node(dpl, node->as.unary.operand);
    if (!operand) {
        fprintf(stderr, "ERROR: Cannot bind operand of unary expression.\n");
        exit(1);
    }

    DPL_Function* function = _dplf_find_by_signature1(
                                 dpl,
                                 nob_sv_from_cstr(function_name),
                                 operand->type_handle);
    if (function) {
        DPL_CallTree_Node* calltree_node = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
        calltree_node->kind = CALLTREE_NODE_FUNCTION;
        calltree_node->type_handle = function->signature.returns;

        calltree_node->as.function.function_handle = function->handle;
        nob_da_append(&calltree_node->as.function.arguments, operand);

        return calltree_node;
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
    if (!lhs) {
        fprintf(stderr, "ERROR: Cannot bind left-hand side of binary expression.\n");
        exit(1);
    }
    DPL_CallTree_Node* rhs = _dplc_bind_node(dpl, node->as.binary.right);
    if (!rhs) {
        fprintf(stderr, "ERROR: Cannot bind right-hand side of binary expression.\n");
        exit(1);
    }

    DPL_Function* function = _dplf_find_by_signature2(
                                 dpl,
                                 nob_sv_from_cstr(function_name),
                                 lhs->type_handle, rhs->type_handle);
    if (function) {
        DPL_CallTree_Node* calltree_node = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
        calltree_node->kind = CALLTREE_NODE_FUNCTION;
        calltree_node->type_handle = function->signature.returns;

        calltree_node->as.function.function_handle = function->handle;
        nob_da_append(&calltree_node->as.function.arguments, lhs);
        nob_da_append(&calltree_node->as.function.arguments, rhs);

        return calltree_node;
    }

    DPL_Type* lhs_type = _dplt_find_by_handle(dpl, lhs->type_handle);
    DPL_Type* rhs_type = _dplt_find_by_handle(dpl, rhs->type_handle);

    DPL_Token operator_token = node->as.binary.operator;
    fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function \"%s("SV_Fmt", "SV_Fmt")\" for binary operator \""SV_Fmt"\".\n",
            LOC_Arg(operator_token.location), function_name, SV_Arg(lhs_type->name), SV_Arg(rhs_type->name), SV_Arg(operator_token.text));
    _dpll_print_token_location(stderr, dpl, operator_token);

    exit(1);
}

DPL_CallTree_Node* _dplc_bind_function_call(DPL* dpl, DPL_Ast_Node* node)
{
    DPL_CallTree_Node* result_ctn = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
    result_ctn->kind = CALLTREE_NODE_FUNCTION;

    DPL_Handles argument_types = {0};

    DPL_Ast_FunctionCall fc = node->as.function_call;
    for (size_t i = 0; i < fc.argument_count; ++i) {
        DPL_CallTree_Node* arg_ctn = _dplc_bind_node(dpl, fc.arguments[i]);
        if (!arg_ctn) {
            fprintf(stderr, "ERROR: Cannot argument of function call.\n");
            exit(1);
        }
        nob_da_append(&result_ctn->as.function.arguments, arg_ctn);
        _dpl_add_handle(&argument_types, arg_ctn->type_handle);
    }

    DPL_Function* function = _dplf_find_by_signature(dpl, fc.name.text, &argument_types);
    if (function) {
        result_ctn->as.function.function_handle = function->handle;
        result_ctn->type_handle = function->signature.returns;
        return result_ctn;
    }

    fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function `"SV_Fmt"(",
            LOC_Arg(fc.name.location), SV_Arg(fc.name.text));
    DPL_CallTree_Nodes arguments = result_ctn->as.function.arguments;
    for (size_t i = 0; i < arguments.count; ++i) {
        if (i > 0) {
            fprintf(stderr, ", ");
        }

        DPL_Type *argument_type = _dplt_find_by_handle(dpl, arguments.items[i]->type_handle);
        fprintf(stderr, SV_Fmt, SV_Arg(argument_type->name));
    }
    fprintf(stderr, ")`.\n");
    _dpll_print_token_location(stderr, dpl, fc.name);
    exit(1);
}

DPL_CallTree_Node* _dplc_bind_scope(DPL* dpl, DPL_Ast_Node* node)
{
    _dplc_symbols_begin_scope(dpl);

    DPL_CallTree_Node* result_ctn = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
    result_ctn->kind = CALLTREE_NODE_SCOPE;

    DPL_Ast_Scope scope = node->as.scope;
    for (size_t i = 0; i < scope.expression_count; ++i) {
        DPL_CallTree_Node* expr_ctn = _dplc_bind_node(dpl, scope.expressions[i]);
        if (!expr_ctn) {
            continue;
        }

        nob_da_append(&result_ctn->as.scope.expressions, expr_ctn);
        result_ctn->type_handle = expr_ctn->type_handle;
    }

    _dplc_symbols_end_scope(dpl);
    return result_ctn;
}

Nob_String_View _dplc_unescape_string(DPL* dpl, Nob_String_View escaped_string) {
    // unescape source string literal
    char* unescaped_string = arena_alloc(&dpl->calltree.memory, sizeof(char) * (escaped_string.count - 2 + 1));
    // -2 for quotes; +1 for terminating null byte

    const char *source_pos = escaped_string.data + 1;
    const char *source_end = escaped_string.data + escaped_string.count - 2;
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

DPL_CallTree_Value _dplc_fold_constant(DPL* dpl, DPL_Ast_Node* node) {
    switch (node->kind) {
    case AST_NODE_LITERAL: {
        DPL_Token value = node->as.literal.value;
        switch (value.kind) {
        case TOKEN_NUMBER:
            return (DPL_CallTree_Value) {
                .type_handle = dpl->types.number_handle,
                .as.number = atof(nob_temp_sv_to_cstr(value.text)),
            };
        case TOKEN_STRING:
            return (DPL_CallTree_Value) {
                .type_handle = dpl->types.string_handle,
                .as.string = _dplc_unescape_string(dpl, value.text),
            };
        default:
            fprintf(stderr, "ERROR: Cannot fold literal constant of type `%s`.\n",
                    _dpll_token_kind_name(value.kind));
            exit(1);
        }
    }
    break;
    default:
        break;
    }

    fprintf(stderr, "ERROR: Cannot fold constant of type `%s`.\n",
            _dpla_node_kind_name(node->kind));
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
            calltree_node->as.value = _dplc_fold_constant(dpl, node);
            return calltree_node;
        }
        case TOKEN_STRING: {
            DPL_CallTree_Node* calltree_node = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
            calltree_node->kind = CALLTREE_NODE_VALUE;
            calltree_node->type_handle = dpl->types.string_handle;
            calltree_node->as.value = _dplc_fold_constant(dpl, node);
            return calltree_node;
        }
        break;
        default:
            fprintf(stderr, "ERROR: Cannot resolve literal type for token of kind \"%s\".\n",
                    _dpll_token_kind_name(node->as.literal.value.kind));
            exit(1);
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
    case AST_NODE_FUNCTIONCALL:
        return _dplc_bind_function_call(dpl, node);
    case AST_NODE_SCOPE:
        return _dplc_bind_scope(dpl, node);
    case AST_NODE_DECLARATION: {
        DPL_Ast_Declaration* decl = &node->as.declaration;
        DPL_Symbol s = {
            .kind = SYMBOL_CONSTANT,
            .name = decl->name.text,
            .as.constant =  _dplc_fold_constant(dpl, decl->initialization),
        };

        DPL_Type* declared_type = _dplt_find_by_name(dpl, decl->type.text);
        if (!declared_type) {
            fprintf(stderr, LOC_Fmt": ERROR: Unknown type `"SV_Fmt"` in declaration of constant `"SV_Fmt"`.\n", LOC_Arg(decl->type.location), SV_Arg(decl->type.text), SV_Arg(decl->name.text));
            _dpll_print_token_location(stderr, dpl, decl->type);
            exit(1);
        }

        DPL_Type* unfolded_type = _dplt_find_by_handle(dpl,  s.as.constant.type_handle);
        if (unfolded_type->handle != declared_type->handle) {
            fprintf(stderr, LOC_Fmt": ERROR: Declared type `"SV_Fmt"` does not match folded type `"SV_Fmt"` in declaration of constant `"SV_Fmt"`.\n",
                    LOC_Arg(decl->assignment.location), SV_Arg(declared_type->name), SV_Arg(unfolded_type->name), SV_Arg(decl->name.text));
            _dpll_print_token_location(stderr, dpl, decl->assignment);
            exit(1);
        }

        nob_da_append(&dpl->symbol_stack.symbols, s);

        return NULL;
    }
    break;
    case AST_NODE_SYMBOL: {
        DPL_Symbol* symbol = _dplc_symbols_lookup(dpl, node->as.symbol.text);
        if (!symbol) {
            fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve symbol `"SV_Fmt"` in current scope.\n",
                    LOC_Arg(node->as.symbol.location), SV_Arg(node->as.symbol.text));
            _dpll_print_token_location(stderr, dpl, node->as.symbol);
            exit(1);
        }

        DPL_CallTree_Node* calltree_node = arena_alloc(&dpl->calltree.memory, sizeof(DPL_CallTree_Node));
        calltree_node->kind = CALLTREE_NODE_VALUE;
        calltree_node->type_handle = symbol->as.constant.type_handle;
        calltree_node->as.value = symbol->as.constant;
        return calltree_node;
    }
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
    if (type) {
        printf("["SV_Fmt"] ", SV_Arg(type->name));
    } else {
        printf("[<unknown>] ");
    }

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
        printf("Value `");
        if (node->as.value.type_handle == dpl->types.number_handle) {
            printf("%f", node->as.value.as.number);
        } else if (node->as.value.type_handle == dpl->types.string_handle) {
            Nob_String_View value = node->as.value.as.string;
            dplp_print_escaped_string(value.data, value.count);
        }
        printf("`\n");
    }
    break;
    case CALLTREE_NODE_SCOPE: {
        printf("$scope(\n");

        for (size_t i = 0; i < node->as.scope.expressions.count; ++i) {
            _dplc_print(dpl, node->as.scope.expressions.items[i], level + 1);
        }

        for (size_t i = 0; i < level; ++i) {
            printf("  ");
        }
        printf(")\n");
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
            dplp_write_push_number(program, node->as.value.as.number);
        } else if (node->type_handle == dpl->types.string_handle) {
            dplp_write_push_string(program, node->as.value.as.string.data);
        } else {
            DPL_Type* type = _dplt_find_by_handle(dpl, node->type_handle);
            fprintf(stderr, "Cannot generate program for value node of type "SV_Fmt".\n", SV_Arg(type->name));
            exit(1);
        }
    }
    break;
    case CALLTREE_NODE_FUNCTION: {
        DPL_CallTree_Function f = node->as.function;
        for (size_t i = 0; i < f.arguments.count; ++i) {
            _dplg_generate(dpl, f.arguments.items[i], program);
        }

        DPL_Function* function = _dplf_find_by_handle(dpl, f.function_handle);
        function->generator.callback(program, function->generator.user_data);
    }
    break;
    case CALLTREE_NODE_SCOPE: {
        DPL_CallTree_Scope s = node->as.scope;
        for (size_t i = 0; i < s.expressions.count; ++i) {
            if (i > 0) {
                dplp_write_pop(program);
            }
            _dplg_generate(dpl, s.expressions.items[i], program);
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