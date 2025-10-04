#pragma once
#include "lexer.h"
#include "ast.h"
#include <memory>

namespace cmini {

class Parser {
public:
    explicit Parser(Lexer& lex) : lex(lex) {}

    std::unique_ptr<Program> parseProgram();
    const std::vector<std::string>& errors() const { return errs; }

private:
    // helpers
    const Token& peek();
    Token eat();
    bool accept(TokenKind k);
    void expect(TokenKind k, const char* msg);
    [[noreturn]] void failHere(const std::string& msg);
    void syncToSemicolon();

    // grammar
    std::unique_ptr<Function> function();
    Type typeSpec();
    Type afterTypeModifiers(Type base);
    Param param();
    void parseArraySuffix(Type& t);
    std::unique_ptr<Block> block();
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> ifStmt();
    std::unique_ptr<Stmt> whileStmt();
    std::unique_ptr<Stmt> doWhileStmt();
    std::unique_ptr<Stmt> forStmt();
    std::unique_ptr<Stmt> returnStmt();
    std::unique_ptr<Stmt> declOrExprStmt();

    std::unique_ptr<Expr> expr();
    std::unique_ptr<Expr> assign();
    std::unique_ptr<Expr> logicOr();
    std::unique_ptr<Expr> logicAnd();
    std::unique_ptr<Expr> bitOr();
    std::unique_ptr<Expr> bitXor();
    std::unique_ptr<Expr> bitAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> relational();
    std::unique_ptr<Expr> shift();
    std::unique_ptr<Expr> additive();
    std::unique_ptr<Expr> mul();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> postfix();
    std::unique_ptr<Expr> primary();

    Lexer& lex;
    std::vector<std::string> errs;
};

} // namespace cmini
