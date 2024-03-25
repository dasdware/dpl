#include "parser.h"
#include "ast.h"

void dplp_init(DPL_Parser *parser, DPL_Lexer *lexer)
{
    *parser = (DPL_Parser) {
        0
    };
    parser->lexer = lexer;
    dpla_init(&parser->tree);
}

void dplp_free(DPL_Parser *parser)
{
    dpla_free(&parser->tree);
    *parser = (DPL_Parser) {
        0
    };
}

void _dplp_error_unexpected_token(DPL_Parser* parser, DPL_Token token) __attribute__ ((noreturn));
void _dplp_error_unexpected_token(DPL_Parser* parser, DPL_Token token) {
    fprintf(stderr, LOC_Fmt ": Unexpected token <%s>:\n", LOC_Arg(token.location), dpll_token_kind_name(token.kind));
    dpll_print_token_location(stderr, parser->lexer, token);
    exit(1);
}

void _dplp_error_unexpected_token_expected(DPL_Parser* parser, DPL_Token token, DPL_TokenKind expected_kind) __attribute__ ((noreturn));
void _dplp_error_unexpected_token_expected(DPL_Parser* parser, DPL_Token token, DPL_TokenKind expected_kind) {
    fprintf(stderr, LOC_Fmt ": Unexpected token <%s>, expected <%s>:\n", LOC_Arg(token.location),
            dpll_token_kind_name(token.kind), dpll_token_kind_name(expected_kind));
    dpll_print_token_location(stderr, parser->lexer, token);
    exit(1);
}

void _dplp_skip_whitespace(DPL_Parser *parser) {
    while (dpll_peek_token(parser->lexer).kind == TOKEN_WHITESPACE) {
        dpll_next_token(parser->lexer);
    }
}

DPL_Token _dplp_next_token(DPL_Parser *parser) {
    _dplp_skip_whitespace(parser);
    return dpll_next_token(parser->lexer);
}

DPL_Token _dplp_peek_token(DPL_Parser *parser) {
    _dplp_skip_whitespace(parser);
    return dpll_peek_token(parser->lexer);
}

DPL_Token _dplp_expect_token(DPL_Parser *parser, DPL_TokenKind kind) {
    DPL_Token next_token = _dplp_next_token(parser);
    if (next_token.kind != kind) {
        _dplp_error_unexpected_token_expected(parser, next_token, kind);
    }
    return next_token;
}

DPL_Ast_Node* _dplp_parse_expression(DPL_Parser* parser);

DPL_Ast_Node* _dplp_parse_primary(DPL_Parser* parser)
{
    DPL_Token token = _dplp_next_token(parser);
    switch (token.kind) {
    case TOKEN_NUMBER:
        /*case TOKEN_IDENTIFIER:*/
    {
        DPL_Ast_Node* node = dpla_create_node(&parser->tree, AST_NODE_LITERAL);
        node->as.literal.value = token;
        return node;
    }
    break;
    case TOKEN_OPEN_PAREN: {
        DPL_Ast_Node* node = _dplp_parse_expression(parser);
        _dplp_expect_token(parser, TOKEN_CLOSE_PAREN);
        return node;
    }
    break;
    default: {
        _dplp_error_unexpected_token(parser, token);
    }
    }
}

DPL_Ast_Node* _dplp_parser_unary(DPL_Parser* parser)
{
    DPL_Token operator_candidate = _dplp_peek_token(parser);
    if (operator_candidate.kind == TOKEN_MINUS) {
        _dplp_next_token(parser);
        DPL_Ast_Node* operand = _dplp_parser_unary(parser);

        DPL_Ast_Node* new_expression = dpla_create_node(&parser->tree, AST_NODE_UNARY);
        new_expression->as.unary.operator = operator_candidate;
        new_expression->as.unary.operand = operand;
        return new_expression;
    }

    return _dplp_parse_primary(parser);
}

DPL_Ast_Node* _dplp_parse_multiplicative(DPL_Parser* parser)
{
    DPL_Ast_Node* expression = _dplp_parser_unary(parser);

    DPL_Token operator_candidate = _dplp_peek_token(parser);
    while (operator_candidate.kind == TOKEN_STAR || operator_candidate.kind == TOKEN_SLASH) {
        _dplp_next_token(parser);
        DPL_Ast_Node* rhs = _dplp_parser_unary(parser);

        DPL_Ast_Node* new_expression = dpla_create_node(&parser->tree, AST_NODE_BINARY);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_additive(DPL_Parser* parser)
{
    DPL_Ast_Node* expression = _dplp_parse_multiplicative(parser);

    DPL_Token operator_candidate = _dplp_peek_token(parser);
    while (operator_candidate.kind == TOKEN_PLUS || operator_candidate.kind == TOKEN_MINUS) {
        _dplp_next_token(parser);
        DPL_Ast_Node* rhs = _dplp_parse_multiplicative(parser);

        DPL_Ast_Node* new_expression = dpla_create_node(&parser->tree, AST_NODE_BINARY);
        new_expression->as.binary.left = expression;
        new_expression->as.binary.operator = operator_candidate;
        new_expression->as.binary.right = rhs;
        expression = new_expression;

        operator_candidate = _dplp_peek_token(parser);
    }

    return expression;
}

DPL_Ast_Node* _dplp_parse_expression(DPL_Parser* parser)
{
    return _dplp_parse_additive(parser);
}


void dplp_parse(DPL_Parser* parser)
{
    parser->tree.root = _dplp_parse_expression(parser);
    _dplp_expect_token(parser, TOKEN_EOF);
}