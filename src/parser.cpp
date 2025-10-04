#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace cmini {

const Token& Parser::peek() { return lex.peek(); }
Token Parser::eat() { return lex.next(); }
bool Parser::accept(TokenKind k) { if (peek().kind==k) { eat(); return true; } return false; }
void Parser::expect(TokenKind k, const char* msg) {
    if (!accept(k)) failHere(std::string("expected ")+msg);
}

[[noreturn]] void Parser::failHere(const std::string& msg) {
    const Token& t = peek();
    std::ostringstream os; os << "parse error at line " << t.line << ", col " << t.col << ": " << msg;
    errs.push_back(os.str());
    throw std::runtime_error(errs.back());
}

void Parser::syncToSemicolon() {
    // consume tokens until ';' or '}' to continue parsing
    while (peek().kind != TokenKind::Semicolon && peek().kind != TokenKind::RBrace && peek().kind != TokenKind::End) eat();
    if (peek().kind == TokenKind::Semicolon) eat();
}

Type Parser::typeSpec() {
    Token t = eat();
    Type ty = Type::intTy();
    switch (t.kind) {
        case TokenKind::KwInt: ty.base = BaseType::Int; break;
        case TokenKind::KwChar: ty.base = BaseType::Char; break;
        case TokenKind::KwFloat: ty.base = BaseType::Float; break;
        case TokenKind::KwVoid: ty.base = BaseType::Void; break;
        case TokenKind::KwEnum: ty.base = BaseType::Int; break; // enums lower to int
        case TokenKind::KwUnion: ty.base = BaseType::Int; break; // unions opaque for MVP
        default: throw std::runtime_error("type expected");
    }
    return afterTypeModifiers(ty);
}

Type Parser::afterTypeModifiers(Type base) {
    // handle multi-level pointers: int **
    while (accept(TokenKind::Star)) base.pointerLevels++;
    // handle arrays: int a[10][20]
    while (accept(TokenKind::LBracket)) {
        Token d = eat();
        if (d.kind != TokenKind::Integer) throw std::runtime_error("array size integer expected");
        base.arrayDims.push_back((size_t)d.intVal);
        expect(TokenKind::RBracket, "]");
    }
    return base;
}

Param Parser::param() {
    Type t = typeSpec();
    Token id = eat();
    if (id.kind != TokenKind::Identifier) throw std::runtime_error("param name expected");
    parseArraySuffix(t);
    return {t, id.text};
}

std::unique_ptr<Block> Parser::block() {
    expect(TokenKind::LBrace, "{");
    auto blk = std::make_unique<Block>();
    while (peek().kind != TokenKind::RBrace && peek().kind != TokenKind::End) {
        blk->items.push_back(statement());
    }
    expect(TokenKind::RBrace, "}");
    return blk;
}

void Parser::parseArraySuffix(Type& t) {
    while (accept(TokenKind::LBracket)) {
        Token d = eat();
        if (d.kind != TokenKind::Integer) throw std::runtime_error("array size integer expected");
        t.arrayDims.push_back((size_t)d.intVal);
        expect(TokenKind::RBracket, "]");
    }
}

std::unique_ptr<Stmt> Parser::declOrExprStmt() {
    // Lookahead for a type keyword
    if (peek().kind==TokenKind::KwInt || peek().kind==TokenKind::KwChar || peek().kind==TokenKind::KwFloat || peek().kind==TokenKind::KwVoid) {
        Type t = typeSpec();
        Token id = eat(); if (id.kind!=TokenKind::Identifier) failHere("identifier expected");
        parseArraySuffix(t);
        auto decl = std::make_unique<Decl>(t, id.text);
        if (accept(TokenKind::Assign)) { auto e = assign(); decl->init = std::move(e); }
        expect(TokenKind::Semicolon, ";");
        return decl;
    }
    auto e = expr();
    // require ';' here; if missing, report and resync
    if (!accept(TokenKind::Semicolon)) { errs.push_back("parse error at line " + std::to_string(peek().line) + ": missing ';' after expression"); syncToSemicolon(); }
    return std::make_unique<ExprStmt>(std::move(e));
}

std::unique_ptr<Stmt> Parser::statement() {
    switch (peek().kind) {
        case TokenKind::LBrace: return block();
        case TokenKind::KwIf: return ifStmt();
        case TokenKind::KwWhile: return whileStmt();
        case TokenKind::KwDo: return doWhileStmt();
        case TokenKind::KwFor: return forStmt();
        case TokenKind::KwReturn: return returnStmt();
        case TokenKind::KwBreak: eat(); expect(TokenKind::Semicolon, ";"); return std::make_unique<BreakStmt>();
        case TokenKind::KwContinue: eat(); expect(TokenKind::Semicolon, ";"); return std::make_unique<ContinueStmt>();
        default: return declOrExprStmt();
    }
}

std::unique_ptr<Stmt> Parser::ifStmt() {
    expect(TokenKind::KwIf, "if");
    expect(TokenKind::LParen, "(");
    auto c = expr();
    expect(TokenKind::RParen, ")");
    auto t = statement();
    std::unique_ptr<Stmt> e;
    if (accept(TokenKind::KwElse)) e = statement();
    auto s = std::make_unique<IfStmt>(); s->cond=std::move(c); s->thenS=std::move(t); s->elseS=std::move(e); return s;
}

std::unique_ptr<Stmt> Parser::whileStmt() {
    expect(TokenKind::KwWhile, "while");
    expect(TokenKind::LParen, "(");
    auto c = expr();
    expect(TokenKind::RParen, ")");
    auto b = statement();
    auto s = std::make_unique<WhileStmt>(); s->cond=std::move(c); s->body=std::move(b); return s;
}

std::unique_ptr<Stmt> Parser::doWhileStmt() {
    expect(TokenKind::KwDo, "do");
    auto b = statement();
    expect(TokenKind::KwWhile, "while");
    expect(TokenKind::LParen, "(");
    auto c = expr();
    expect(TokenKind::RParen, ")");
    expect(TokenKind::Semicolon, ";");
    auto s = std::make_unique<DoWhileStmt>(); s->body=std::move(b); s->cond=std::move(c); return s;
}

std::unique_ptr<Stmt> Parser::forStmt() {
    expect(TokenKind::KwFor, "for");
    expect(TokenKind::LParen, "(");
    std::unique_ptr<Stmt> init;
    if (!accept(TokenKind::Semicolon)) {
        if (peek().kind==TokenKind::KwInt || peek().kind==TokenKind::KwChar || peek().kind==TokenKind::KwFloat || peek().kind==TokenKind::KwVoid) init = declOrExprStmt();
        else { auto e = expr(); expect(TokenKind::Semicolon, ";"); init = std::make_unique<ExprStmt>(std::move(e)); }
    }
    std::unique_ptr<Expr> cond;
    if (!accept(TokenKind::Semicolon)) { cond = expr(); expect(TokenKind::Semicolon, ";"); }
    std::unique_ptr<Expr> step;
    if (!accept(TokenKind::RParen)) { step = expr(); expect(TokenKind::RParen, ")"); }
    auto b = statement();
    auto s = std::make_unique<ForStmt>(); s->init=std::move(init); s->cond=std::move(cond); s->step=std::move(step); s->body=std::move(b); return s;
}

std::unique_ptr<Stmt> Parser::returnStmt() {
    expect(TokenKind::KwReturn, "return");
    std::unique_ptr<Expr> e;
    if (!accept(TokenKind::Semicolon)) { e = expr(); expect(TokenKind::Semicolon, ";"); }
    auto s = std::make_unique<ReturnStmt>(); s->expr = std::move(e); return s;
}

std::unique_ptr<Expr> Parser::expr() { return assign(); }

std::unique_ptr<Expr> Parser::assign() {
    auto lhs = logicOr();
    if (accept(TokenKind::Assign)) {
        auto rhs = assign();
        return std::make_unique<AssignExpr>(std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::logicOr() {
    auto e = logicAnd();
    while (accept(TokenKind::OrOr)) {
        auto r = logicAnd();
        e = std::make_unique<BinaryExpr>(BinaryOp::Or, std::move(e), std::move(r));
    }
    return e;
}

std::unique_ptr<Expr> Parser::logicAnd() {
    auto e = bitOr();
    while (accept(TokenKind::AndAnd)) {
        auto r = bitOr();
        e = std::make_unique<BinaryExpr>(BinaryOp::And, std::move(e), std::move(r));
        }
    return e;
}

std::unique_ptr<Expr> Parser::bitOr() {
    auto e = bitXor();
    while (accept(TokenKind::Pipe)) { auto r = bitXor(); e = std::make_unique<BinaryExpr>(BinaryOp::BitOr, std::move(e), std::move(r)); }
    return e;
}
std::unique_ptr<Expr> Parser::bitXor() {
    auto e = bitAnd();
    while (accept(TokenKind::Caret)) { auto r = bitAnd(); e = std::make_unique<BinaryExpr>(BinaryOp::BitXor, std::move(e), std::move(r)); }
    return e;
}
std::unique_ptr<Expr> Parser::bitAnd() {
    auto e = equality();
    while (accept(TokenKind::Amp)) { auto r = equality(); e = std::make_unique<BinaryExpr>(BinaryOp::BitAnd, std::move(e), std::move(r)); }
    return e;
}
std::unique_ptr<Expr> Parser::equality() {
    auto e = relational();
    while (true) {
        if (accept(TokenKind::EQ)) { auto r = relational(); e = std::make_unique<BinaryExpr>(BinaryOp::EQ, std::move(e), std::move(r)); }
        else if (accept(TokenKind::NE)) { auto r = relational(); e = std::make_unique<BinaryExpr>(BinaryOp::NE, std::move(e), std::move(r)); }
        else break;
    }
    return e;
}
std::unique_ptr<Expr> Parser::relational() {
    auto e = shift();
    while (true) {
        if (accept(TokenKind::LT)) { auto r = shift(); e = std::make_unique<BinaryExpr>(BinaryOp::LT, std::move(e), std::move(r)); }
        else if (accept(TokenKind::LE)) { auto r = shift(); e = std::make_unique<BinaryExpr>(BinaryOp::LE, std::move(e), std::move(r)); }
        else if (accept(TokenKind::GT)) { auto r = shift(); e = std::make_unique<BinaryExpr>(BinaryOp::GT, std::move(e), std::move(r)); }
        else if (accept(TokenKind::GE)) { auto r = shift(); e = std::make_unique<BinaryExpr>(BinaryOp::GE, std::move(e), std::move(r)); }
        else break;
    }
    return e;
}
std::unique_ptr<Expr> Parser::shift() {
    auto e = additive();
    while (true) {
        if (accept(TokenKind::Shl)) { auto r = additive(); e = std::make_unique<BinaryExpr>(BinaryOp::Shl, std::move(e), std::move(r)); }
        else if (accept(TokenKind::Shr)) { auto r = additive(); e = std::make_unique<BinaryExpr>(BinaryOp::Shr, std::move(e), std::move(r)); }
        else break;
    }
    return e;
}
std::unique_ptr<Expr> Parser::additive() {
    auto e = mul();
    while (true) {
        if (accept(TokenKind::Plus)) { auto r = mul(); e = std::make_unique<BinaryExpr>(BinaryOp::Add, std::move(e), std::move(r)); }
        else if (accept(TokenKind::Minus)) { auto r = mul(); e = std::make_unique<BinaryExpr>(BinaryOp::Sub, std::move(e), std::move(r)); }
        else break;
    }
    return e;
}
std::unique_ptr<Expr> Parser::mul() {
    auto e = unary();
    while (true) {
        if (accept(TokenKind::Star)) { auto r = unary(); e = std::make_unique<BinaryExpr>(BinaryOp::Mul, std::move(e), std::move(r)); }
        else if (accept(TokenKind::Slash)) { auto r = unary(); e = std::make_unique<BinaryExpr>(BinaryOp::Div, std::move(e), std::move(r)); }
        else if (accept(TokenKind::Percent)) { auto r = unary(); e = std::make_unique<BinaryExpr>(BinaryOp::Mod, std::move(e), std::move(r)); }
        else break;
    }
    return e;
}
std::unique_ptr<Expr> Parser::unary() {
    if (accept(TokenKind::Plus)) return std::make_unique<UnaryExpr>(UnaryOp::Plus, unary());
    if (accept(TokenKind::Minus)) return std::make_unique<UnaryExpr>(UnaryOp::Minus, unary());
    if (accept(TokenKind::Amp)) return std::make_unique<UnaryExpr>(UnaryOp::Addr, unary());
    if (accept(TokenKind::Star)) return std::make_unique<UnaryExpr>(UnaryOp::Deref, unary());
    return postfix();
}
std::unique_ptr<Expr> Parser::postfix() {
    auto e = primary();
    while (true) {
        if (accept(TokenKind::LBracket)) { auto idx = expr(); expect(TokenKind::RBracket, "]"); e = std::make_unique<ArrayIndex>(std::move(e), std::move(idx)); }
        else break;
    }
    return e;
}
std::unique_ptr<Expr> Parser::primary() {
    Token t = eat();
    switch (t.kind) {
        case TokenKind::Identifier: return std::make_unique<VarRef>(t.text);
        case TokenKind::Integer: return std::make_unique<IntegerLiteral>(t.intVal);
        case TokenKind::Char: return std::make_unique<CharLiteral>((char)t.intVal);
        case TokenKind::String: return std::make_unique<StringLiteral>(t.text);
        case TokenKind::LParen: { auto e = expr(); expect(TokenKind::RParen, ")"); return e; }
        default: throw std::runtime_error("expression expected");
    }
}

std::unique_ptr<Function> Parser::function() {
    Type ret = typeSpec();
    Token id = eat(); if (id.kind!=TokenKind::Identifier) failHere("function name");
    expect(TokenKind::LParen, "(");
    std::vector<Param> params;
    if (peek().kind != TokenKind::RParen) {
        try { params.push_back(param()); while (accept(TokenKind::Comma)) params.push_back(param()); }
        catch (const std::exception&) { syncToSemicolon(); }
    }
    expect(TokenKind::RParen, ")");
    auto fun = std::make_unique<Function>();
    fun->retType = ret; fun->name = id.text; fun->params = std::move(params);
    try { fun->body = block(); }
    catch (const std::exception&) { /* error captured, continue */ }
    return fun;
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto p = std::make_unique<Program>();
    while (peek().kind != TokenKind::End) {
        try { p->functions.push_back(function()); }
        catch (const std::exception&) { syncToSemicolon(); eat(); }
    }
    return p;
}

} // namespace cmini
