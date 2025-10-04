%option noyywrap
%option c++
%{
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <FlexLexer.h>

// Forward declaration
struct str_list { std::vector<std::string> arr; };

// Include the generated header (from parser.y)
#include "parser.tab.hh"

int yylineno = 1;

// A single global lexer instance and a helper to feed it streams
static yyFlexLexer g_lexer;
extern "C" void lexer_switch_streams(std::istream* in) {
    g_lexer.switch_streams(in, nullptr);
}

// Global yylex function for the C++ bison parser
int yylex(void* yylval) {
    int token = g_lexer.yylex();
    yylineno = g_lexer.lineno();

    if (yylval) {
        yy::parser::semantic_type* semantic_val = static_cast<yy::parser::semantic_type*>(yylval);
        switch (token) {
            case yy::parser::token::IDENTIFIER:
                semantic_val->str = new std::string(g_lexer.YYText());
                break;
            case yy::parser::token::CONST_INT:
                semantic_val->ival = atoi(g_lexer.YYText());
                break;
            case yy::parser::token::CONST_FLOAT:
                semantic_val->fval = atof(g_lexer.YYText());
                break;
            case yy::parser::token::CONST_CHAR:
            case yy::parser::token::STRING_LITERAL:
                semantic_val->str = new std::string(g_lexer.YYText());
                break;
        }
    }
    return token;
}
%}

DIGIT    [0-9]
LETTER   [a-zA-Z_]
HEX      [0-9a-fA-F]
OCT      [0-7]
ESC      \\[nrt0\\'"]

/* C-style comment pattern (flex-friendly) and single-line // comments handled below */

%%

"/*"([^*]|\*+[^*/])*\*+"/"    { /* skip comment */ 
                                  /* count newlines inside comment */
                                  const char *s = yytext;
                                  for (; *s; ++s) if (*s == '\n') ++yylineno;
                                }

"//".*                       { /* skip line comment */ ++yylineno; }

"#"[ \t]*"include"[ \t]*[<\"][^>\n\"]*[>\"][ \t]*\n    { ++yylineno; /* Skip #include */ }
"#"[ \t]*[a-zA-Z_][a-zA-Z0-9_]*.*\n                      { ++yylineno; /* Skip other preprocessor directives */ }

\n                          { ++yylineno; }
[ \t\r]+                    { /* skip whitespace */ }

"auto"          { return yy::parser::token::KW_AUTO; }
"break"         { return yy::parser::token::KW_BREAK; }
"case"          { return yy::parser::token::KW_CASE; }
"char"          { return yy::parser::token::KW_CHAR; }
"continue"      { return yy::parser::token::KW_CONTINUE; }
"default"       { return yy::parser::token::KW_DEFAULT; }
"do"            { return yy::parser::token::KW_DO; }
"double"        { return yy::parser::token::KW_DOUBLE; }
"else"          { return yy::parser::token::KW_ELSE; }
"float"         { return yy::parser::token::KW_FLOAT; }
"for"           { return yy::parser::token::KW_FOR; }
"goto"          { return yy::parser::token::KW_GOTO; }
"if"            { return yy::parser::token::KW_IF; }
"int"           { return yy::parser::token::KW_INT; }
"long"          { return yy::parser::token::KW_LONG; }
"return"        { return yy::parser::token::KW_RETURN; }
"short"         { return yy::parser::token::KW_SHORT; }
"signed"        { return yy::parser::token::KW_SIGNED; }
"sizeof"        { return yy::parser::token::KW_SIZEOF; }
"static"        { return yy::parser::token::KW_STATIC; }
"struct"        { return yy::parser::token::KW_STRUCT; }
"switch"        { return yy::parser::token::KW_SWITCH; }
"typedef"       { return yy::parser::token::KW_TYPEDEF; }
"unsigned"      { return yy::parser::token::KW_UNSIGNED; }
"void"          { return yy::parser::token::KW_VOID; }
"while"         { return yy::parser::token::KW_WHILE; }
"malloc"        { return yy::parser::token::KW_MALLOC; }
"free"          { return yy::parser::token::KW_FREE; }
"printf"        { return yy::parser::token::KW_PRINTF; }
"scanf"         { return yy::parser::token::KW_SCANF; }

{LETTER}({LETTER}|{DIGIT})*   { return yy::parser::token::IDENTIFIER; }

0[xX]{HEX}+                   { return yy::parser::token::CONST_INT; }
0{OCT}*                       { return yy::parser::token::CONST_INT; }
{DIGIT}+                      { return yy::parser::token::CONST_INT; }

{DIGIT}+\.{DIGIT}*([eE][+-]?{DIGIT}+)? { return yy::parser::token::CONST_FLOAT; }
\.{DIGIT}+([eE][+-]?{DIGIT}+)?         { return yy::parser::token::CONST_FLOAT; }
{DIGIT}+[eE][+-]?{DIGIT}+              { return yy::parser::token::CONST_FLOAT; }

'([^'\\]|{ESC})'              { return yy::parser::token::CONST_CHAR; }
\"([^"\\]|{ESC})*\"           { return yy::parser::token::STRING_LITERAL; }

"=="    { return yy::parser::token::OP_EQ; }
"!="    { return yy::parser::token::OP_NE; }
"<="    { return yy::parser::token::OP_LE; }
">="    { return yy::parser::token::OP_GE; }
"&&"    { return yy::parser::token::OP_AND; }
"||"    { return yy::parser::token::OP_OR; }
"++"    { return yy::parser::token::OP_INC; }
"--"    { return yy::parser::token::OP_DEC; }
"->"    { return yy::parser::token::OP_ARROW; }
"+="    { return yy::parser::token::OP_PLUSEQ; }
"-="    { return yy::parser::token::OP_MINUSEQ; }
"*="    { return yy::parser::token::OP_MULTEQ; }
"/="    { return yy::parser::token::OP_DIVEQ; }
"%="    { return yy::parser::token::OP_MODEQ; }
"&="    { return yy::parser::token::OP_ANDEQ; }
"|="    { return yy::parser::token::OP_OREQ; }
"^="    { return yy::parser::token::OP_XOREQ; }
"<<="   { return yy::parser::token::OP_SHLEQ; }
">>="   { return yy::parser::token::OP_SHREQ; }
"<<"    { return yy::parser::token::OP_SHL; }
">>"    { return yy::parser::token::OP_SHR; }

"("     { return '('; }
")"     { return ')'; }
"{"     { return '{'; }
"}"     { return '}'; }
"["     { return '['; }
"]"     { return ']'; }
","     { return ','; }
";"     { return ';'; }
"."     { return '.'; }

"="     { return yy::parser::token::OP_ASSIGN; }
"+"     { return yy::parser::token::OP_PLUS; }
"-"     { return yy::parser::token::OP_MINUS; }
"*"     { return '*'; }
"/"     { return yy::parser::token::OP_DIV; }
"%"     { return yy::parser::token::OP_MOD; }
"<"     { return yy::parser::token::OP_LT; }
">"     { return yy::parser::token::OP_GT; }
"&"     { return yy::parser::token::OP_AND_BIT; }
"|"     { return yy::parser::token::OP_OR_BIT; }
"^"     { return yy::parser::token::OP_XOR; }
"!"     { return yy::parser::token::OP_NOT; }
"~"     { return yy::parser::token::OP_NEG; }
"?"     { return yy::parser::token::OP_QUESTION; }
":"     { return yy::parser::token::OP_COLON; }

.   { 
    fprintf(stderr, "LEXER ERROR: unexpected character '%s' at line %d\n", yytext, yylineno);
    return -1;
}

%%
