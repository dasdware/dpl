#include "ast.h"

void dpla_init(DPL_Ast_Tree* tree)
{
    *tree = (DPL_Ast_Tree) {
        0
    };
}

void dpla_free(DPL_Ast_Tree* tree)
{
    arena_free(&tree->memory);
    dpla_init(tree);
}

DPL_Ast_Node*  dpla_create_node(DPL_Ast_Tree* tree, DPL_AstNodeKind kind)
{
    DPL_Ast_Node* node = arena_alloc(&tree->memory, sizeof(DPL_Ast_Node));
    node->kind = kind;
    return node;
}

const char* dpla_node_kind_name(DPL_AstNodeKind kind) {
    switch (kind) {
    case AST_NODE_LITERAL:
        return "AST_NODE_LITERAL";
    case AST_NODE_UNARY:
        return "AST_NODE_UNARY";
    case AST_NODE_BINARY:
        return "AST_NODE_BINARY";
    }

    assert(false && "unreachable: dpla_node_kind_name");
    return "";
}

void dpla_print(DPL_Ast_Node* node, size_t level) {
    for (size_t i = 0; i < level; ++i) {
        printf("  ");
    }

    if (!node) {
        printf("<nil>\n");
        return;
    }

    printf("%s", dpla_node_kind_name(node->kind));

    switch (node->kind) {
    case AST_NODE_UNARY: {
        printf(" [%s]\n", dpll_token_kind_name(node->as.unary.operator.kind));
        dpla_print(node->as.unary.operand, level + 1);
        break;
    }
    case AST_NODE_BINARY: {
        printf(" [%s]\n", dpll_token_kind_name(node->as.binary.operator.kind));
        dpla_print(node->as.binary.left, level + 1);
        dpla_print(node->as.binary.right, level + 1);
        break;
    }
    case AST_NODE_LITERAL: {
        printf(" [%s: "SV_Fmt"]\n", dpll_token_kind_name(node->as.literal.value.kind), SV_Arg(node->as.literal.value.text));
        break;
    }
    default: {
        printf("\n");
        break;
    }
    }
}