#ifdef DPL_LEAKCHECK
#   include "stb_leakcheck.h"
#endif

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


// Forward declarations needed for initialization
void _dpl_add_handle(DPL_Handles* handles, DPL_Handle handle);

DPL_Handle _dplt_register(DPL* dpl, DPL_Type type);
DPL_Handle _dplt_register_by_name(DPL* dpl, Nob_String_View name);
void _dplt_print(FILE* out, DPL* dpl, DPL_Type* type);

DPL_Handle _dplf_register(DPL* dpl,
                          Nob_String_View name,
                          DPL_Signature* signature,
                          DPL_Generator_Callback generator_callback,
                          void* generator_user_data);
void _dplf_print(FILE* out, DPL* dpl, DPL_Function* function);

void _dple_register(DPL *dpl, DPL_ExternalFunctions externals);

void _dplg_generate_inst(DPL* dpl, DPL_Program* program, void* instruction)
{
    (void) dpl;
    dplp_write(program, (DPL_Instruction_Kind) instruction);
}

void dpl_init(DPL *dpl, DPL_ExternalFunctions externals)
{
    // CATALOGS

    /// TYPES
    dpl->number_type_handle = _dplt_register_by_name(dpl, nob_sv_from_cstr("number"));
    dpl->string_type_handle = _dplt_register_by_name(dpl, nob_sv_from_cstr("string"));
    dpl->boolean_type_handle = _dplt_register_by_name(dpl, nob_sv_from_cstr("boolean"));
    dpl->none_type_handle = _dplt_register_by_name(dpl, nob_sv_from_cstr("none"));

    /// FUNCTIONS AND GENERATORS

    DPL_Signature unary_number = {0};
    _dpl_add_handle(&unary_number.arguments, dpl->number_type_handle);
    unary_number.returns = dpl->number_type_handle;

    DPL_Signature binary_number = {0};
    _dpl_add_handle(&binary_number.arguments, dpl->number_type_handle);
    _dpl_add_handle(&binary_number.arguments, dpl->number_type_handle);
    binary_number.returns = dpl->number_type_handle;

    DPL_Signature binary_string = {0};
    _dpl_add_handle(&binary_string.arguments, dpl->string_type_handle);
    _dpl_add_handle(&binary_string.arguments, dpl->string_type_handle);
    binary_string.returns = dpl->string_type_handle;

    DPL_Signature comparison_number = {0};
    _dpl_add_handle(&comparison_number.arguments, dpl->number_type_handle);
    _dpl_add_handle(&comparison_number.arguments, dpl->number_type_handle);
    comparison_number.returns = dpl->boolean_type_handle;

    DPL_Signature comparison_string = {0};
    _dpl_add_handle(&comparison_string.arguments, dpl->string_type_handle);
    _dpl_add_handle(&comparison_string.arguments, dpl->string_type_handle);
    comparison_string.returns = dpl->boolean_type_handle;

    DPL_Signature unary_boolean = {0};
    _dpl_add_handle(&unary_boolean.arguments, dpl->boolean_type_handle);
    unary_boolean.returns = dpl->boolean_type_handle;

    DPL_Signature comparison_boolean = {0};
    _dpl_add_handle(&comparison_boolean.arguments, dpl->boolean_type_handle);
    _dpl_add_handle(&comparison_boolean.arguments, dpl->boolean_type_handle);
    comparison_boolean.returns = dpl->boolean_type_handle;

    // unary operators
    _dplf_register(dpl, nob_sv_from_cstr("negate"), &unary_number, _dplg_generate_inst, (void*) INST_NEGATE);
    _dplf_register(dpl, nob_sv_from_cstr("not"), &unary_boolean, _dplg_generate_inst, (void*) INST_NOT);

    // binary operators
    _dplf_register(dpl, nob_sv_from_cstr("add"), &binary_number, _dplg_generate_inst, (void*) INST_ADD);
    _dplf_register(dpl, nob_sv_from_cstr("add"), &binary_string, _dplg_generate_inst, (void*) INST_ADD);
    _dplf_register(dpl, nob_sv_from_cstr("subtract"), &binary_number, _dplg_generate_inst, (void*) INST_SUBTRACT);
    _dplf_register(dpl, nob_sv_from_cstr("multiply"), &binary_number, _dplg_generate_inst, (void*) INST_MULTIPLY);
    _dplf_register(dpl, nob_sv_from_cstr("divide"), &binary_number, _dplg_generate_inst, (void*) INST_DIVIDE);

    // comparison operators
    _dplf_register(dpl, nob_sv_from_cstr("less"), &comparison_number, _dplg_generate_inst, (void*) INST_LESS);
    _dplf_register(dpl, nob_sv_from_cstr("less_equal"), &comparison_number, _dplg_generate_inst, (void*) INST_LESS_EQUAL);
    _dplf_register(dpl, nob_sv_from_cstr("greater"), &comparison_number, _dplg_generate_inst, (void*) INST_GREATER);
    _dplf_register(dpl, nob_sv_from_cstr("greater_equal"), &comparison_number, _dplg_generate_inst, (void*) INST_GREATER_EQUAL);
    _dplf_register(dpl, nob_sv_from_cstr("equal"), &comparison_number, _dplg_generate_inst, (void*) INST_EQUAL);
    _dplf_register(dpl, nob_sv_from_cstr("not_equal"), &comparison_number, _dplg_generate_inst, (void*) INST_NOT_EQUAL);
    _dplf_register(dpl, nob_sv_from_cstr("equal"), &comparison_string, _dplg_generate_inst, (void*) INST_EQUAL);
    _dplf_register(dpl, nob_sv_from_cstr("not_equal"), &comparison_string, _dplg_generate_inst, (void*) INST_NOT_EQUAL);
    _dplf_register(dpl, nob_sv_from_cstr("equal"), &comparison_boolean, _dplg_generate_inst, (void*) INST_EQUAL);
    _dplf_register(dpl, nob_sv_from_cstr("not_equal"), &comparison_boolean, _dplg_generate_inst, (void*) INST_NOT_EQUAL);

    if (externals != NULL) {
        _dple_register(dpl, externals);
    }

    // lexer initialization
    dpl->current_line = dpl->source.data;
}

void dpl_free(DPL *dpl)
{
    // catalog freeing
    da_free(dpl->types);
    da_free(dpl->functions);
    da_free(dpl->user_functions);

    // parser freeing
    arena_free(&dpl->tree.memory);

    // bound tree freeing
    arena_free(&dpl->bound_tree.memory);

    // symbol stack freeing
    da_free(dpl->symbol_stack.symbols);
    da_free(dpl->symbol_stack.frames);

    // scope stack freeing
    da_free(dpl->scope_stack);
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
        DPL_ERROR("Excceded maximum capacity of handles.");
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
    size_t index = da_size(dpl->types);
    da_add(dpl->types, type);

    return index;
}

DPL_Handle _dplt_register_by_name(DPL* dpl, Nob_String_View name)
{
    DPL_Type type = {
        .name = name,
        .handle = da_size(dpl->types) + 1,
        .hash = _dplt_hash(name),
    };
    _dplt_add(dpl, type);
    return type.handle;
}

DPL_Handle _dplt_register(DPL* dpl, DPL_Type type)
{
    type.handle = da_size(dpl->types) + 1;
    type.hash = _dplt_hash(type.name);
    _dplt_add(dpl, type);
    return type.handle;
}

DPL_Type* _dplt_find_by_handle(DPL *dpl, DPL_Handle handle)
{
    for (size_t i = 0;  i < da_size(dpl->types); ++i) {
        if (dpl->types[i].handle == handle) {
            return &dpl->types[i];
        }
    }
    return 0;
}

DPL_Type* _dplt_find_by_name(DPL *dpl, Nob_String_View name)
{
    for (size_t i = 0;  i < da_size(dpl->types); ++i) {
        if (nob_sv_eq(dpl->types[i].name, name)) {
            return &dpl->types[i];
        }
    }
    return 0;
}

DPL_Type* _dplt_find_by_signature(DPL* dpl, DPL_Handles* arguments, DPL_Handle returns)
{
    for (size_t i = 0;  i < da_size(dpl->types); ++i)
    {
        DPL_Type *type = &dpl->types[i];
        if (type->kind == TYPE_FUNCTION
                && _dpl_handles_equal(&type->as.function.arguments, arguments)
                && type->as.function.returns == returns)
        {
            return type;
        }
    }

    return NULL;
}

DPL_Type* _dplt_find_by_object_query(DPL* dpl, DPL_TypeObjectQuery query)
{
    for (size_t i = 0;  i < da_size(dpl->types); ++i)
    {
        DPL_Type *type = &dpl->types[i];
        if (type->kind == TYPE_OBJECT && type->as.object.field_count == da_size(query)) {
            bool is_match = true;
            for (size_t j = 0; j < type->as.object.field_count; ++j) {
                if (!nob_sv_eq(query[j].name, type->as.object.fields[j].name)) {
                    is_match = false;
                    break;
                }
                if (query[j].type != type->as.object.fields[j].type) {
                    is_match = false;
                    break;
                }
            }
            if (is_match) {
                return type;
            }
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
    case TYPE_NAME:
        break;
    case TYPE_OBJECT:
        break;
    default:
        DW_UNIMPLEMENTED_MSG("%d", type->kind);
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
        .handle = da_size(dpl->functions) + 1,
        .name = name,
        .signature = *signature,
        .generator = {
            .callback = generator_callback,
            .user_data = generator_user_data,
        },
    };
    da_add(dpl->functions, function);

    return function.handle;
}

DPL_Function* _dplf_find_by_handle(DPL *dpl, DPL_Handle handle)
{
    for (size_t i = 0; i < da_size(dpl->functions); ++i) {
        if (dpl->functions[i].handle == handle) {
            return &dpl->functions[i];
        }
    }
    return 0;
}

DPL_Function* _dplf_find_by_signature(DPL *dpl,
                                      Nob_String_View name, DPL_Handles* arguments)
{
    for (size_t i = 0; i < da_size(dpl->functions); ++i) {
        DPL_Function* function = &dpl->functions[i];
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
    for (size_t i = 0; i < da_size(dpl->functions); ++i) {
        DPL_Function* function = &dpl->functions[i];
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
    for (size_t i = 0; i < da_size(dpl->functions); ++i) {
        DPL_Function* function = &dpl->functions[i];
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

void _dplg_call_external_callback(DPL* dpl, DPL_Program* program, void* user_data)
{
    (void) dpl;
    dplp_write_call_external(program, (size_t)user_data);
}

void _dple_register(DPL *dpl, DPL_ExternalFunctions externals)
{
    for (size_t i = 0; i < da_size(externals); ++i)
    {
        DPL_ExternalFunction* external = &externals[i];

        // find the function type

        DPL_Signature signature = {0};

        DPL_Type* returns = _dplt_find_by_name(dpl, nob_sv_from_cstr(external->return_type));
        if (returns == NULL) {
            DPL_ERROR("Cannot register external function `%s`: return type `%s` cannot be resolved.",
                      external->name, external->return_type);
        }
        signature.returns = returns->handle;

        for (size_t j = 0; j < da_size(external->argument_types); ++j) {
            DPL_Type* argument = _dplt_find_by_name(dpl, nob_sv_from_cstr(external->argument_types[j]));
            if (argument == NULL) {
                DPL_ERROR("Cannot register external function `%s`: argument type `%s` cannot be resolved.",
                          external->name, external->argument_types[j]);
            }

            _dpl_add_handle(&signature.arguments, argument->handle);
        }

        _dplf_register(dpl, nob_sv_from_cstr(external->name), &signature,
                       _dplg_call_external_callback, (void*)i);
    }
}


// LEXER

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
    DPL_Token token = (DPL_Token) {
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

    return token;
}

DPL_Token _dpll_build_empty_token(DPL* dpl, DPL_TokenKind kind)
{
    return (DPL_Token) {
        .kind = kind,
        .location = dpl->token_start_location,
        .text = SV_NULL,
    };
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
        return _dpll_build_token(dpl, TOKEN_OPEN_BRACE);
    case '}':
        _dpll_advance(dpl);
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
        }

        return t;
    }


    if (_dpll_current(dpl) == '"')
    {
        _dpll_advance(dpl);

        bool in_escape = false;
        while (true) {
            if (_dpll_is_eof(dpl)) {
                DPL_LEXER_ERROR(dpl, "Untermintated string literal.");
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

    DPL_LEXER_ERROR(dpl, "Invalid character '%c'.", _dpll_current(dpl));
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
        return "<none>";
    case TOKEN_EOF:
        return "<end of file>";
    case TOKEN_NUMBER:
        return "number literal";
    case TOKEN_STRING:
        return "string literal";
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

// AST

DPL_Ast_Node* _dpla_create_node(DPL_Ast_Tree* tree, DPL_AstNodeKind kind, DPL_Token first, DPL_Token last)
{
    DPL_Ast_Node* node = arena_alloc(&tree->memory, sizeof(DPL_Ast_Node));
    node->kind = kind;
    node->first = first;
    node->last = last;
    return node;
}

const char* _dpla_node_kind_name(DPL_AstNodeKind kind) {
    switch (kind) {
    case AST_NODE_LITERAL:
        return "AST_NODE_LITERAL";
    case AST_NODE_OBJECT_LITERAL:
        return "AST_NODE_OBJECT_LITERAL";
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
    case AST_NODE_ASSIGNMENT:
        return "AST_NODE_ASSIGNMENT";
    case AST_NODE_FUNCTION:
        return "AST_NODE_FUNCTION";
    case AST_NODE_CONDITIONAL:
        return "AST_NODE_CONDITIONAL";
    case AST_NODE_WHILE_LOOP:
        return "AST_NODE_WHILE_LOOP";
    case AST_NODE_FIELD_ACCESS:
        return "AST_NODE_FIELD_ACCESS";
    default:
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
}

void _dpla_build_type_name(DPL_Ast_Type* ast_type, Nob_String_Builder* sb) {
    switch (ast_type->kind) {
    case TYPE_NAME:
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
            printf("<field "SV_Fmt">\n", SV_Arg(object_literal.fields[i].name.text));
            _dpla_print(object_literal.fields[i].expression, level + 2);
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

DPL_Token _dplp_expect_token(DPL *dpl, DPL_TokenKind kind) {
    DPL_Token next_token = _dplp_next_token(dpl);
    if (next_token.kind != kind) {
        DPL_TOKEN_ERROR(dpl, next_token, "Unexpected %s, expected %s.",
                        _dpll_token_kind_name(next_token.kind), _dpll_token_kind_name(kind));
    }
    return next_token;
}

DPL_Ast_Node* _dplp_parse_expression(DPL* dpl);
DPL_Ast_Node* _dplp_parse_scope(DPL* dpl, DPL_Token opening_token, DPL_TokenKind closing_token_kind);

DPL_Ast_Type* _dplp_parse_type(DPL* dpl) {
    DPL_Token type_begin = _dplp_peek_token(dpl);
    switch (type_begin.kind) {
    case TOKEN_IDENTIFIER: {
        _dplp_next_token(dpl);

        DPL_Ast_Type* name_type = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_Type));
        name_type->kind = TYPE_NAME;
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
                .type = type,
            };
            da_add(tmp_fields, field);
        }


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
        _dplp_expect_token(dpl, TOKEN_COLON);
        DPL_Ast_Type* result_type = _dplp_parse_type(dpl);

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
    {
        DPL_Ast_Node* node = _dpla_create_node(&dpl->tree, AST_NODE_LITERAL, token, token);
        node->as.literal.value = token;
        return node;
    }
    break;
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
        da_array(DPL_Ast_ObjectLiteralField) tmp_fields = NULL;
        while (_dplp_peek_token(dpl).kind != TOKEN_CLOSE_BRACKET) {
            if (!first) {
                _dplp_expect_token(dpl, TOKEN_COMMA);
            }
            DPL_Token field_name = _dplp_expect_token(dpl, TOKEN_IDENTIFIER);
            _dplp_expect_token(dpl, TOKEN_COLON);
            DPL_Ast_Node* field_expression = _dplp_parse_expression(dpl);

            DPL_Ast_ObjectLiteralField field = {
                .name = field_name,
                .expression = field_expression,
            };
            da_add(tmp_fields, field);

            first = false;
        }

        DPL_Ast_Node* object_literal = _dpla_create_node(&dpl->tree, AST_NODE_OBJECT_LITERAL, token, _dplp_next_token(dpl));
        object_literal->as.object_literal.field_count = da_size(tmp_fields);
        if (da_size(tmp_fields) > 0) {
            object_literal->as.object_literal.fields = arena_alloc(&dpl->tree.memory, sizeof(DPL_Ast_ObjectLiteralField) * da_size(tmp_fields));
            memcpy(object_literal->as.object_literal.fields, tmp_fields, sizeof(DPL_Ast_ObjectLiteralField) * da_size(tmp_fields));
        }
        da_free(tmp_fields);
        return object_literal;
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
    if (operator_candidate.kind == TOKEN_MINUS || operator_candidate.kind == TOKEN_BANG) {
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
        if (target->kind != AST_NODE_SYMBOL) {
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
    case BOUND_NODE_FIELD_ACCESS:
        return "BOUND_NODE_FIELD_ACCESS";
    default:
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
}

void _dplb_symbols_begin_scope(DPL* dpl) {
    DPL_SymbolStack* s = &dpl->symbol_stack;
    da_add(s->frames, da_size(s->symbols));
}

void _dplb_symbols_end_scope(DPL* dpl) {
    DPL_SymbolStack* s = &dpl->symbol_stack;
    da_pop(s->frames);
    da_set_size(s->symbols, s->frames[da_size(s->frames)]);
}

DPL_Symbol* _dplb_symbols_lookup(DPL* dpl, Nob_String_View name) {
    DPL_SymbolStack* s = &dpl->symbol_stack;
    for (size_t i = da_size(s->symbols); i > s->bottom; --i) {
        if (nob_sv_eq(s->symbols[i - 1].name, name)) {
            return &s->symbols[i - 1];
        }
    }
    return NULL;
}

DPL_Symbol* _dplb_symbols_lookup_function(DPL* dpl, Nob_String_View name, DPL_Handles* arguments) {
    DPL_SymbolStack* s = &dpl->symbol_stack;
    for (size_t i = da_size(s->symbols); i > s->bottom; --i) {
        DPL_Symbol* sym = &s->symbols[i - 1];
        if (sym->kind == SYMBOL_FUNCTION && nob_sv_eq(sym->name, name)
                && _dpl_handles_equal(&sym->as.function.signature.arguments, arguments)) {
            return &s->symbols[i - 1];
        }
    }
    return NULL;
}

const char* _dplb_symbols_kind_name(DPL_SymbolKind kind) {
    switch (kind) {
    case SYMBOL_CONSTANT:
        return "constant";
    case SYMBOL_VAR:
        return "variable";
    case SYMBOL_ARGUMENT:
        return "argument";
    default:
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
}

DPL_Scope* _dplb_scopes_current(DPL* dpl) {
    if (da_empty(dpl->scope_stack)) {
        DPL_ERROR("Cannot get current scope information: no scope pushed onto stack.");
    }

    return &dpl->scope_stack[da_size(dpl->scope_stack) - 1];
}

void _dplb_scopes_begin_scope(DPL* dpl) {
    DPL_Scope scope = {0};

    if (da_some(dpl->scope_stack)) {
        DPL_Scope* current_top = _dplb_scopes_current(dpl);
        scope.offset = current_top->offset + current_top->count;
    }

    da_add(dpl->scope_stack, scope);
}

void _dplb_scopes_end_scope(DPL* dpl) {
    da_pop(dpl->scope_stack);
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

DPL_Type* _dplb_bind_type(DPL* dpl, DPL_Ast_Type* ast_type) {
    DPL_Type* result = NULL;
    Nob_String_View type_name = nob_sv_from_cstr(strdup(_dpla_type_name(ast_type)));

    DPL_Type* cached_type = _dplt_find_by_name(dpl, type_name);
    if (cached_type) {
        nob_return_defer(cached_type);
    }

    if (ast_type->kind == TYPE_OBJECT) {
        DPL_Ast_TypeObject object_type = ast_type->as.object;

        DPL_TypeField* fields = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_TypeField) * object_type.field_count);
        for (size_t i = 0; i < object_type.field_count; ++i) {
            DPL_Ast_TypeField ast_field = object_type.fields[i];
            DPL_Type* bound_field_type = _dplb_bind_type(dpl, ast_field.type);
            if (!bound_field_type) {
                DPL_AST_ERROR(dpl, ast_field.type, "Unable to bind type `%s` for field `"SV_Fmt"`.",
                              _dpla_type_name(ast_field.type), SV_Arg(ast_field.name.text));
            }
            fields[i].name = _dpl_arena_svcpy(&dpl->bound_tree.memory, ast_field.name.text);
            fields[i].type = bound_field_type->handle;
        }

        DPL_Type new_object_type = {
            .name = _dpl_arena_svcpy(&dpl->bound_tree.memory, type_name),
            .kind = TYPE_OBJECT,
            .as.object.field_count = object_type.field_count,
            .as.object.fields = fields,
        };
        DPL_Handle registered_type_handle = _dplt_register(dpl, new_object_type);
        nob_return_defer(_dplt_find_by_handle(dpl, registered_type_handle));
    }

defer:
    free((char*)type_name.data);
    return result;
}

void _dplg_generate_call_userfunction(DPL* dpl, DPL_Program* program, void* data)
{
    DPL_UserFunction* uf = &dpl->user_functions[(size_t) data];
    dplp_write_call_user(program, uf->arity, uf->begin_ip);
}

void _dplb_check_function_used(DPL* dpl, DPL_Symbol* symbol) {
    DPL_Symbol_Function* f = &symbol->as.function;
    if (!f->used) {
        f->used = true;
        f->user_handle = da_size(dpl->user_functions);

        Nob_String_View name = nob_sv_from_cstr(
                                   nob_temp_sprintf("$%zu_"SV_Fmt,
                                           f->user_handle,
                                           SV_Arg(symbol->name)));

        f->function_handle = _dplf_register(dpl, name, &f->signature,
                                            _dplg_generate_call_userfunction,
                                            (void*)(size_t)f->user_handle);

        DPL_UserFunction user_function = {
            .function_handle = f->function_handle,
            .arity = f->signature.arguments.count,
            .begin_ip = 0,
            .body = f->body,
        };
        da_add(dpl->user_functions, user_function);
    }
}

DPL_Bound_Node* _dplb_bind_node(DPL* dpl, DPL_Ast_Node* node);

DPL_Bound_Node* _dplb_bind_unary(DPL* dpl, DPL_Ast_Node* node, const char* function_name)
{
    DPL_Bound_Node* operand = _dplb_bind_node(dpl, node->as.unary.operand);
    if (!operand) {
        DPL_AST_ERROR(dpl, node, "Cannot bind operand of unary expression.");
    }

    DPL_Handles argument_types = {0};
    _dpl_add_handle(&argument_types, operand->type_handle);

    DPL_Bound_Node* bound_node = NULL;

    DPL_Symbol* function_symbol = _dplb_symbols_lookup_function(dpl, nob_sv_from_cstr(function_name), &argument_types);
    if (function_symbol) {
        DPL_Symbol_Function* f = &function_symbol->as.function;
        _dplb_check_function_used(dpl, function_symbol);

        bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        bound_node->kind = BOUND_NODE_FUNCTIONCALL;
        bound_node->type_handle = f->signature.returns;
        bound_node->as.function_call.function_handle = f->function_handle;
    } else {
        DPL_Function* function = _dplf_find_by_signature1(
                                     dpl,
                                     nob_sv_from_cstr(function_name),
                                     operand->type_handle);
        if (function) {
            bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            bound_node->kind = BOUND_NODE_FUNCTIONCALL;
            bound_node->type_handle = function->signature.returns;
            bound_node->as.function_call.function_handle = function->handle;
        }
    }

    if (bound_node) {
        da_array(DPL_Bound_Node*) temp_arguments = 0;
        da_add(temp_arguments, operand);
        _dplb_move_nodelist(dpl, temp_arguments, &bound_node->as.function_call.arguments_count, &bound_node->as.function_call.arguments);
        return bound_node;
    }

    DPL_Type* operand_type = _dplt_find_by_handle(dpl, operand->type_handle);
    DPL_Token operator_token = node->as.unary.operator;
    DPL_AST_ERROR(dpl, node, "Cannot resolve function \"%s("SV_Fmt")\" for unary operator \""SV_Fmt"\".",
                  function_name, SV_Arg(operand_type->name), SV_Arg(operator_token.text));
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

    DPL_Handles argument_types = {0};
    _dpl_add_handle(&argument_types, lhs->type_handle);
    _dpl_add_handle(&argument_types, rhs->type_handle);

    DPL_Bound_Node* bound_node = NULL;

    DPL_Symbol* function_symbol = _dplb_symbols_lookup_function(dpl, nob_sv_from_cstr(function_name), &argument_types);
    if (function_symbol) {
        DPL_Symbol_Function* f = &function_symbol->as.function;
        _dplb_check_function_used(dpl, function_symbol);

        bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        bound_node->kind = BOUND_NODE_FUNCTIONCALL;
        bound_node->type_handle = f->signature.returns;
        bound_node->as.function_call.function_handle = f->function_handle;
    } else {
        DPL_Function* function = _dplf_find_by_signature2(
                                     dpl,
                                     nob_sv_from_cstr(function_name),
                                     lhs->type_handle, rhs->type_handle);
        if (function) {
            bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            bound_node->kind = BOUND_NODE_FUNCTIONCALL;
            bound_node->type_handle = function->signature.returns;
            bound_node->as.function_call.function_handle = function->handle;
        }
    }

    if (bound_node) {
        da_array(DPL_Bound_Node*) temp_arguments = 0;
        da_add(temp_arguments, lhs);
        da_add(temp_arguments, rhs);
        _dplb_move_nodelist(dpl, temp_arguments, &bound_node->as.function_call.arguments_count, &bound_node->as.function_call.arguments);
        return bound_node;
    }

    DPL_Type* lhs_type = _dplt_find_by_handle(dpl, lhs->type_handle);
    DPL_Type* rhs_type = _dplt_find_by_handle(dpl, rhs->type_handle);
    DPL_Token operator_token = node->as.binary.operator;
    DPL_AST_ERROR(dpl, node, "Cannot resolve function \"%s("SV_Fmt", "SV_Fmt")\" for binary operator \""SV_Fmt"\".",
                  function_name, SV_Arg(lhs_type->name), SV_Arg(rhs_type->name), SV_Arg(operator_token.text));
}

DPL_Bound_Node* _dplb_bind_function_call(DPL* dpl, DPL_Ast_Node* node)
{
    DPL_Bound_Node* result_ctn = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
    result_ctn->kind = BOUND_NODE_FUNCTIONCALL;

    DPL_Handles argument_types = {0};

    DPL_Ast_FunctionCall fc = node->as.function_call;
    da_array(DPL_Bound_Node*) temp_arguments = 0;
    for (size_t i = 0; i < fc.argument_count; ++i) {
        DPL_Bound_Node* arg_ctn = _dplb_bind_node(dpl, fc.arguments[i]);
        if (!arg_ctn) {
            DPL_AST_ERROR(dpl, fc.arguments[i], "Cannot bind argument #%zu of function call.", i);
        }
        da_add(temp_arguments, arg_ctn);
        _dpl_add_handle(&argument_types, arg_ctn->type_handle);
    }
    _dplb_move_nodelist(dpl, temp_arguments, &result_ctn->as.function_call.arguments_count,
                        &result_ctn->as.function_call.arguments);

    DPL_Symbol* function_symbol = _dplb_symbols_lookup_function(dpl, fc.name.text, &argument_types);
    if (function_symbol) {
        DPL_Symbol_Function* f = &function_symbol->as.function;
        _dplb_check_function_used(dpl, function_symbol);

        result_ctn->as.function_call.function_handle = f->function_handle;
        result_ctn->type_handle = f->signature.returns;
        return result_ctn;
    }

    DPL_Function* function = _dplf_find_by_signature(dpl, fc.name.text, &argument_types);
    if (function) {
        result_ctn->as.function_call.function_handle = function->handle;
        result_ctn->type_handle = function->signature.returns;
        return result_ctn;
    }

    Nob_String_Builder signature_builder = {0};
    {
        nob_sb_append_sv(&signature_builder, fc.name.text);
        nob_sb_append_cstr(&signature_builder, "(");
        DPL_Bound_Node** arguments = result_ctn->as.function_call.arguments;
        size_t arguments_count =  result_ctn->as.function_call.arguments_count;
        for (size_t i = 0; i < arguments_count; ++i) {
            if (i > 0) {
                nob_sb_append_cstr(&signature_builder, ", ");
            }

            DPL_Type *argument_type = _dplt_find_by_handle(dpl, arguments[i]->type_handle);
            nob_sb_append_sv(&signature_builder, argument_type->name);
        }
        nob_sb_append_cstr(&signature_builder, ")");
        nob_sb_append_null(&signature_builder);
    }
    DPL_AST_ERROR(dpl, node, "Cannot resolve function `%s`.", signature_builder.items);
}

DPL_Bound_Node* _dplb_bind_scope(DPL* dpl, DPL_Ast_Node* node)
{
    _dplb_symbols_begin_scope(dpl);
    _dplb_scopes_begin_scope(dpl);

    DPL_Bound_Node* result_ctn = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
    result_ctn->kind = BOUND_NODE_SCOPE;

    DPL_Ast_Scope scope = node->as.scope;
    da_array(DPL_Bound_Node*) temp_expressions = 0;
    for (size_t i = 0; i < scope.expression_count; ++i) {
        DPL_Bound_Node* expr_ctn = _dplb_bind_node(dpl, scope.expressions[i]);
        if (!expr_ctn) {
            continue;
        }

        da_add(temp_expressions, expr_ctn);
        result_ctn->type_handle = expr_ctn->type_handle;
    }
    _dplb_move_nodelist(dpl, temp_expressions, &result_ctn->as.scope.expressions_count, &result_ctn->as.scope.expressions);

    _dplb_scopes_end_scope(dpl);
    _dplb_symbols_end_scope(dpl);
    return result_ctn;
}

Nob_String_View _dplb_unescape_string(DPL* dpl, Nob_String_View escaped_string) {
    // unescape source string literal
    char* unescaped_string = arena_alloc(&dpl->bound_tree.memory, sizeof(char) * (escaped_string.count - 2 + 1));
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

DPL_Bound_Value _dplb_fold_constant(DPL* dpl, DPL_Ast_Node* node) {
    switch (node->kind) {
    case AST_NODE_LITERAL: {
        DPL_Token value = node->as.literal.value;
        switch (value.kind) {
        case TOKEN_NUMBER:
            return (DPL_Bound_Value) {
                .type_handle = dpl->number_type_handle,
                .as.number = atof(nob_temp_sv_to_cstr(value.text)),
            };
        case TOKEN_STRING:
            return (DPL_Bound_Value) {
                .type_handle = dpl->string_type_handle,
                .as.string = _dplb_unescape_string(dpl, value.text),
            };
        case TOKEN_TRUE:
            return (DPL_Bound_Value) {
                .type_handle = dpl->boolean_type_handle,
                .as.boolean = true,
            };
        case TOKEN_FALSE:
            return (DPL_Bound_Value) {
                .type_handle = dpl->boolean_type_handle,
                .as.boolean = false,
            };
        default:
            DPL_TOKEN_ERROR(dpl, value, "Cannot fold %s.", _dpll_token_kind_name(value.kind));
        }
    }
    break;
    case AST_NODE_BINARY: {
        DPL_Token operator = node->as.binary.operator;
        DPL_Bound_Value lhs_value = _dplb_fold_constant(dpl, node->as.binary.left);
        DPL_Bound_Value rhs_value = _dplb_fold_constant(dpl, node->as.binary.right);

        switch (operator.kind) {
        case TOKEN_PLUS: {
            if (lhs_value.type_handle == dpl->number_type_handle && rhs_value.type_handle == dpl->number_type_handle) {
                return (DPL_Bound_Value) {
                    .type_handle = dpl->number_type_handle,
                    .as.number = lhs_value.as.number + rhs_value.as.number,
                };
            }
            if (lhs_value.type_handle == dpl->string_type_handle && rhs_value.type_handle == dpl->string_type_handle) {
                return (DPL_Bound_Value) {
                    .type_handle = dpl->string_type_handle,
                    .as.string = nob_sv_from_cstr(nob_temp_sprintf(SV_Fmt SV_Fmt, SV_Arg(lhs_value.as.string), SV_Arg(rhs_value.as.string))),
                };
            }

            DPL_TOKEN_ERROR(dpl, operator, "Cannot fold constant for binary operator %s: Both operands must be either of type `number` or `string`.",
                            _dpll_token_kind_name(operator.kind));

        }
        case TOKEN_MINUS: {
            if (lhs_value.type_handle != dpl->number_type_handle) {
                DPL_AST_ERROR(dpl, node->as.binary.left, "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                              _dpll_token_kind_name(operator.kind));
            }
            if (rhs_value.type_handle != dpl->number_type_handle) {
                DPL_AST_ERROR(dpl, node->as.binary.right, "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                              _dpll_token_kind_name(operator.kind));
            }

            return (DPL_Bound_Value) {
                .type_handle = dpl->number_type_handle,
                .as.number = lhs_value.as.number - rhs_value.as.number,
            };
        }
        break;
        case TOKEN_STAR: {
            if (lhs_value.type_handle != dpl->number_type_handle) {
                DPL_TOKEN_ERROR(dpl, operator, "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                                _dpll_token_kind_name(operator.kind));
            }
            if (rhs_value.type_handle != dpl->number_type_handle) {
                DPL_TOKEN_ERROR(dpl, operator, "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                                _dpll_token_kind_name(operator.kind));
            }

            return (DPL_Bound_Value) {
                .type_handle = dpl->number_type_handle,
                .as.number = lhs_value.as.number * rhs_value.as.number,
            };
        }
        break;
        case TOKEN_SLASH: {
            if (lhs_value.type_handle != dpl->number_type_handle) {
                DPL_TOKEN_ERROR(dpl, operator, "Cannot fold constant for binary operator %s: Left operand must be of type `number`.",
                                _dpll_token_kind_name(operator.kind));
            }
            if (rhs_value.type_handle != dpl->number_type_handle) {
                DPL_TOKEN_ERROR(dpl, operator, "Cannot fold constant for binary operator %s: Right operand must be of type `number`.",
                                _dpll_token_kind_name(operator.kind));
            }

            return (DPL_Bound_Value) {
                .type_handle = dpl->number_type_handle,
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
        DPL_Symbol* symbol = _dplb_symbols_lookup(dpl, symbol_name);
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

void _dplb_check_assignment(DPL* dpl, const char* what, DPL_Ast_Node* node, DPL_Handle expression_type_handle)
{
    DPL_Ast_Declaration *decl = &node->as.declaration;
    DPL_Type* expression_type = _dplt_find_by_handle(dpl,  expression_type_handle);
    if (expression_type->handle == dpl->none_type_handle) {
        DPL_AST_ERROR(dpl, decl->initialization,
                      "Expressions of type `"SV_Fmt"` cannot be assigned.", SV_Arg(expression_type->name));
    }

    if (decl->type) {
        DPL_Type* declared_type = _dplb_bind_type(dpl, decl->type);
        if (!declared_type) {
            DPL_AST_ERROR(dpl, decl->type, "Unknown type `%s` in declaration of %s `"SV_Fmt"`.",
                          _dpla_type_name(decl->type), what, SV_Arg(decl->name.text));
        }

        if (expression_type->handle != declared_type->handle) {
            DPL_AST_ERROR(dpl, node, "Declared type `"SV_Fmt"` does not match expression type `"SV_Fmt"` in declaration of %s `"SV_Fmt"`.",
                          SV_Arg(declared_type->name), SV_Arg(expression_type->name), what, SV_Arg(decl->name.text));
        }
    }
}

DPL_Bound_Node* _dplb_bind_node(DPL* dpl, DPL_Ast_Node* node)
{
    switch (node->kind)
    {
    case AST_NODE_LITERAL: {
        switch (node->as.literal.value.kind)
        {
        case TOKEN_NUMBER: {
            DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            bound_node->kind = BOUND_NODE_VALUE;
            bound_node->type_handle = dpl->number_type_handle;
            bound_node->as.value = _dplb_fold_constant(dpl, node);
            return bound_node;
        }
        case TOKEN_STRING: {
            DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            bound_node->kind = BOUND_NODE_VALUE;
            bound_node->type_handle = dpl->string_type_handle;
            bound_node->as.value = _dplb_fold_constant(dpl, node);
            return bound_node;
        }
        break;
        case TOKEN_TRUE:
        case TOKEN_FALSE: {
            DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            bound_node->kind = BOUND_NODE_VALUE;
            bound_node->type_handle = dpl->boolean_type_handle;
            bound_node->as.value = _dplb_fold_constant(dpl, node);
            return bound_node;
        }
        break;
        default:
            DPL_AST_ERROR(dpl, node, "Cannot resolve literal type for %s.",
                          _dpll_token_kind_name(node->as.literal.value.kind));
        }
        break;
    }
    case AST_NODE_OBJECT_LITERAL: {
        DPL_Ast_ObjectLiteral object_literal = node->as.object_literal;
        da_array(DPL_Bound_ObjectField) tmp_bound_fields = NULL;
        DPL_TypeObjectQuery type_query = NULL;
        for (size_t i = 0; i < object_literal.field_count; ++i) {
            DPL_Bound_ObjectField bound_field = {
                .name = _dpl_arena_svcpy(&dpl->bound_tree.memory, object_literal.fields[i].name.text),
                .expression = _dplb_bind_node(dpl, object_literal.fields[i].expression),
            };
            da_add(tmp_bound_fields, bound_field);

            DPL_TypeField query_field = {
                .name = bound_field.name,
                .type = bound_field.expression->type_handle,
            };
            da_add(type_query, query_field);
        }

        DPL_Type* bound_type = _dplt_find_by_object_query(dpl, type_query);

        if (!bound_type) {
            Nob_String_Builder type_name_builder = {0};
            nob_sb_append_cstr(&type_name_builder, "[");
            for (size_t i = 0; i < da_size(type_query); ++i) {
                if (i > 0) {
                    nob_sb_append_cstr(&type_name_builder, ", ");
                }
                nob_sb_append_sv(&type_name_builder, type_query[i].name);
                nob_sb_append_cstr(&type_name_builder, ": ");

                DPL_Type* field_type = _dplt_find_by_handle(dpl, type_query[i].type);
                if (!field_type) {
                    DW_UNIMPLEMENTED_MSG("Did not find a field type which really should already be there.");
                }
                nob_sb_append_sv(&type_name_builder, field_type->name);
            }
            nob_sb_append_cstr(&type_name_builder, "]");
            nob_sb_append_null(&type_name_builder);

            DPL_TypeField* new_object_type_fields = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_TypeField) * da_size(type_query));
            memcpy(new_object_type_fields, type_query, sizeof(DPL_TypeField) * da_size(type_query));

            DPL_Type new_object_type = {
                .name = _dpl_arena_strcpy(&dpl->bound_tree.memory, type_name_builder.items),
                .kind = TYPE_OBJECT,
                .as.object.field_count = da_size(type_query),
                .as.object.fields = new_object_type_fields,
            };
            DPL_Handle registered_type_handle = _dplt_register(dpl, new_object_type);
            bound_type = _dplt_find_by_handle(dpl, registered_type_handle);

            nob_sb_free(type_name_builder);
        }
        da_free(type_query);

        DPL_Bound_ObjectField* bound_fields = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_ObjectField) * da_size(tmp_bound_fields));
        memcpy(bound_fields, tmp_bound_fields, sizeof(DPL_Bound_ObjectField) * da_size(tmp_bound_fields));

        DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        bound_node->kind = BOUND_NODE_VALUE;
        bound_node->type_handle = bound_type->handle;
        bound_node->as.value = (DPL_Bound_Value) {
            .type_handle = bound_type->handle,
            .as.object = {
                .field_count = da_size(tmp_bound_fields),
                .fields = bound_fields,
            }
        };

        da_free(tmp_bound_fields);
        return bound_node;
    }
    break;
    case AST_NODE_FIELD_ACCESS: {
        DPL_Bound_Node* bound_expression = _dplb_bind_node(dpl, node->as.field_access.expression);

        DPL_Type* expression_type = _dplt_find_by_handle(dpl, bound_expression->type_handle);
        if (!expression_type || expression_type->kind != TYPE_OBJECT) {
            DPL_AST_ERROR(dpl, node->as.field_access.expression, "Can access fields only for object types.");
        }

        DPL_Token field_name = node->as.field_access.field->as.symbol;
        DPL_TypeObject object_type = expression_type->as.object;

        bool field_found = false;
        size_t field_index = 0;
        DPL_Handle field_type = 0;
        for (size_t i = 0; i < object_type.field_count; ++i) {
            if (nob_sv_eq(object_type.fields[i].name, field_name.text)) {
                field_found = true;
                field_index = i;
                field_type = object_type.fields[i].type;
                break;
            }
        }

        if (!field_found) {
            DPL_AST_ERROR(dpl, node, "Objects of type `"SV_Fmt"`, have no field `"SV_Fmt"`.",
                          SV_Arg(expression_type->name), SV_Arg(field_name.text));
        }

        DPL_Bound_Node* bound_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        bound_node->kind = BOUND_NODE_FIELD_ACCESS;
        bound_node->type_handle = field_type;
        bound_node->as.field_access = (DPL_Bound_FieldAccess) {
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
            return _dplb_bind_binary(dpl, node, "less_equal");
        case TOKEN_GREATER:
            return _dplb_bind_binary(dpl, node, "greater");
        case TOKEN_GREATER_EQUAL:
            return _dplb_bind_binary(dpl, node, "greater_equal");
        case TOKEN_EQUAL_EQUAL:
            return _dplb_bind_binary(dpl, node, "equal");
        case TOKEN_BANG_EQUAL:
            return _dplb_bind_binary(dpl, node, "not_equal");
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
            bound_node->type_handle = dpl->boolean_type_handle;
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
            DPL_Symbol s = {
                .kind = SYMBOL_CONSTANT,
                .name = decl->name.text,
                .as.constant =  _dplb_fold_constant(dpl, decl->initialization),
            };
            _dplb_check_assignment(dpl, "constant", node, s.as.constant.type_handle);

            da_add(dpl->symbol_stack.symbols, s);

            return NULL;
        } else {
            DPL_Bound_Node* expression = _dplb_bind_node(dpl, decl->initialization);
            _dplb_check_assignment(dpl, "variable", node, expression->type_handle);

            DPL_Scope* current_scope = _dplb_scopes_current(dpl);
            DPL_Symbol s = {
                .kind = SYMBOL_VAR,
                .name = decl->name.text,
                .as.var = {
                    .type_handle = expression->type_handle,
                    .scope_index = current_scope->offset + current_scope->count,
                }
            };
            current_scope->count++;

            da_add(dpl->symbol_stack.symbols, s);

            expression->persistent = true;
            return expression;
        }
    }
    break;
    case AST_NODE_SYMBOL: {
        DPL_Symbol* symbol = _dplb_symbols_lookup(dpl, node->as.symbol.text);
        if (!symbol) {
            DPL_AST_ERROR(dpl, node, "Cannot resolve symbol `"SV_Fmt"` in current scope.",
                          SV_Arg(node->as.symbol.text));
        }

        switch (symbol->kind) {
        case SYMBOL_CONSTANT: {
            DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            node->kind = BOUND_NODE_VALUE;
            node->type_handle = symbol->as.constant.type_handle;
            node->as.value = symbol->as.constant;
            return node;
        }
        break;
        case SYMBOL_VAR: {
            DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            node->kind = BOUND_NODE_VARREF;
            node->type_handle = symbol->as.var.type_handle;
            node->as.varref = symbol->as.var.scope_index;
            return node;
        }
        break;
        case SYMBOL_ARGUMENT: {
            DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
            node->kind = BOUND_NODE_ARGREF;
            node->type_handle = symbol->as.argument.type_handle;
            node->as.argref = symbol->as.argument.scope_index;
            return node;
        }
        break;
        default:
            DPL_AST_ERROR(dpl, node, "Cannot resolve symbols of type `%s`.",
                          _dplb_symbols_kind_name(symbol->kind));

        }
    }
    case AST_NODE_ASSIGNMENT: {
        if (node->as.assignment.target->kind != AST_NODE_SYMBOL) {
            DPL_AST_ERROR(dpl, node, "Cannot assign to target of type `%s`.",
                          _dpla_node_kind_name(node->as.assignment.target->kind));
        }

        Nob_String_View symbol_name = node->as.assignment.target->as.symbol.text;
        DPL_Symbol* symbol = _dplb_symbols_lookup(dpl, symbol_name);
        if (!symbol) {
            DPL_AST_ERROR(dpl, node->as.assignment.target, "Cannot resolve symbol `"SV_Fmt"` in current scope.",
                          SV_Arg(symbol_name));
        }
        if (symbol->kind != SYMBOL_VAR) {
            DPL_AST_ERROR(dpl, node->as.assignment.target, "Cannot assign to %s `"SV_Fmt"` in current scope.",
                          _dplb_symbols_kind_name(symbol->kind), SV_Arg(symbol_name));
        }

        DPL_Bound_Node* ct_node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        ct_node->kind = BOUND_NODE_ASSIGNMENT;
        ct_node->type_handle = symbol->as.var.type_handle;
        ct_node->as.assignment.scope_index = symbol->as.var.scope_index;
        ct_node->as.assignment.expression = _dplb_bind_node(dpl, node->as.assignment.expression);

        return ct_node;
    }
    case AST_NODE_FUNCTION: {
        DPL_Ast_Function* function = &node->as.function;

        DPL_Signature signature = {0};
        for (size_t i = 0; i < function->signature.argument_count; ++i) {
            DPL_Ast_FunctionArgument arg = function->signature.arguments[i];
            DPL_Type* arg_type = _dplb_bind_type(dpl, arg.type);
            if (!arg_type) {
                DPL_AST_ERROR(dpl, arg.type,
                              "Cannot resolve type `%s` for argument `"SV_Fmt"` in current scope.",
                              _dpla_type_name(arg.type), SV_Arg(arg.name.text));
            }

            _dpl_add_handle(&signature.arguments, arg_type->handle);
        }

        DPL_Type* return_type = _dplb_bind_type(dpl, function->signature.type);
        if (!return_type) {
            DPL_AST_ERROR(dpl, function->signature.type,
                          "Cannot resolve return type `%s` in current scope.",
                          _dpla_type_name(function->signature.type));
        }
        signature.returns = return_type->handle;

        _dplb_symbols_begin_scope(dpl);
        _dplb_scopes_begin_scope(dpl);

        size_t current_bottom = dpl->symbol_stack.bottom;
        dpl->symbol_stack.bottom = da_size(dpl->symbol_stack.symbols);

        DPL_Scope* current_scope = _dplb_scopes_current(dpl);
        for (size_t i = 0; i < function->signature.argument_count; ++i) {
            DPL_Symbol arg_symbol = {
                .kind = SYMBOL_ARGUMENT,
                .name = function->signature.arguments[i].name.text,
                .as.argument = {
                    .scope_index = i,
                    .type_handle = signature.arguments.items[i],
                }
            };

            da_add(dpl->symbol_stack.symbols, arg_symbol);
            current_scope->count++;
        }

        DPL_Symbol s = {
            .kind = SYMBOL_FUNCTION,
            .name = function->name.text,
            .as.function = {
                .signature = signature,
                .used = false,
                .user_handle = 0,
                .body = _dplb_bind_node(dpl, function->body),
            },
        };

        dpl->symbol_stack.bottom = current_bottom;

        _dplb_scopes_end_scope(dpl);
        _dplb_symbols_end_scope(dpl);

        da_add(dpl->symbol_stack.symbols, s);
        return NULL;
    }
    case AST_NODE_CONDITIONAL: {
        DPL_Ast_Conditional conditional =  node->as.conditional;

        DPL_Bound_Node* bound_condition = _dplb_bind_node(dpl, conditional.condition);
        if (bound_condition->type_handle != dpl->boolean_type_handle) {
            DPL_AST_ERROR(dpl, conditional.condition,
                          "Condition operand of a conditional must be of type `boolean`.");
        }

        DPL_Bound_Node* bound_then_clause = _dplb_bind_node(dpl, conditional.then_clause);
        DPL_Bound_Node* bound_else_clause = _dplb_bind_node(dpl, conditional.else_clause);
        if (bound_then_clause->type_handle != bound_else_clause->type_handle) {
            DPL_Type* then_clause_type = _dplt_find_by_handle(dpl, bound_then_clause->type_handle);
            DPL_Type* else_clause_type = _dplt_find_by_handle(dpl, bound_else_clause->type_handle);

            DPL_AST_ERROR(dpl, node, "Types of then and else clause operands of a conditional must match. Then clause type is `"SV_Fmt"`, else clause type is `"SV_Fmt"`.",
                          SV_Arg(then_clause_type->name), SV_Arg(else_clause_type->name));
        }

        DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        node->kind = BOUND_NODE_CONDITIONAL;
        node->type_handle = bound_then_clause->type_handle;
        node->as.conditional.condition = bound_condition;
        node->as.conditional.then_clause = bound_then_clause;
        node->as.conditional.else_clause = bound_else_clause;
        return node;
    }
    break;
    case AST_NODE_WHILE_LOOP: {
        DPL_Ast_WhileLoop while_loop =  node->as.while_loop;

        DPL_Bound_Node* bound_condition = _dplb_bind_node(dpl, while_loop.condition);
        if (bound_condition->type_handle != dpl->boolean_type_handle) {
            DPL_AST_ERROR(dpl, while_loop.condition,
                          "Condition operand of a while loop must be of type `boolean`.");
        }

        DPL_Bound_Node* bound_body = _dplb_bind_node(dpl, while_loop.body);

        // TODO: At the moment, loops do not produce values and therefore are of type `none`.
        //       This should change in the future, where they can yield optional values or
        //       arrays.
        DPL_Bound_Node* node = arena_alloc(&dpl->bound_tree.memory, sizeof(DPL_Bound_Node));
        node->kind = BOUND_NODE_WHILE_LOOP;
        node->type_handle = dpl->none_type_handle;
        node->as.while_loop.condition = bound_condition;
        node->as.while_loop.body = bound_body;
        return node;
    }
    break;
    default:
        DPL_AST_ERROR(dpl, node, "Cannot bind AST node of kind \"%s\".",
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

    DPL_Type* type = _dplt_find_by_handle(dpl, node->type_handle);
    if (type) {
        printf("["SV_Fmt"] ", SV_Arg(type->name));
    } else {
        printf("[<unknown>] ");
    }

    switch(node->kind) {
    case BOUND_NODE_FUNCTIONCALL: {
        DPL_Function* function = _dplf_find_by_handle(dpl, node->as.function_call.function_handle);
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
        if (node->as.value.type_handle == dpl->number_type_handle) {
            printf("%f", node->as.value.as.number);
        } else if (node->as.value.type_handle == dpl->string_type_handle) {
            Nob_String_View value = node->as.value.as.string;
            dplp_print_escaped_string(value.data, value.count);
        } else if (node->as.value.type_handle == dpl->boolean_type_handle) {
            printf("%s", node->as.value.as.boolean ? "true" : "false");
        } else {
            if (!type) {
                printf("<unknown>");
            } else if (type->kind == TYPE_OBJECT) {
                DPL_Bound_Object object = node->as.value.as.object;
                printf("[\n");

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
                printf("]");
            }
        }
        printf("`\n");
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
    case BOUND_NODE_FIELD_ACCESS: {
        printf("$field_access( #%zu\n", node->as.field_access.field_index);
        _dplb_print(dpl, node->as.field_access.expression, level + 1);

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
        if (node->type_handle == dpl->number_type_handle) {
            dplp_write_push_number(program, node->as.value.as.number);
        } else if (node->type_handle == dpl->string_type_handle) {
            dplp_write_push_string(program, node->as.value.as.string.data);
        } else if (node->type_handle == dpl->boolean_type_handle) {
            dplp_write_push_boolean(program, node->as.value.as.boolean);
        } else {
            DPL_Type* type = _dplt_find_by_handle(dpl, node->type_handle);
            if (type->kind == TYPE_OBJECT) {
                DPL_Bound_Object object = node->as.value.as.object;
                for (size_t i = 0; i < object.field_count; ++i) {
                    _dplg_generate(dpl, object.fields[i].expression, program);
                }

                dplp_write_create_object(program, object.field_count);
            } else {
                DPL_ERROR("Cannot generate program for value node of type "SV_Fmt".",
                          SV_Arg(type->name));
            }
        }
    }
    break;
    case BOUND_NODE_FIELD_ACCESS: {
        _dplg_generate(dpl, node->as.field_access.expression, program);
        dplp_write_load_field(program, node->as.field_access.field_index);
    }
    break;
    case BOUND_NODE_FUNCTIONCALL: {
        DPL_Bound_FunctionCall f = node->as.function_call;
        for (size_t i = 0; i < f.arguments_count; ++i) {
            _dplg_generate(dpl, f.arguments[i], program);
        }

        DPL_Function* function = _dplf_find_by_handle(dpl, f.function_handle);
        function->generator.callback(dpl, program, function->generator.user_data);
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
            DPL_Function* f = _dplf_find_by_handle(dpl, uf->function_handle);
            printf("### "SV_Fmt" (arity: %zu) ###\n", SV_Arg(f->name), uf->arity);
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
        printf("Types:\n");
        for (size_t i = 0; i < da_size(dpl->types); ++i) {
            printf("* %u: ", dpl->types[i].handle);
            _dplt_print(stdout, dpl, &dpl->types[i]);
            printf("\n");
        }
        printf("\n");
    }

    if (dpl->debug)
    {
        printf("Functions:\n");
        for (size_t i = 0; i < da_size(dpl->functions); ++i) {
            printf("* %u: ", dpl->functions[i].handle);
            _dplf_print(stdout, dpl, &dpl->functions[i]);
            printf("\n");
        }
        printf("\n");
    }
}