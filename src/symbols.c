#include <dpl/symbols.h>

#include <error.h>

#define ABORT_IF_NULL(value) \
    if (value == NULL)       \
    {                        \
        return NULL;         \
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
    if (stack->entries_count > 0)
    {
        bool function_boundary_crossed = false;
        for (int i = stack->entries_count - 1; i >= 0; --i)
        {
            DPL_Symbol *candidate = stack->entries[i];
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
        }
    }

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

DPL_Symbol *dpl_symbols_push_constant_number_cstr(DPL_SymbolStack *stack, const char *name, double value)
{
    // TODO: Extract base type names into #defines
    DPL_Symbol *type = dpl_symbols_find_kind(stack, nob_sv_from_cstr("Number"), SYMBOL_TYPE);
    ABORT_IF_NULL(type);

    DPL_Symbol *symbol = dpl_symbols_push_cstr(stack, SYMBOL_CONSTANT, name);
    ABORT_IF_NULL(symbol);

    symbol->as.constant = (DPL_Symbol_Constant){
        .type = type,
        .as.number = value,
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

DPL_Symbol *dpl_symbols_push_function_external_cstr(DPL_SymbolStack *stack, const char *name, const char *return_type,
                                                    size_t argument_count, const char **argument_types,
                                                    size_t index)
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
        .kind = FUNCTION_EXTERNAL,
        .as.external_function = index,
    };
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

    // Base types
    dpl_symbols_push_type_base_cstr(stack, "Number", TYPE_BASE_NUMBER);
    dpl_symbols_push_type_base_cstr(stack, "String", TYPE_BASE_STRING);
    dpl_symbols_push_type_base_cstr(stack, "Boolean", TYPE_BASE_BOOLEAN);
    dpl_symbols_push_type_base_cstr(stack, "None", TYPE_BASE_NONE);

    // Operators on base types

    // unary operators
    dpl_symbols_push_function_instruction_cstr(stack, "negate", "Number", DPL_ARG_TYPES(1, "Number"), INST_NEGATE);
    dpl_symbols_push_function_instruction_cstr(stack, "not", "Number", DPL_ARG_TYPES(1, "Boolean"), INST_NOT);

    // binary operators
    dpl_symbols_push_function_instruction_cstr(stack, "add", "Number", DPL_ARG_TYPES(2, "Number", "Number"), INST_ADD);
    dpl_symbols_push_function_instruction_cstr(stack, "add", "String", DPL_ARG_TYPES(2, "String", "String"), INST_ADD);
    dpl_symbols_push_function_instruction_cstr(stack, "subtract", "Number", DPL_ARG_TYPES(2, "Number", "Number"), INST_SUBTRACT);
    dpl_symbols_push_function_instruction_cstr(stack, "multiply", "Number", DPL_ARG_TYPES(2, "Number", "Number"), INST_MULTIPLY);
    dpl_symbols_push_function_instruction_cstr(stack, "divide", "Number", DPL_ARG_TYPES(2, "Number", "Number"), INST_DIVIDE);

    // comparison operators
    dpl_symbols_push_function_instruction_cstr(stack, "less", "Boolean", DPL_ARG_TYPES(2, "Number", "Number"), INST_LESS);
    dpl_symbols_push_function_instruction_cstr(stack, "lessEqual", "Boolean", DPL_ARG_TYPES(2, "Number", "Number"), INST_LESS_EQUAL);
    dpl_symbols_push_function_instruction_cstr(stack, "greater", "Boolean", DPL_ARG_TYPES(2, "Number", "Number"), INST_GREATER);
    dpl_symbols_push_function_instruction_cstr(stack, "greaterEqual", "Boolean", DPL_ARG_TYPES(2, "Number", "Number"), INST_GREATER_EQUAL);
    dpl_symbols_push_function_instruction_cstr(stack, "equal", "Boolean", DPL_ARG_TYPES(2, "Number", "Number"), INST_EQUAL);
    dpl_symbols_push_function_instruction_cstr(stack, "notEqual", "Boolean", DPL_ARG_TYPES(2, "Number", "Number"), INST_NOT_EQUAL);
    dpl_symbols_push_function_instruction_cstr(stack, "equal", "Boolean", DPL_ARG_TYPES(2, "String", "String"), INST_EQUAL);
    dpl_symbols_push_function_instruction_cstr(stack, "notEqual", "Boolean", DPL_ARG_TYPES(2, "String", "String"), INST_NOT_EQUAL);
    dpl_symbols_push_function_instruction_cstr(stack, "equal", "Boolean", DPL_ARG_TYPES(2, "Boolean", "Boolean"), INST_EQUAL);
    dpl_symbols_push_function_instruction_cstr(stack, "notEqual", "Boolean", DPL_ARG_TYPES(2, "Boolean", "Boolean"), INST_NOT_EQUAL);
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
        case FUNCTION_EXTERNAL:
            nob_sb_append_format(sb, "external: %zu", symbol->as.function.as.external_function);
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
        nob_sb_append_cstr(&sb, "\n");
        nob_sb_append_null(&sb);
        printf(sb.items);
    }
    nob_sb_free(sb);
}
