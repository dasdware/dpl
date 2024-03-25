#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "functions.h"
#include "calltree.h"
#include "error.h"
#include "bytecode.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char** argv) {
    const char* program = nob_shift_args(&argc, &argv);

    if (argc <= 0) {
        fprintf(stderr, "Usage: %s sourceFileName\n", program);
        exit(1);
    }

    DPL_Types types = {0};
    dplt_init(&types);

    // for (size_t i = 0; i < types.count; ++i) {
    //     printf("* %zu: ", types.items[i].handle);
    //     dplt_print(stdout, &types, &types.items[i]);
    //     printf("\n");
    // }
    // printf("\n");

    DPL_Functions functions = {0};
    dplf_init(&functions, &types);

    // for (size_t i = 0; i < functions.count; ++i) {
    //     printf("* %zu: ", functions.items[i].handle);
    //     dplf_print(stdout, &types, &functions.items[i]);
    //     printf("\n");
    // }
    // printf("\n");

    const char* source_filename = nob_shift_args(&argc, &argv);

    Nob_String_Builder source = {0};
    nob_read_entire_file(source_filename, &source);

    DPL_Lexer lexer;
    dpll_init(&lexer, source_filename, nob_sv_from_parts(source.items, source.count));

    DPL_Parser parser;
    dplp_init(&parser, &lexer);
    dplp_parse(&parser);

    DPL_CallTree call_tree = {0};
    dplc_init(&call_tree, &types, &functions, &lexer, &parser);
    dplc_bind(&call_tree);

    DPL_ByteCode bytecode = {0};
    dplb_generate(&call_tree, call_tree.root, &bytecode);
    dplb_print(&bytecode);
    printf("\n");

    dplb_free(&bytecode);
    dplc_free(&call_tree);
    dplp_free(&parser);
    dplf_free(&functions);
    dplt_free(&types);

    nob_da_free(source);

    return 0;
}