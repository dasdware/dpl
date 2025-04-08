#include <dpl/symbols.h>

#include <error.h>

#include <dpl/utils.h>

#define ABORT_IF_NULL(value) \
    if (value == NULL)       \
    {                        \
        return NULL;         \
    }

#define STACK_FOREACH_BEGIN(stack, it)                                                                    \
    if (stack->entries_count > 0)                                                                         \
    {                                                                                                     \
        for (size_t __##it##_iterator = stack->entries_count; __##it##_iterator > 0; --__##it##_iterator) \
        {                                                                                                 \
            DPL_Symbol *it = stack->entries[__##it##_iterator - 1];

#define STACK_FOREACH_END \
    }                     \
    }

const char *SYMBOL_KIND_NAMES[COUNT_SYMBOL_KINDS] = {
    [SYMBOL_BOUNDARY] = "boundary",
    [SYMBOL_TYPE] = "type",
    [SYMBOL_ARGUMENT] = "argument",
    [SYMBOL_CONSTANT] = "constant",
    [SYMBOL_FUNCTION] = "function",
    [SYMBOL_VAR] = "variable",
};

static_assert(COUNT_SYMBOL_KINDS == 6,
              "Count of symbol kinds has changed, please update symbol kind names map.");

const char *dpl_symbols_kind_name(DPL_Symbol_Kind kind)
{
    return SYMBOL_KIND_NAMES[kind];
}

const char *SYMBOL_BOUNDARY_KIND_NAMES[COUNT_SYMBOL_BOUNDARY_KINDS] = {
    [BOUNDARY_MODULE] = "module",
    [BOUNDARY_SCOPE] = "scope",
    [BOUNDARY_FUNCTION] = "function",
};

static_assert(COUNT_SYMBOL_BOUNDARY_KINDS == 3,
              "Count of symbol boundary kinds has changed, please update symbol kind names map.");

const char *SYMBOL_TYPE_BASE_KIND_NAMES[COUNT_SYMBOL_TYPE_BASE_KINDS] = {
    [TYPE_BASE_NUMBER] = "number",
    [TYPE_BASE_STRING] = "string",
    [TYPE_BASE_BOOLEAN] = "boolean",
    [TYPE_BASE_NONE] = "none",
};

static_assert(COUNT_SYMBOL_TYPE_BASE_KINDS == 4,
              "Count of symbol base type kinds has changed, please update symbol kind names map.");

Nob_String_Builder error_sb = {0};

const char *dpl_symbols_last_error()
{
    if (error_sb.count == 0)
    {
        return "";
    }
    return error_sb.items;
}

DPL_Symbol *dpl_symbols_find(DPL_SymbolStack *stack, Nob_String_View name)
{
    bool function_boundary_crossed = false;

    STACK_FOREACH_BEGIN(stack, candidate)
    if (nob_sv_eq(candidate->name, name))
    {
        if (function_boundary_crossed && (candidate->kind == SYMBOL_ARGUMENT || candidate->kind == SYMBOL_VAR))
        {
            break;
        }
        return candidate;
    }
    else if (candidate->kind == SYMBOL_BOUNDARY && candidate->as.boundary == BOUNDARY_FUNCTION)
    {
        function_boundary_crossed = true;
    }
    STACK_FOREACH_END

    error_sb.count = 0;
    nob_sb_append_cstr(&error_sb, "Cannot find symbol ");
    nob_sb_append_cstr(&error_sb, " `");
    nob_sb_append_sv(&error_sb, name);
    nob_sb_append_cstr(&error_sb, "`.");
    nob_sb_append_null(&error_sb);

    return NULL;
}

DPL_Symbol *dpl_symbols_find_cstr(DPL_SymbolStack *stack, const char *name)
{
    return dpl_symbols_find(stack, nob_sv_from_cstr(name));
}

DPL_Symbol *dpl_symbols_find_kind(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol_Kind kind)
{
    DPL_Symbol *result = dpl_symbols_find(stack, name);
    if (result != NULL && result->kind == kind)
    {
        return result;
    }

    error_sb.count = 0;
    nob_sb_append_cstr(&error_sb, "Cannot find ");
    nob_sb_append_cstr(&error_sb, SYMBOL_KIND_NAMES[kind]);
    nob_sb_append_cstr(&error_sb, " `");
    nob_sb_append_sv(&error_sb, name);
    nob_sb_append_cstr(&error_sb, "`.");
    nob_sb_append_null(&error_sb);

    return NULL;
}

DPL_Symbol *dpl_symbols_find_kind_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol_Kind kind)
{
    return dpl_symbols_find_kind(stack, nob_sv_from_cstr(name), kind);
}

DPL_Symbol *dpl_symbols_find_type_base(DPL_SymbolStack *stack, DPL_Symbol_Type_Base_Kind kind)
{
    STACK_FOREACH_BEGIN(stack, symbol)
    if (symbol->kind == SYMBOL_TYPE && symbol->as.type.kind == TYPE_BASE && symbol->as.type.as.base == kind)
    {
        return symbol;
    }
    STACK_FOREACH_END

    error_sb.count = 0;
    nob_sb_append_cstr(&error_sb, "Cannot find base type `");
    nob_sb_append_cstr(&error_sb, SYMBOL_TYPE_BASE_KIND_NAMES[kind]);
    nob_sb_append_cstr(&error_sb, "`.");
    nob_sb_append_null(&error_sb);

    return NULL;
}

DPL_Symbol *dpl_symbols_find_type_object_query(DPL_SymbolStack *stack, DPL_Symbol_Type_ObjectQuery query)
{
    STACK_FOREACH_BEGIN(stack, symbol)
    if (symbol->kind != SYMBOL_TYPE || symbol->as.type.kind != TYPE_OBJECT)
    {
        continue;
    }

    DPL_Symbol_Type_Object *object_type = &symbol->as.type.as.object;
    if (object_type->field_count == da_size(query))
    {
        bool is_match = true;
        for (size_t j = 0; j < object_type->field_count; ++j)
        {
            if (!nob_sv_eq(query[j].name, object_type->fields[j].name))
            {
                is_match = false;
                break;
            }
            if (query[j].type != object_type->fields[j].type)
            {
                is_match = false;
                break;
            }
        }
        if (is_match)
        {
            return symbol;
        }
    }
    STACK_FOREACH_END

    // TODO: Prepare error message(?)
    return NULL;
}

DPL_Symbol *dpl_symbols_check_type_object_query(DPL_SymbolStack *stack, DPL_Symbol_Type_ObjectQuery query)
{
    DPL_Symbol *object_type = dpl_symbols_find_type_object_query(stack, query);
    if (!object_type)
    {
        Nob_String_Builder sb_name = {0};
        nob_sb_append_cstr(&sb_name, "[");
        for (size_t i = 0; i < da_size(query); ++i)
        {
            if (i > 0)
            {
                nob_sb_append_cstr(&sb_name, ", ");
            }
            nob_sb_append_sv(&sb_name, query[i].name);
            nob_sb_append_cstr(&sb_name, ": ");
            nob_sb_append_sv(&sb_name, query[i].type->name);
        }
        nob_sb_append_cstr(&sb_name, "]");
        nob_sb_append_null(&sb_name);

        object_type = dpl_symbols_push_type_object_cstr(stack, sb_name.items, da_size(query));
        memcpy(object_type->as.type.as.object.fields, query, sizeof(DPL_Symbol_Type_ObjectField) * da_size(query));

        nob_sb_free(sb_name);
    }
    return object_type;
}

DPL_Symbol *dpl_symbols_find_function(DPL_SymbolStack *stack,
                                      Nob_String_View name, size_t arguments_count, DPL_Symbol **arguments)
{
    STACK_FOREACH_BEGIN(stack, candidate)
    if (candidate->kind != SYMBOL_FUNCTION || !nob_sv_eq(candidate->name, name) || arguments_count != candidate->as.function.signature.argument_count)
    {
        continue;
    }

    bool is_match = true;
    DPL_Symbol_Function *f = &candidate->as.function;
    for (size_t i = 0; i < arguments_count; ++i)
    {
        if (f->signature.arguments[i] != arguments[i])
        {
            is_match = false;
            break;
        }
    }

    if (is_match)
    {
        return candidate;
    }
    STACK_FOREACH_END

    return NULL;
}

DPL_Symbol *dpl_symbols_find_function1(DPL_SymbolStack *stack,
                                       Nob_String_View name, DPL_Symbol *arg0)
{
    return dpl_symbols_find_function(stack, name, 1, (DPL_Symbol *[]){arg0});
}

DPL_Symbol *dpl_symbols_find_function1_cstr(DPL_SymbolStack *stack,
                                            const char *name, DPL_Symbol *arg0)
{
    return dpl_symbols_find_function1(stack, nob_sv_from_cstr(name), arg0);
}

DPL_Symbol *dpl_symbols_find_function2(DPL_SymbolStack *stack,
                                       Nob_String_View name, DPL_Symbol *arg0, DPL_Symbol *arg1)
{
    return dpl_symbols_find_function(stack, name, 2, (DPL_Symbol *[]){arg0, arg1});
}

DPL_Symbol *dpl_symbols_find_function2_cstr(DPL_SymbolStack *stack,
                                            const char *name, DPL_Symbol *arg0, DPL_Symbol *arg1)
{
    return dpl_symbols_find_function2(stack, nob_sv_from_cstr(name), arg0, arg1);
}

Nob_String_View dpl_symbols_alloc_sv(DPL_SymbolStack *stack, const char *value)
{
    size_t count = strlen(value);
    char *data = arena_alloc(&stack->memory, count);
    memcpy(data, value, count);
    return nob_sv_from_parts(data, count);
}

DPL_Symbol *dpl_symbols_push(DPL_SymbolStack *stack, DPL_Symbol_Kind kind, Nob_String_View name)
{
    size_t boundary_count;
    if (kind == SYMBOL_BOUNDARY)
    {
        boundary_count = 1;
    }
    else if (stack->entries_count > 0)
    {
        boundary_count = stack->entries[stack->entries_count - 1]->boundary_count + 1;
    }
    else
    {
        error_sb.count = 0;
        nob_sb_append_cstr(&error_sb, "Can push non-boundary symbols only if there is already a boundary symbol on the stack.");
        nob_sb_append_null(&error_sb);

        return NULL;
    }

    size_t stack_index = -1;
    if (stack->entries_count > 0)
    {
        stack_index = stack->entries[stack->entries_count - 1]->stack_index;
        if (kind == SYMBOL_VAR || kind == SYMBOL_ARGUMENT)
        {
            stack_index += 1;
        }
    }

    DPL_Symbol *symbol = arena_alloc(&stack->memory, sizeof(DPL_Symbol));
    symbol->kind = kind;
    symbol->name = name;
    symbol->boundary_count = boundary_count;
    symbol->stack_index = stack_index;

    stack->entries[stack->entries_count] = symbol;
    stack->entries_count += 1;

    return symbol;
}

DPL_Symbol *dpl_symbols_push_cstr(DPL_SymbolStack *stack, DPL_Symbol_Kind kind, const char *name)
{
    Nob_String_View sv_name = SV_NULL;
    if (name != NULL)
    {
        sv_name = dpl_symbols_alloc_sv(stack, name);
    }

    return dpl_symbols_push(stack, kind, sv_name);
}

DPL_Symbol *dpl_symbols_push_boundary_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol_Boundary_Kind kind)
{
    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_BOUNDARY, name);
    if (symbol == NULL)
    {
        return NULL;
    }

    symbol->as.boundary = kind;
    if (kind == BOUNDARY_FUNCTION)
    {
        symbol->stack_index = -1;
    }

    return symbol;
}

bool dpl_symbols_pop_boundary(DPL_SymbolStack *stack)
{
    if (stack->entries_count == 0)
    {
        error_sb.count = 0;
        nob_sb_append_cstr(&error_sb, "Cannot pop current boundary: symbol stack is empty.");
        nob_sb_append_null(&error_sb);

        return false;
    }

    stack->entries_count -= stack->entries[stack->entries_count - 1]->boundary_count;
    return true;
}

DPL_Symbol *dpl_symbols_push_type_base_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol_Type_Base_Kind base_kind)
{
    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_TYPE, name);
    ABORT_IF_NULL(symbol);

    symbol->as.type = (DPL_Symbol_Type){
        .kind = TYPE_BASE,
        .as.base = base_kind,
    };
    return symbol;
}

DPL_Symbol *dpl_symbols_push_type_object_cstr(DPL_SymbolStack *stack, const char *name, size_t field_count)
{
    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_TYPE, name);
    ABORT_IF_NULL(symbol);

    symbol->as.type = (DPL_Symbol_Type){
        .kind = TYPE_OBJECT,
        .as.object = {
            .field_count = field_count,
            .fields = arena_alloc(&stack->memory, sizeof(DPL_Symbol_Type_ObjectField) * field_count),
        }};
    return symbol;
}

DPL_Symbol *dpl_symbols_push_type_alias(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol *type)
{
    DPL_Symbol *symbol = dpl_symbols_push(stack, SYMBOL_TYPE, name);
    ABORT_IF_NULL(symbol);

    symbol->as.type = (DPL_Symbol_Type){
        .kind = TYPE_ALIAS,
        .as.alias = type,
    };
    return symbol;
}

bool dpl_symbols_is_type_base(DPL_Symbol *symbol, DPL_Symbol_Type_Base_Kind kind)
{
    return symbol->kind == SYMBOL_TYPE && symbol->as.type.kind == TYPE_BASE && symbol->as.type.as.base == kind;
}

DPL_Symbol *dpl_symbols_push_constant_number_cstr(DPL_SymbolStack *stack, const char *name, double value)
{
    DPL_Symbol *type = dpl_symbols_find_type_number(stack);
    ABORT_IF_NULL(type);

    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_CONSTANT, name);
    ABORT_IF_NULL(symbol);

    symbol->as.constant = (DPL_Symbol_Constant){
        .type = type,
        .as.number = value,
    };
    return symbol;
}

DPL_Symbol *dpl_symbols_push_argument(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol *type)
{
    DPL_Symbol *symbol = dpl_symbols_push(stack, SYMBOL_ARGUMENT, name);
    ABORT_IF_NULL(symbol);

    symbol->as.argument = (DPL_Symbol_Var){
        .type = type,
        .scope_index = symbol->stack_index,
    };
    return symbol;
}

DPL_Symbol *dpl_symbols_push_argument_cstr(DPL_SymbolStack *stack, const char *name, const char *type_name)
{
    DPL_Symbol *type = NULL;
    if (type_name != NULL)
    {
        type = dpl_symbols_find_kind(stack, nob_sv_from_cstr(type_name), SYMBOL_TYPE);
        ABORT_IF_NULL(type);
    }

    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_ARGUMENT, name);
    ABORT_IF_NULL(symbol);

    symbol->as.argument = (DPL_Symbol_Var){
        .type = type,
        .scope_index = symbol->stack_index,
    };
    return symbol;
}

DPL_Symbol *dpl_symbols_push_var(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol *type)
{
    DPL_Symbol *symbol = dpl_symbols_push(stack, SYMBOL_VAR, name);
    ABORT_IF_NULL(symbol);

    symbol->as.var = (DPL_Symbol_Var){
        .type = type,
        .scope_index = symbol->stack_index,
    };
    return symbol;
}

DPL_Symbol *dpl_symbols_push_var_cstr(DPL_SymbolStack *stack, const char *name, const char *type_name)
{
    DPL_Symbol *type = NULL;
    if (type_name != NULL)
    {
        type = dpl_symbols_find_kind(stack, nob_sv_from_cstr(type_name), SYMBOL_TYPE);
        ABORT_IF_NULL(type);
    }

    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_VAR, name);
    ABORT_IF_NULL(symbol);

    symbol->as.var = (DPL_Symbol_Var){
        .type = type,
        .scope_index = symbol->stack_index,
    };
    return symbol;
}

static bool dpl_symbols_resolve_function_signature_cstr(DPL_SymbolStack *stack, DPL_Symbol_Type_Signature *signature,
                                                        const char *return_type, size_t argument_count, const char **argument_types)
{
    signature->returns = dpl_symbols_find_kind(stack, nob_sv_from_cstr(return_type), SYMBOL_TYPE);
    if (signature->returns == NULL)
    {
        return false;
    }

    signature->argument_count = argument_count;
    signature->arguments = arena_alloc(&stack->memory, argument_count * sizeof(DPL_Symbol *));
    for (size_t argument_index = 0; argument_index < argument_count; ++argument_index)
    {
        signature->arguments[argument_index] = dpl_symbols_find_kind(stack, nob_sv_from_cstr(argument_types[argument_index]), SYMBOL_TYPE);
        if (signature->arguments[argument_index] == NULL)
        {
            return false;
        }
    }

    return true;
}

DPL_Symbol *dpl_symbols_push_function_instruction_cstr(DPL_SymbolStack *stack,
                                                       const char *name, const char *return_type, size_t argument_count, const char **argument_types,
                                                       DPL_Instruction_Kind instruction)
{
    DPL_Symbol_Type_Signature signature = {0};
    if (!dpl_symbols_resolve_function_signature_cstr(stack, &signature, return_type, argument_count, argument_types))
    {
        return NULL;
    }

    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_FUNCTION, name);
    ABORT_IF_NULL(symbol);

    symbol->as.function = (DPL_Symbol_Function){
        .signature = signature,
        .kind = FUNCTION_INSTRUCTION,
        .as.instruction_function = instruction,
    };
    return symbol;
}

DPL_Symbol *dpl_symbols_push_function_intrinsic(DPL_SymbolStack *stack,
                                                const char *name, DPL_Symbol *return_type, size_t argument_count, DPL_Symbol **argument_types,
                                                DPL_Intrinsic_Kind intrinsic)
{
    DPL_Symbol_Type_Signature signature = {0};
    signature.returns = return_type;
    signature.argument_count = argument_count;

    signature.arguments = arena_alloc(&stack->memory, argument_count * sizeof(DPL_Symbol *));
    memcpy(signature.arguments, argument_types, argument_count * sizeof(DPL_Symbol *));

    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_FUNCTION, name);
    ABORT_IF_NULL(symbol);

    symbol->as.function = (DPL_Symbol_Function){
        .signature = signature,
        .kind = FUNCTION_INTRINSIC,
        .as.intrinsic_function = intrinsic,
    };
    return symbol;
}

DPL_Symbol *dpl_symbols_push_function_intrinsic_cstr(DPL_SymbolStack *stack,
                                                     const char *name, const char *return_type, size_t argument_count, const char **argument_types,
                                                     DPL_Intrinsic_Kind intrinsic)
{
    DPL_Symbol_Type_Signature signature = {0};
    if (!dpl_symbols_resolve_function_signature_cstr(stack, &signature, return_type, argument_count, argument_types))
    {
        return NULL;
    }

    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_FUNCTION, name);
    ABORT_IF_NULL(symbol);

    symbol->as.function = (DPL_Symbol_Function){
        .signature = signature,
        .kind = FUNCTION_INTRINSIC,
        .as.intrinsic_function = intrinsic,
    };
    return symbol;
}

DPL_Symbol *dpl_symbols_push_function_user(DPL_SymbolStack *stack, Nob_String_View name, size_t argument_count)
{
    DPL_Symbol *symbol = dpl_symbols_push(stack, SYMBOL_FUNCTION, name);
    ABORT_IF_NULL(symbol);

    symbol->as.function = (DPL_Symbol_Function){
        .kind = FUNCTION_USER,
        .signature = {
            .argument_count = argument_count,
            .arguments = arena_alloc(&stack->memory, sizeof(DPL_Symbol *) * argument_count),
            .returns = NULL},
        .as.user_function = {
            .body = NULL,
            .function_handle = 0,
            .used = false,
            .user_handle = 0,
        }};
    return symbol;
}

void dpl_symbols_init(DPL_SymbolStack *stack)
{
    if (stack->entries_capacity == 0)
    {
        stack->entries_capacity = 1024;
    }
    stack->entries = arena_alloc(&stack->memory, stack->entries_capacity * sizeof(*stack->entries));

    dpl_symbols_push_boundary_cstr(stack, NULL, BOUNDARY_MODULE);
}

void dpl_symbols_free(DPL_SymbolStack *stack)
{
    arena_free(&stack->memory);
}

void dpl_symbols_print_signature(DPL_Symbol *symbol, Nob_String_Builder *sb)
{
    nob_sb_append_sv(sb, symbol->name);

    switch (symbol->kind)
    {
    case SYMBOL_CONSTANT:
    {
        nob_sb_append_cstr(sb, ": ");
        dpl_symbols_print_signature(symbol->as.constant.type, sb);
    }
    break;
    case SYMBOL_FUNCTION:
    {
        DPL_Symbol_Type_Signature signature = symbol->as.function.signature;
        nob_sb_append_cstr(sb, "(");
        for (size_t i = 0; i < signature.argument_count; ++i)
        {
            if (i > 0)
            {
                nob_sb_append_cstr(sb, ", ");
            }
            dpl_symbols_print_signature(signature.arguments[i], sb);
        }
        nob_sb_append_cstr(sb, "): ");
        dpl_symbols_print_signature(signature.returns, sb);
    }
    break;
    case SYMBOL_VAR:
    {
        nob_sb_append_cstr(sb, ": ");
        dpl_symbols_print_signature(symbol->as.var.type, sb);
    }
    break;
    case SYMBOL_ARGUMENT:
    {
        nob_sb_append_cstr(sb, ": ");
        dpl_symbols_print_signature(symbol->as.argument.type, sb);
    }
    break;
    default:
        break;
    }
}

void dpl_symbols_print_full_signature(DPL_Symbol *symbol, Nob_String_Builder *sb)
{
    nob_sb_append_cstr(sb, SYMBOL_KIND_NAMES[symbol->kind]);

    if (symbol->name.count > 0)
    {
        nob_sb_append_cstr(sb, " ");
        dpl_symbols_print_signature(symbol, sb);
    }
}

static void dpl_symbols_print_flags(DPL_Symbol *symbol, Nob_String_Builder *sb)
{
    switch (symbol->kind)
    {
    case SYMBOL_BOUNDARY:
    {
        nob_sb_append_cstr(sb, SYMBOL_BOUNDARY_KIND_NAMES[symbol->as.boundary]);
    }
    break;
    case SYMBOL_TYPE:
    {
        switch (symbol->as.type.kind)
        {
        case TYPE_BASE:
        {
            nob_sb_append_format(sb, "base: %s", SYMBOL_TYPE_BASE_KIND_NAMES[symbol->as.type.as.base]);
        }
        break;
        case TYPE_OBJECT:
        {
            nob_sb_append_cstr(sb, "object");
        }
        break;
        default:
            DW_UNIMPLEMENTED;
        }
    }
    break;
    case SYMBOL_CONSTANT:
    {
        DPL_Symbol_Type *type = &symbol->as.constant.type->as.type;
        switch (type->kind)
        {
        case TYPE_BASE:
        {
            switch (type->as.base)
            {
            case TYPE_BASE_NUMBER:
            {
                nob_sb_append_format(sb, "value: %f", symbol->as.constant.as.number);
            }
            break;
            default:
                DW_UNIMPLEMENTED_MSG("Unimplemented base type kind %d.", type->as.base);
            }
        }
        break;
        default:
            DW_UNIMPLEMENTED;
        }
    }
    break;
    case SYMBOL_FUNCTION:
    {
        switch (symbol->as.function.kind)
        {
        case FUNCTION_INSTRUCTION:
        {
            nob_sb_append_cstr(sb, "instruction: ");
            nob_sb_append_cstr(sb, dplp_inst_kind_name(symbol->as.function.as.instruction_function));
        }
        break;
        case FUNCTION_INTRINSIC:
        {
            nob_sb_append_format(sb, "intrinsic: #%2zu", symbol->as.function.as.intrinsic_function);
        }
        break;
        case FUNCTION_USER:
            nob_sb_append_cstr(sb, " user");
            break;
        default:
            DW_UNIMPLEMENTED;
        }
    }
    break;
    case SYMBOL_VAR:
    {
        nob_sb_append_format(sb, "scope_index: %d", symbol->as.var.scope_index);
    }
    break;
    case SYMBOL_ARGUMENT:
    {
        nob_sb_append_format(sb, "scope_index: %d", symbol->as.argument.scope_index);
    }
    break;
    default:
        break;
    }
}

static void dpl_symbols_print_table_row(DPL_Symbol *symbol, Nob_String_Builder *sb)
{
    Nob_String_Builder signature_builder = {0};
    dpl_symbols_print_signature(symbol, &signature_builder);
    nob_sb_append_null(&signature_builder);

    Nob_String_Builder flags_builder = {0};
    dpl_symbols_print_flags(symbol, &flags_builder);
    nob_sb_append_null(&flags_builder);

    nob_sb_append_format(sb, "| %3d | %3d | %-8s | %-40s | %-30s | ",
                         symbol->boundary_count,
                         symbol->stack_index,
                         SYMBOL_KIND_NAMES[symbol->kind],
                         signature_builder.items,
                         flags_builder.items);

    nob_sb_free(signature_builder);
    nob_sb_free(flags_builder);
}

void dpl_symbols_print_table(DPL_SymbolStack *stack)
{
    Nob_String_Builder sb = {0};
    for (size_t i = 0; i < stack->entries_count; ++i)
    {
        sb.count = 0;
        dpl_symbols_print_table_row(stack->entries[i], &sb);
        nob_sb_append_null(&sb);
        printf("%s\n", sb.items);
    }
    nob_sb_free(sb);
}
