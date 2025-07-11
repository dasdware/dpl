#ifndef __DPL_SYMBOLS_H
#define __DPL_SYMBOLS_H

#include <arena.h>
#include <nobx.h>

#include <dpl/intrinsics.h>
#include <dpl/program.h>
#include <dpl/utils.h>

#define DPL_SYMBOLS(...) \
    DPL_ARG_COUNT(__VA_ARGS__), (DPL_Symbol *[]) { __VA_ARGS__ }

#define DPL_OBJECT_FIELD(field_name, field_type) \
    ((DPL_Symbol_Type_ObjectField){              \
        .name = nob_sv_from_cstr(field_name),    \
        .type = field_type})

typedef struct DPL_Symbol DPL_Symbol;

typedef enum
{
    SYMBOL_BOUNDARY,
    SYMBOL_TYPE,
    SYMBOL_CONSTANT,
    SYMBOL_VAR,
    SYMBOL_FUNCTION,
    SYMBOL_ARGUMENT,

    COUNT_SYMBOL_KINDS,
} DPL_Symbol_Kind;

// BOUNDARIES

typedef enum
{
    BOUNDARY_MODULE,
    BOUNDARY_SCOPE,
    BOUNDARY_FUNCTION,

    COUNT_SYMBOL_BOUNDARY_KINDS,
} DPL_Symbol_Boundary_Kind;

// TYPES

typedef enum
{
    TYPE_BASE,
    TYPE_FUNCTION,
    TYPE_OBJECT,
    TYPE_ALIAS,
    TYPE_ARRAY,
    TYPE_MULTI,

    COUNT_SYMBOL_TYPE_KINDS,
} DPL_Symbol_Type_Kind;

typedef enum
{
    TYPE_BASE_NUMBER,
    TYPE_BASE_STRING,
    TYPE_BASE_BOOLEAN,
    TYPE_BASE_NONE,
    TYPE_BASE_EMPTY_ARRAY,

    COUNT_SYMBOL_TYPE_BASE_KINDS,
} DPL_Symbol_Type_Base_Kind;

#define TYPENAME_NUMBER "Number"
#define TYPENAME_STRING "String"
#define TYPENAME_BOOLEAN "Boolean"
#define TYPENAME_NONE "None"
#define TYPENAME_EMPTY_ARRAY "[]"

typedef struct
{
    Nob_String_View name;
    DPL_Symbol *type;
} DPL_Symbol_Type_ObjectField;

typedef struct
{
    DPL_Symbol_Type_ObjectField *items;
    size_t count;
    size_t capacity;
} DPL_Symbol_Type_ObjectQuery;

typedef struct
{
    size_t field_count;
    DPL_Symbol_Type_ObjectField *fields;
} DPL_Symbol_Type_Object;

typedef struct
{
    DPL_Symbol *element_type;
} DPL_Symbol_Type_Array;

typedef struct
{
    size_t argument_count;
    DPL_Symbol **arguments;
    DPL_Symbol *returns;
} DPL_Symbol_Type_Signature;

typedef struct
{
    DPL_Symbol_Type_Kind kind;
    union
    {
        DPL_Symbol_Type_Base_Kind base;
        DPL_Symbol_Type_Object object;
        DPL_Symbol_Type_Array array;
        DPL_Symbol_Type_Signature function;
        DPL_Symbol *alias;
        DPL_Symbol_Type_Array multi;
    } as;
} DPL_Symbol_Type;

// Constants

typedef struct DPL_Symbol_Constant DPL_Symbol_Constant;

typedef struct
{
    Nob_String_View name;
    DPL_Symbol_Constant *value;
} DPL_Symbol_Constant_ObjectField;

typedef struct
{
    size_t field_count;
    DPL_Symbol_Constant_ObjectField *fields;
} DPL_Symbol_Constant_Object;

struct DPL_Symbol_Constant
{
    DPL_Symbol *type;
    union
    {
        double number;
        Nob_String_View string;
        bool boolean;

        DPL_Symbol_Constant_Object object;
    } as;
};

// Variables

typedef struct
{
    DPL_Symbol *type;
    size_t scope_index;
} DPL_Symbol_Var;

// Functions

typedef enum
{
    FUNCTION_INSTRUCTION,
    FUNCTION_USER,
    FUNCTION_INTRINSIC,

    COUNT_SYMBOL_FUNCTION_KINDS,
} DPL_Symbol_Function_Kind;

typedef struct
{
    DPL_Symbol_Type_Signature signature;
    DPL_Symbol_Function_Kind kind;
    union
    {
        DPL_Instruction_Kind instruction_function;
        struct
        {
            void *body;
            bool used;
            size_t user_handle;
            size_t function_handle;
        } user_function;
        DPL_Intrinsic_Kind intrinsic_function;
    } as;
} DPL_Symbol_Function;

// Symbols

struct DPL_Symbol
{
    DPL_Symbol_Kind kind;
    Nob_String_View name;
    union
    {
        DPL_Symbol_Boundary_Kind boundary;
        DPL_Symbol_Constant constant;
        DPL_Symbol_Var var;
        DPL_Symbol_Var argument;
        DPL_Symbol_Function function;
        DPL_Symbol_Type type;
    } as;

    size_t boundary_count;
    int stack_index;
};

typedef struct
{
    DPL_Symbol **items;
    size_t count;
    size_t capacity;
} DPL_Symbols;

// Symbol stack

typedef struct
{
    DPL_Symbol **entries;
    size_t entries_count;
    size_t entries_capacity;

    Arena memory;
} DPL_SymbolStack;

// API

const char *dpl_symbols_kind_name(DPL_Symbol_Kind kind);

void dpl_symbols_init(DPL_SymbolStack *stack);
void dpl_symbols_free(DPL_SymbolStack *stack);
const char *dpl_symbols_last_error();

DPL_Symbol *dpl_symbols_find(DPL_SymbolStack *stack, Nob_String_View name);
DPL_Symbol *dpl_symbols_find_cstr(DPL_SymbolStack *stack, const char *name);
DPL_Symbol *dpl_symbols_find_kind(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol_Kind kind);
DPL_Symbol *dpl_symbols_find_kind_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol_Kind kind);
DPL_Symbol *dpl_symbols_find_type_base(DPL_SymbolStack *stack, DPL_Symbol_Type_Base_Kind kind);
#define dpl_symbols_find_type_number(symbols) dpl_symbols_find_type_base((symbols), TYPE_BASE_NUMBER)
#define dpl_symbols_find_type_string(symbols) dpl_symbols_find_type_base((symbols), TYPE_BASE_STRING)
#define dpl_symbols_find_type_boolean(symbols) dpl_symbols_find_type_base((symbols), TYPE_BASE_BOOLEAN)
#define dpl_symbols_find_type_none(symbols) dpl_symbols_find_type_base((symbols), TYPE_BASE_NONE)
#define dpl_symbols_find_type_empty_array(symbols) dpl_symbols_find_type_base((symbols), TYPE_BASE_EMPTY_ARRAY)
DPL_Symbol *dpl_symbols_find_type_object_query(DPL_SymbolStack *stack, DPL_Symbol_Type_ObjectQuery query);
DPL_Symbol *dpl_symbols_find_type_array_query(DPL_SymbolStack *stack, DPL_Symbol *element_type);
DPL_Symbol *dpl_symbols_find_type_multi_query(DPL_SymbolStack *stack, DPL_Symbol *element_type);
DPL_Symbol *dpl_symbols_find_function(DPL_SymbolStack *stack, Nob_String_View name, size_t arguments_count, DPL_Symbol **arguments);
DPL_Symbol *dpl_symbols_find_function1(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol *arg0);
DPL_Symbol *dpl_symbols_find_function1_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol *arg0);
DPL_Symbol *dpl_symbols_find_function2(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol *arg0, DPL_Symbol *arg1);
DPL_Symbol *dpl_symbols_find_function2_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol *arg0, DPL_Symbol *arg1);

DPL_Symbol *dpl_symbols_check_type_object_query(DPL_SymbolStack *stack, DPL_Symbol_Type_ObjectQuery query);
DPL_Symbol *dpl_symbols_check_type_array_query(DPL_SymbolStack *stack, DPL_Symbol *element_type);
DPL_Symbol *dpl_symbols_check_type_multi_query(DPL_SymbolStack *stack, DPL_Symbol *element_type);

DPL_Symbol *dpl_symbols_resolve_type_alias(DPL_Symbol *type);
bool dpl_symbols_type_assignable(DPL_Symbol *from, DPL_Symbol *to);

// Common
DPL_Symbol *dpl_symbols_push(DPL_SymbolStack *stack, DPL_Symbol_Kind kind, Nob_String_View name);
DPL_Symbol *dpl_symbols_push_cstr(DPL_SymbolStack *stack, DPL_Symbol_Kind kind, const char *name);

// Boundaries
DPL_Symbol *dpl_symbols_push_boundary_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol_Boundary_Kind kind);
bool dpl_symbols_pop_boundary(DPL_SymbolStack *stack);

// Types
DPL_Symbol *dpl_symbols_push_type_base_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol_Type_Base_Kind base_kind);
DPL_Symbol *dpl_symbols_push_type_object_cstr(DPL_SymbolStack *stack, const char *name, size_t field_count);
DPL_Symbol *dpl_symbols_push_type_array_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol *element_type);
DPL_Symbol *dpl_symbols_push_type_multi_cstr(DPL_SymbolStack *stack, const char *name, DPL_Symbol *element_type);
DPL_Symbol *dpl_symbols_push_type_alias(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol *type);

bool dpl_symbols_is_type_base(DPL_Symbol *symbol, DPL_Symbol_Type_Base_Kind kind);
bool dpl_symbols_is_type_array(DPL_Symbol *symbol);

// Constants
DPL_Symbol *dpl_symbols_push_constant_number_cstr(DPL_SymbolStack *stack, const char *name, double value);

// Functions
DPL_Symbol *dpl_symbols_push_function_instruction_cstr(DPL_SymbolStack *stack,
                                                       const char *name, const char *return_type, size_t argument_count, const char **argument_types,
                                                       DPL_Instruction_Kind instruction);
DPL_Symbol *dpl_symbols_push_function_intrinsic(DPL_SymbolStack *stack,
                                                const char *name, DPL_Symbol *return_type, size_t argument_count, DPL_Symbol **argument_types,
                                                DPL_Intrinsic_Kind intrinsic);
DPL_Symbol *dpl_symbols_push_function_intrinsic_cstr(DPL_SymbolStack *stack,
                                                     const char *name, const char *return_type, size_t argument_count, const char **argument_types,
                                                     DPL_Intrinsic_Kind intrinsic);
DPL_Symbol *dpl_symbols_push_function_user(DPL_SymbolStack *stack, Nob_String_View name, size_t argument_count);

// Variables
DPL_Symbol *dpl_symbols_push_var(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol *type);
DPL_Symbol *dpl_symbols_push_var_cstr(DPL_SymbolStack *stack, const char *name, const char *type);

// Arguments
DPL_Symbol *dpl_symbols_push_argument(DPL_SymbolStack *stack, Nob_String_View name, DPL_Symbol *type);
DPL_Symbol *dpl_symbols_push_argument_cstr(DPL_SymbolStack *stack, const char *name, const char *type_name);

// Printing
void dpl_symbols_print_signature(DPL_Symbol *symbol, Nob_String_Builder *sb);
void dpl_symbols_print_full_signature(DPL_Symbol *symbol, Nob_String_Builder *sb);
void dpl_symbols_print_table(DPL_SymbolStack *stack);

#endif // __DPL_SYMBOLS_H