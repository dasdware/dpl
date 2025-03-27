#include <dpl/parser.h>
#include <error.h>

const char *AST_NODE_KIND_NAMES[COUNT_AST_NODE_KINDS] = {

    [AST_NODE_LITERAL] = "AST_NODE_LITERAL",
    [AST_NODE_OBJECT_LITERAL] = "AST_NODE_OBJECT_LITERAL",
    [AST_NODE_UNARY] = "AST_NODE_UNARY",
    [AST_NODE_BINARY] = "AST_NODE_BINARY",
    [AST_NODE_FUNCTIONCALL] = "AST_NODE_FUNCTIONCALL",
    [AST_NODE_SCOPE] = "AST_NODE_SCOPE",
    [AST_NODE_DECLARATION] = "AST_NODE_DECLARATION",
    [AST_NODE_SYMBOL] = "AST_NODE_SYMBOL",
    [AST_NODE_ASSIGNMENT] = "AST_NODE_ASSIGNMENT",
    [AST_NODE_FUNCTION] = "AST_NODE_FUNCTION",
    [AST_NODE_CONDITIONAL] = "AST_NODE_CONDITIONAL",
    [AST_NODE_WHILE_LOOP] = "AST_NODE_WHILE_LOOP",
    [AST_NODE_FIELD_ACCESS] = "AST_NODE_FIELD_ACCESS",
    [AST_NODE_INTERPOLATION] = "AST_NODE_INTERPOLATION",
};

static_assert(COUNT_AST_NODE_KINDS == 14,
              "Count of ast node kinds has changed, please update bound node kind names map.");

const char *dpl_parse_nodekind_name(DPL_AstNodeKind kind)
{
    if (kind < 0 || kind >= COUNT_AST_NODE_KINDS)
    {
        DW_UNIMPLEMENTED_MSG("%d", kind);
    }
    return AST_NODE_KIND_NAMES[kind];
}
