#include "lexer.h"
#include <cctype>
#include <optional>

namespace cmini {

static bool isIdentStart(char c) { return std::isalpha((unsigned char)c) || c=='_'; }
static bool isIdentCont(char c) { return std::isalnum((unsigned char)c) || c=='_'; }

Lexer::Lexer(const std::string& input) : src(input) {}

const Token& Lexer::peek() {
    if (!hasLookahead) { lookahead = scan(); hasLookahead = true; }
    return lookahead;
}

Token Lexer::next() {
    if (hasLookahead) { hasLookahead = false; return lookahead; }
    return scan();
}

bool Lexer::isAtEnd() const { return pos >= src.size(); }
char Lexer::current() const { return isAtEnd() ? '\0' : src[pos]; }
char Lexer::advance() { return isAtEnd() ? '\0' : src[pos++]; }
bool Lexer::match(char c) { if (current()==c) { ++pos; return true; } return false; }

Token Lexer::scan() {
    // skip whitespace and comments
    while (!isAtEnd()) {
        char c = current();
        if (std::isspace((unsigned char)c)) { ++pos; continue; }
        if (c=='/' && pos+1 < src.size() && src[pos+1]=='/') {
            pos+=2; while (!isAtEnd() && current()!='\n') ++pos; continue;
        }
        if (c=='/' && pos+1 < src.size() && src[pos+1]=='*') {
            pos+=2; while (!isAtEnd() && !(current()=='*' && pos+1<src.size() && src[pos+1]=='/')) ++pos; if (pos+1<src.size()) pos+=2; continue;
        }
        break;
    }
    if (isAtEnd()) return {TokenKind::End, ""};

    char c = advance();

    // identifiers and keywords
    if (isIdentStart(c)) {
        std::string s(1,c);
        while (isIdentCont(current())) s.push_back(advance());
        if (s=="int") return {TokenKind::KwInt, s};
        if (s=="char") return {TokenKind::KwChar, s};
        if (s=="float") return {TokenKind::KwFloat, s};
        if (s=="void") return {TokenKind::KwVoid, s};
        if (s=="enum") return {TokenKind::KwEnum, s};
        if (s=="union") return {TokenKind::KwUnion, s};
        if (s=="if") return {TokenKind::KwIf, s};
        if (s=="else") return {TokenKind::KwElse, s};
        if (s=="for") return {TokenKind::KwFor, s};
        if (s=="while") return {TokenKind::KwWhile, s};
        if (s=="do") return {TokenKind::KwDo, s};
        if (s=="return") return {TokenKind::KwReturn, s};
        if (s=="break") return {TokenKind::KwBreak, s};
        if (s=="continue") return {TokenKind::KwContinue, s};
        return {TokenKind::Identifier, s};
    }

    // numbers (decimal only for brevity)
    if (std::isdigit((unsigned char)c)) {
        long v = c - '0';
        while (std::isdigit((unsigned char)current())) v = v*10 + (advance()-'0');
        return {TokenKind::Integer, "", v};
    }

    // strings and chars
    if (c=='"') {
        std::string s;
        while (!isAtEnd() && current()!='"') {
            char ch = advance();
            if (ch=='\\' && !isAtEnd()) {
                char n = advance();
                switch(n){case 'n': s+='\n'; break; case 't': s+='\t'; break; default: s+=n;}
            } else s.push_back(ch);
        }
        if (current()=='"') ++pos;
        Token t; t.kind=TokenKind::String; t.text=std::move(s); return t;
    }
    if (c=='\'') {
        char v = advance();
        if (v=='\\') { char n = advance(); v = n=='n'?'\n':n; }
        if (current()=='\'') ++pos;
        Token t; t.kind=TokenKind::Char; t.intVal=v; return t;
    }

    // punctuation and operators
    auto two = [&](char a,char b, TokenKind k)->std::optional<Token> {
        if (c==a && current()==b) { ++pos; return Token{k, std::string({a,b})}; }
        return std::nullopt;
    };
    if (auto t=two('&','&',TokenKind::AndAnd)) return *t;
    if (auto t=two('|','|',TokenKind::OrOr)) return *t;
    if (auto t=two('=','=',TokenKind::EQ)) return *t;
    if (auto t=two('!','=',TokenKind::NE)) return *t;
    if (auto t=two('<','=',TokenKind::LE)) return *t;
    if (auto t=two('>','=',TokenKind::GE)) return *t;
    if (auto t=two('<','<',TokenKind::Shl)) return *t;
    if (auto t=two('>','>',TokenKind::Shr)) return *t;

    switch (c) {
        case '+': return {TokenKind::Plus, "+"};
        case '-': return {TokenKind::Minus, "-"};
        case '*': return {TokenKind::Star, "*"};
        case '/': return {TokenKind::Slash, "/"};
        case '%': return {TokenKind::Percent, "%"};
        case '&': return {TokenKind::Amp, "&"};
        case '|': return {TokenKind::Pipe, "|"};
        case '^': return {TokenKind::Caret, "^"};
        case '~': return {TokenKind::Tilde, "~"};
        case '(': return {TokenKind::LParen, "("};
        case ')': return {TokenKind::RParen, ")"};
        case '{': return {TokenKind::LBrace, "{"};
        case '}': return {TokenKind::RBrace, "}"};
        case '[': return {TokenKind::LBracket, "["};
        case ']': return {TokenKind::RBracket, "]"};
        case ';': return {TokenKind::Semicolon, ";"};
        case ',': return {TokenKind::Comma, ","};
        case '=': return {TokenKind::Assign, "="};
        case '<': return {TokenKind::LT, "<"};
        case '>': return {TokenKind::GT, ">"};
        default: return {TokenKind::End, ""};
    }
}

} // namespace cmini
