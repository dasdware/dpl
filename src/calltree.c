#include "calltree.h"

void dplc_init(DPL_CallTree *call_tree, DPL_Types *types, DPL_Functions *functions,
               DPL_Lexer *lexer, DPL_Parser *parser)
{
    call_tree->types = types;
    call_tree->functions = functions;
    call_tree->lexer = lexer;
    call_tree->parser = parser;
}

void dplc_free(DPL_CallTree *call_tree)
{
    arena_free(&call_tree->memory);
}

DPL_CallTree_Node* _dplc_bind_node(DPL_CallTree* call_tree, DPL_Ast_Node* node);

DPL_CallTree_Node* _dplc_bind_unary(DPL_CallTree* call_tree, DPL_Ast_Node* node, const char* function_name)
{
    DPL_CallTree_Node* operand = _dplc_bind_node(call_tree, node->as.unary.operand);

    DPL_Function* function = dplf_find_by_signature1(
                                 call_tree->types, call_tree->functions,
                                 nob_sv_from_cstr(function_name),
                                 operand->type_handle);
    if (function) {
        DPL_Type* function_type = dplt_find_by_handle(call_tree->types, function->type);

        if (function_type) {
            DPL_CallTree_Node* calltree_node = arena_alloc(&call_tree->memory, sizeof(DPL_CallTree_Node));
            calltree_node->kind = CALLTREE_NODE_FUNCTION;
            calltree_node->type_handle = function_type->as.function.returns;

            calltree_node->as.function.function_handle = function->handle;
            nob_da_append(&calltree_node->as.function.arguments, operand);

            return calltree_node;
        }
    }

    DPL_Type* operand_type = dplt_find_by_handle(call_tree->types, operand->type_handle);

    DPL_Token operator_token = node->as.unary.operator;
    fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function \"%s("SV_Fmt")\" for unary operator \""SV_Fmt"\".\n",
            LOC_Arg(operator_token.location), function_name, SV_Arg(operand_type->name), SV_Arg(operator_token.text));
    dpll_print_token_location(stderr, call_tree->lexer, operator_token);

    exit(1);
}


DPL_CallTree_Node* _dplc_bind_binary(DPL_CallTree* call_tree, DPL_Ast_Node* node, const char* function_name)
{
    DPL_CallTree_Node* lhs = _dplc_bind_node(call_tree, node->as.binary.left);
    DPL_CallTree_Node* rhs = _dplc_bind_node(call_tree, node->as.binary.right);

    DPL_Function* function = dplf_find_by_signature2(
                                 call_tree->types, call_tree->functions,
                                 nob_sv_from_cstr(function_name),
                                 lhs->type_handle, rhs->type_handle);
    if (function) {
        DPL_Type* function_type = dplt_find_by_handle(call_tree->types, function->type);

        if (function_type) {
            DPL_CallTree_Node* calltree_node = arena_alloc(&call_tree->memory, sizeof(DPL_CallTree_Node));
            calltree_node->kind = CALLTREE_NODE_FUNCTION;
            calltree_node->type_handle = function_type->as.function.returns;

            calltree_node->as.function.function_handle = function->handle;
            nob_da_append(&calltree_node->as.function.arguments, lhs);
            nob_da_append(&calltree_node->as.function.arguments, rhs);

            return calltree_node;
        }
    }

    DPL_Type* lhs_type = dplt_find_by_handle(call_tree->types, lhs->type_handle);
    DPL_Type* rhs_type = dplt_find_by_handle(call_tree->types, rhs->type_handle);

    DPL_Token operator_token = node->as.binary.operator;
    fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function \"%s("SV_Fmt", "SV_Fmt")\" for binary operator \""SV_Fmt"\".\n",
            LOC_Arg(operator_token.location), function_name, SV_Arg(lhs_type->name), SV_Arg(rhs_type->name), SV_Arg(operator_token.text));
    dpll_print_token_location(stderr, call_tree->lexer, operator_token);

    exit(1);
}

DPL_CallTree_Node* _dplc_bind_node(DPL_CallTree* call_tree, DPL_Ast_Node* node)
{
    switch (node->kind)
    {
    case AST_NODE_LITERAL: {
        switch (node->as.literal.value.kind)
        {
        case TOKEN_NUMBER: {
            DPL_CallTree_Node* calltree_node = arena_alloc(&call_tree->memory, sizeof(DPL_CallTree_Node));
            calltree_node->kind = CALLTREE_NODE_VALUE;
            calltree_node->type_handle = call_tree->types->number_handle;
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
            return _dplc_bind_unary(call_tree, node, "negate");
        default:
            break;
        }

        fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function for unary operator \""SV_Fmt"\".\n",
                LOC_Arg(operator.location), SV_Arg(operator.text));
        dpll_print_token_location(stderr, call_tree->lexer, operator);
        exit(1);

    }
    case AST_NODE_BINARY: {
        DPL_Token operator = node->as.binary.operator;
        switch(operator.kind) {
        case TOKEN_PLUS:
            return _dplc_bind_binary(call_tree, node, "add");
        case TOKEN_MINUS:
            return _dplc_bind_binary(call_tree, node, "subtract");
        case TOKEN_STAR:
            return _dplc_bind_binary(call_tree, node, "multiply");
        case TOKEN_SLASH:
            return _dplc_bind_binary(call_tree, node, "divide");
        default:
            break;
        }

        fprintf(stderr, LOC_Fmt": ERROR: Cannot resolve function for binary operator \""SV_Fmt"\".\n",
                LOC_Arg(operator.location), SV_Arg(operator.text));
        dpll_print_token_location(stderr, call_tree->lexer, operator);
        exit(1);
    }
    break;
    default:
        break;
    }

    fprintf(stderr, "ERROR: Cannot resolve function call tree for AST node of kind \"%s\".\n",
            dpla_node_kind_name(node->kind));
    exit(1);
}

void dplc_bind(DPL_CallTree* call_tree)
{
    call_tree->root = _dplc_bind_node(call_tree, call_tree->parser->tree.root);
}

void dplc_print(DPL_CallTree* tree, DPL_CallTree_Node* node, size_t level) {
    for (size_t i = 0; i < level; ++i) {
        printf("  ");
    }

    if (!node) {
        printf("<nil>\n");
        return;
    }

    DPL_Type* type = dplt_find_by_handle(tree->types, node->type_handle);
    printf("["SV_Fmt"] ", SV_Arg(type->name));

    switch(node->kind) {
    case CALLTREE_NODE_FUNCTION: {
        DPL_Function* function = dplf_find_by_handle(tree->functions, node->as.function.function_handle);
        printf(SV_Fmt"(\n", SV_Arg(function->name));

        for (size_t i = 0; i < node->as.function.arguments.count; ++i) {
            dplc_print(tree, node->as.function.arguments.items[i], level + 1);
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