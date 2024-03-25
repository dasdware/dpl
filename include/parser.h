#ifndef __DPL_PARSER_H
#define __DPL_PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct
{
    DPL_Lexer *lexer;
    DPL_Ast_Tree tree;
} DPL_Parser;

void dplp_init(DPL_Parser *parser, DPL_Lexer *lexer);
void dplp_free(DPL_Parser *parser);

void dplp_parse(DPL_Parser *parser);

#endif // __DPL_PARSER_H