#ifdef DPL_LEAKCHECK
#include "stb_leakcheck.h"
#endif

#include <dpl/utils.h>
#include <dpl/generator.h>

#include "dpl.h"
#include "error.h"

#define DPL_ERROR DW_ERROR

#define DPL_OBJECT_FIELD(field_name, field_type) \
    ((DPL_Symbol_Type_ObjectField){              \
        .name = nob_sv_from_cstr(field_name),    \
        .type = field_type})

void dpl_init(DPL *dpl, DPL_ExternalFunctions externals)
{
    // SYMBOL STACK
    dpl_symbols_init(&dpl->symbols);

    // Base types
    DPL_Symbol *number_t = dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_NUMBER, TYPE_BASE_NUMBER);
    dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_STRING, TYPE_BASE_STRING);
    DPL_Symbol *boolean_t = dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_BOOLEAN, TYPE_BASE_BOOLEAN);
    dpl_symbols_push_type_base_cstr(&dpl->symbols, TYPENAME_NONE, TYPE_BASE_NONE);

    // Operators on base types

    // unary operators
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "negate", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER), INST_NEGATE);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "not", TYPENAME_BOOLEAN, DPL_ARGS(TYPENAME_BOOLEAN), INST_NOT);

    // binary operators
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "add", TYPENAME_NUMBER, DPL_ARGS(TYPENAME_NUMBER, TYPENAME_NUMBER), INST_ADD);
    dpl_symbols_push_function_instruction_cstr(&dpl->symbols, "add", TYPENAME_STRING, DPL_ARGS(TYPENAME_STRING, TYPENAME_STRING), INST_ADD);
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
    DPL_Symbol_Type_ObjectQuery query = NULL;

    // Number range type
    da_clear(query);
    da_add(query, DPL_OBJECT_FIELD("from", number_t));
    da_add(query, DPL_OBJECT_FIELD("to", number_t));
    DPL_Symbol *number_range_t = dpl_symbols_check_type_object_query(&dpl->symbols, query);

    // Number iterator type
    da_clear(query);
    da_add(query, DPL_OBJECT_FIELD("current", number_t));
    da_add(query, DPL_OBJECT_FIELD("finished", boolean_t));
    da_add(query, DPL_OBJECT_FIELD("to", number_t));
    DPL_Symbol *number_iterator_t = dpl_symbols_check_type_object_query(&dpl->symbols, query);

    da_free(query);

    dpl_symbols_push_function_intrinsic(&dpl->symbols, "iterator", number_iterator_t, DPL_SYMBOLS(number_range_t), INTRINSIC_NUMBER_ITERATOR);
    dpl_symbols_push_function_intrinsic(&dpl->symbols, "next", number_iterator_t, DPL_SYMBOLS(number_iterator_t), INTRINSIC_NUMBER_ITERATOR_NEXT);

    // externals
    for (size_t i = 0; i < da_size(externals); ++i)
    {
        DPL_Symbol *function = dpl_symbols_push_function_external_cstr(
            &dpl->symbols,
            externals[i].name, externals[i].return_type,
            da_size(externals[i].argument_types), externals[i].argument_types,
            i);

        if (!function)
        {
            Nob_String_Builder sb = {0};
            nob_sb_append_cstr(&sb, "Cannot add external function `");
            nob_sb_append_cstr(&sb, externals[i].name);
            nob_sb_append_cstr(&sb, "(");

            for (size_t j = 0; j < da_size(externals[i].argument_types); ++j)
            {
                if (j > 0)
                {
                    nob_sb_append_cstr(&sb, ", ");
                }
                nob_sb_append_cstr(&sb, externals[i].argument_types[j]);
            }
            nob_sb_append_cstr(&sb, "): ");
            nob_sb_append_cstr(&sb, externals[i].return_type);
            nob_sb_append_cstr(&sb, "`: ");
            nob_sb_append_cstr(&sb, dpl_symbols_last_error());

            nob_sb_append_null(&sb);

            DW_ERROR("%s", sb.items);
        }
    }
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
        .user_functions = 0,
    };
    DPL_Bound_Node *bound_root_expression = dpl_bind_node(&binding, root_expression);

    if (dpl->debug)
    {
        for (size_t i = 0; i < da_size(binding.user_functions); ++i)
        {
            DPL_Binding_UserFunction *uf = &binding.user_functions[i];
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
    for (size_t i = 0; i < da_size(generator.user_functions); ++i)
    {
        DPL_Binding_UserFunction *uf = &generator.user_functions[i];
        uf->begin_ip = da_size(program->code);
        dpl_generate(&generator, uf->body, program);
        dplp_write_return(program);
    }

    program->entry = da_size(program->code);
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

    da_free(binding.user_functions);
}