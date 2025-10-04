#pragma once
#include <string>
#include <cstddef>

namespace cmini {

enum class TokenKind {
    End,
    Identifier,
    Integer,
    Char,
    String,

    KwInt, KwChar, KwFloat, KwVoid,
    KwEnum, KwUnion,
    KwIf, KwElse, KwFor, KwWhile, KwDo, KwReturn,
    KwBreak, KwContinue,

    Plus, Minus, Star, Slash, Percent,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Semicolon, Comma, Assign,
    LT, GT, LE, GE, EQ, NE,
    AndAnd, OrOr,
    Amp, Pipe, Caret, Tilde,
    Shl, Shr
};

struct Token {
    TokenKind kind {TokenKind::End};
    std::string text;
    long intVal {0};
    int line {1};
    int col {1};
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    Token next();
    const Token& peek();
private:
    Token scan();
    bool isAtEnd() const;
    char current() const;
    char advance();
    bool match(char c);

    const std::string src;
    size_t pos {0};
    Token lookahead;
    bool hasLookahead {false};
    int line {1};
    int col {1};
};

} // namespace cmini
