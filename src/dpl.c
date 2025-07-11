#ifdef DPL_LEAKCHECK
#include "stb_leakcheck.h"
#endif

#include <dpl/utils.h>
#include <dpl/generator.h>

#include "dpl.h"
#include "error.h"

#define DPL_ERROR DW_ERROR

void dpl_init(DPL *dpl)
{
    // SYMBOL STACK
    dpl_symbols_init(&dpl->symbols);

    // Base types
    DPL_Symbol *number_t = dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_NUMBER, TYPE_BASE_NUMBER);
    DPL_Symbol *string_t = dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_STRING, TYPE_BASE_STRING);
    DPL_Symbol *boolean_t = dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_BOOLEAN, TYPE_BASE_BOOLEAN);
    dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_NONE, TYPE_BASE_NONE);
    dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_EMPTY_ARRAY, TYPE_BASE_EMPTY_ARRAY);

    // Operators on base types

    // unary operators
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "negate", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER), INST_NEGATE);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "not", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_BOOLEAN), INST_NOT);

    // binary operators
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "add", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_ADD);
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

    // intrinsic functions
    DPL_Symbol_Type_ObjectQuery query = {0};

    // Number range type
    query.count = 0;
    nob_da_append(&query, DPL_OBJECT_FIELD("from", number_t));
    nob_da_append(&query, DPL_OBJECT_FIELD("to", number_t));
    DPL_Symbol *number_range_t = dpl_symbols_check_type_object_query(&dpl->symbols, query);

    // Number iterator type
    query.count = 0;
    nob_da_append(&query, DPL_OBJECT_FIELD("current", number_t));
    nob_da_append(&query, DPL_OBJECT_FIELD("finished", boolean_t));
    nob_da_append(&query, DPL_OBJECT_FIELD("to", number_t));
    DPL_Symbol *number_iterator_t = dpl_symbols_check_type_object_query(&dpl->symbols, query);

    nob_da_free(query);

    dpl_symbols_push_function_intrinsic(&dpl->symbols, "print", boolean_t, DPL_SYMBOLS(boolean_t), INTRINSIC_BOOLEAN_PRINT);
    dpl_symbols_push_function_intrinsic(&dpl->symbols, "toString", string_t, DPL_SYMBOLS(boolean_t), INTRINSIC_BOOLEAN_TOSTRING);

    dpl_symbols_push_function_intrinsic(&dpl->symbols, "print", number_t, DPL_SYMBOLS(number_t), INTRINSIC_NUMBER_PRINT);
    dpl_symbols_push_function_intrinsic(&dpl->symbols, "toString", string_t, DPL_SYMBOLS(number_t), INTRINSIC_NUMBER_TOSTRING);
    dpl_symbols_push_function_intrinsic(&dpl->symbols, "next", number_iterator_t, DPL_SYMBOLS(number_iterator_t), INTRINSIC_NUMBERITERATOR_NEXT);
    dpl_symbols_push_function_intrinsic(&dpl->symbols, "iterator", number_iterator_t, DPL_SYMBOLS(number_range_t), INTRINSIC_NUMBERRANGE_ITERATOR);

    dpl_symbols_push_function_intrinsic(&dpl->symbols, "length", number_t, DPL_SYMBOLS(string_t), INTRINSIC_STRING_LENGTH);
    dpl_symbols_push_function_intrinsic(&dpl->symbols, "print", string_t, DPL_SYMBOLS(string_t), INTRINSIC_STRING_PRINT);
}

void dpl_free(DPL *dpl)
{
    // symbol stack freeing
    dpl_symbols_free(&dpl->symbols);
}

// COMPILATION PROCESS

void dpl_compile(DPL *dpl, DPL_Program *program)
{
    // lexer initialization
    DPL_Lexer lexer = {0};
    lexer.file_name = dpl->file_name;
    lexer.source = dpl->source;
    lexer.current_line = lexer.source.data;

    DPL_Parser parser = {
        .memory = dpl->memory,
        .lexer = &lexer,
    };

    DPL_Ast_Node *root_expression = dpl_parse(&parser);
    if (dpl->debug)
    {
        dpl_parse_print(root_expression, 0);
        printf("\n");
    }

    DPL_Binding binding = (DPL_Binding){
        .memory = dpl->memory,
        .source = dpl->source,
        .symbols = &dpl->symbols,
        .user_functions = {0},
    };
    DPL_Bound_Node *bound_root_expression = dpl_bind_node(&binding, root_expression);

    if (dpl->debug)
    {
        for (size_t i = 0; i < binding.user_functions.count; ++i)
        {
            DPL_Binding_UserFunction *uf = &binding.user_functions.items[i];
            printf("### " SV_Fmt " (arity: %zu) ###\n", SV_Arg(uf->function->name), uf->arity);
            dpl_bind_print(&binding, uf->body, 0);
            printf("\n");
        }

        printf("### program ###\n");
        dpl_bind_print(&binding, bound_root_expression, 0);
        printf("\n");
    }

    DPL_Generator generator = {
        .user_functions = binding.user_functions,
    };
    for (size_t i = 0; i < generator.user_functions.count; ++i)
    {
        DPL_Binding_UserFunction *uf = &generator.user_functions.items[i];
        uf->begin_ip = program->code.count;
        dpl_generate(&generator, uf->body, program);
        dplp_write_return(program);
    }

    program->entry = program->code.count;
    dpl_generate(&generator, bound_root_expression, program);
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

    nob_da_free(binding.user_functions);
}