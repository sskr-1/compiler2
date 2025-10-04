#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>

// A small C-like AST and symbol table

namespace cmini {

enum class BaseType { Void, Int, Char, Float };

enum class NamedKind { None, Enum, Union };

struct Type {
    BaseType base {BaseType::Int};
    int pointerLevels {0};
    std::vector<size_t> arrayDims; // multi-dimensional array sizes outermost-first
    NamedKind namedKind {NamedKind::None};
    std::string namedTag; // enum/union tag name if any

    static Type voidTy();
    static Type intTy();

    std::string toString() const;
};

struct Node {
    virtual ~Node() = default;
};

struct Expr : Node {
    Type type; // inferred during semantic analysis
};

struct Stmt : Node {};

// Expressions
struct IntegerLiteral : Expr {
    long value {0};
    explicit IntegerLiteral(long v) : value(v) {}
};

struct CharLiteral : Expr {
    char value {0};
    explicit CharLiteral(char v) : value(v) {}
};

struct StringLiteral : Expr {
    std::string value;
    explicit StringLiteral(std::string v) : value(std::move(v)) {}
};

struct VarRef : Expr {
    std::string name;
    explicit VarRef(std::string n) : name(std::move(n)) {}
};

struct ArrayIndex : Expr {
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
    ArrayIndex(std::unique_ptr<Expr> b, std::unique_ptr<Expr> i)
        : base(std::move(b)), index(std::move(i)) {}
};

enum class UnaryOp { Plus, Minus, Not, BitNot, PreInc, PreDec, Addr, Deref };
struct UnaryExpr : Expr {
    UnaryOp op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(UnaryOp o, std::unique_ptr<Expr> e)
        : op(o), operand(std::move(e)) {}
};

enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    LT, GT, LE, GE, EQ, NE,
    And, Or, BitAnd, BitOr, BitXor,
    Shl, Shr
};
struct BinaryExpr : Expr {
    BinaryOp op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    BinaryExpr(BinaryOp o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct AssignExpr : Expr {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    AssignExpr(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : lhs(std::move(l)), rhs(std::move(r)) {}
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;
    explicit CallExpr(std::string c) : callee(std::move(c)) {}
};

// Statements
struct Decl : Stmt {
    Type varType;
    std::string name;
    std::unique_ptr<Expr> init; // optional, may be null
    Decl(Type t, std::string n) : varType(t), name(std::move(n)) {}
};

struct ExprStmt : Stmt { std::unique_ptr<Expr> expr; explicit ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {} };

struct ReturnStmt : Stmt { std::unique_ptr<Expr> expr; };

struct BreakStmt : Stmt {};
struct ContinueStmt : Stmt {};

struct Block : Stmt { std::vector<std::unique_ptr<Stmt>> items; };

struct IfStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> thenS;
    std::unique_ptr<Stmt> elseS; // may be null
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;
};

struct DoWhileStmt : Stmt {
    std::unique_ptr<Stmt> body;
    std::unique_ptr<Expr> cond;
};

struct ForStmt : Stmt {
    std::unique_ptr<Stmt> init; // decl or expr or null
    std::unique_ptr<Expr> cond; // may be null
    std::unique_ptr<Expr> step; // may be null
    std::unique_ptr<Stmt> body;
};

struct Param {
    Type type;
    std::string name;
};

struct Function : Node {
    Type retType;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<Block> body; // null for declaration
};

struct Program : Node {
    std::vector<std::unique_ptr<Function>> functions;
};

// Semantic structures
struct Symbol {
    Type type;
    bool isFunction {false};
    std::vector<Type> paramTypes; // for functions
};

struct Scope {
    std::unordered_map<std::string, Symbol> table;
    Scope* parent {nullptr};
    explicit Scope(Scope* p=nullptr): parent(p) {}
    Symbol* lookupLocal(const std::string& n);
    Symbol* lookup(const std::string& n);
    void insert(const std::string& n, const Symbol& s);
};

} // namespace cmini
