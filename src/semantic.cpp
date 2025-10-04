#include "semantic.h"
#include <stdexcept>

namespace cmini {

static bool isIntegerLike(const Type& t) {
    return t.base==BaseType::Int || t.base==BaseType::Char;
}

void Semantic::analyze(Program& p) {
    // predeclare functions
    for (auto& fn : p.functions) {
        Symbol s; s.type = fn->retType; s.isFunction = true; for (auto& prm : fn->params) s.paramTypes.push_back(prm.type);
        global.insert(fn->name, s);
    }
    for (auto& fn : p.functions) analyze(*fn);
}

void Semantic::analyze(Function& f) {
    Scope scope{&global};
    for (auto& prm : f.params) { Symbol s; s.type = prm.type; scope.insert(prm.name, s); }
    if (f.body) analyze(*f.body, scope);
}

void Semantic::analyze(Block& b, Scope& scope) {
    Scope local{&scope};
    for (auto& it : b.items) analyze(*it, local, Type::voidTy());
}

void Semantic::analyze(Stmt& s, Scope& scope, const Type& retTy) {
    if (auto d = dynamic_cast<Decl*>(&s)) {
        if (scope.lookupLocal(d->name)) diags.error("redefinition: "+d->name);
        Symbol sym; sym.type = d->varType; scope.insert(d->name, sym);
        if (d->init) { auto t = analyze(*d->init, scope); (void)t; }
        return;
    }
    if (auto r = dynamic_cast<ReturnStmt*>(&s)) {
        if (r->expr) { auto t = analyze(*r->expr, scope); (void)t; }
        return;
    }
    if (auto e = dynamic_cast<ExprStmt*>(&s)) { analyze(*e->expr, scope); return; }
    if (auto w = dynamic_cast<WhileStmt*>(&s)) { analyze(*w->cond, scope); analyze(*w->body, scope, retTy); return; }
    if (auto d = dynamic_cast<DoWhileStmt*>(&s)) { analyze(*d->body, scope, retTy); analyze(*d->cond, scope); return; }
    if (auto f = dynamic_cast<ForStmt*>(&s)) {
        Scope inner{&scope};
        if (f->init) analyze(*f->init, inner, retTy);
        if (f->cond) analyze(*f->cond, inner);
        if (f->step) analyze(*f->step, inner);
        analyze(*f->body, inner, retTy);
        return;
    }
    if (auto i = dynamic_cast<IfStmt*>(&s)) {
        analyze(*i->cond, scope);
        analyze(*i->thenS, scope, retTy);
        if (i->elseS) analyze(*i->elseS, scope, retTy);
        return;
    }
    if (auto b = dynamic_cast<Block*>(&s)) { analyze(*b, scope); return; }
}

Type Semantic::analyze(Expr& e, Scope& scope) {
    if (auto v = dynamic_cast<VarRef*>(&e)) {
        auto* sym = scope.lookup(v->name);
        if (!sym) { diags.error("use of undeclared identifier: "+v->name); e.type = Type::intTy(); return e.type; }
        e.type = sym->type; return e.type;
    }
    if (auto lit = dynamic_cast<IntegerLiteral*>(&e)) { e.type = Type::intTy(); return e.type; }
    if (auto litc = dynamic_cast<CharLiteral*>(&e)) { e.type = Type{BaseType::Char,false,std::nullopt}; return e.type; }
    if (auto str = dynamic_cast<StringLiteral*>(&e)) { Type t; t.base=BaseType::Char; t.isPointer=true; e.type=t; (void)str; return e.type; }
    if (auto a = dynamic_cast<AssignExpr*>(&e)) { auto lt=analyze(*a->lhs, scope); auto rt=analyze(*a->rhs, scope); (void)lt; (void)rt; e.type=lt; return e.type; }
    if (auto b = dynamic_cast<BinaryExpr*>(&e)) { auto lt=analyze(*b->lhs, scope); auto rt=analyze(*b->rhs, scope); (void)rt; if (isIntegerLike(lt)) e.type=lt; else e.type=rt; return e.type; }
    if (auto u = dynamic_cast<UnaryExpr*>(&e)) { auto t=analyze(*u->operand, scope); if (u->op==UnaryOp::Addr){ Type p=t; p.isPointer=true; e.type=p; } else e.type=t; return e.type; }
    if (auto idx = dynamic_cast<ArrayIndex*>(&e)) { auto bt=analyze(*idx->base, scope); auto it=analyze(*idx->index, scope); (void)it; if (bt.isPointer || bt.arrayLen) { Type elem=bt; elem.isPointer=false; elem.arrayLen.reset(); e.type=elem; } else e.type=bt; return e.type; }
    if (auto call = dynamic_cast<CallExpr*>(&e)) {
        auto* sym = scope.lookup(call->callee);
        if (!sym || !sym->isFunction) { diags.error("call to undeclared function: "+call->callee); e.type=Type::intTy(); return e.type; }
        for (auto& a : call->args) analyze(*a, scope);
        e.type = sym->type; return e.type;
    }
    e.type = Type::intTy(); return e.type;
}

} // namespace cmini
