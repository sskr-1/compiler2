#include "irgen.h"
#include <sstream>

namespace cmini {

std::string IRGen::typeToIR(const Type& t) {
    std::string b;
    switch (t.base) {
        case BaseType::Void: b = "void"; break;
        case BaseType::Int: b = "i32"; break;
        case BaseType::Char: b = "i8"; break;
        case BaseType::Float: b = "float"; break;
    }
    for (int i=0;i<t.pointerLevels;++i) b += "*";
    // arrays are lowered as pointers to first element for now
    if (!t.arrayDims.empty()) b += "*";
    return b;
    return "i32";
}

std::string IRGen::newTmp() { std::ostringstream os; os << "%t" << (++tmpCounter); return os.str(); }

std::string IRGen::gen(Program& p) {
    out.clear(); tmpCounter=0;
    out += "; ModuleID = 'cmini'\nsource_filename = \"cmini\"\n\n";
    for (auto& f : p.functions) gen(*f);
    return out;
}

void IRGen::gen(Function& f) {
    std::ostringstream sig;
    sig << "define " << typeToIR(f.retType) << " @" << f.name << "(";
    for (size_t i=0;i<f.params.size();++i) {
        if (i) sig << ", ";
        sig << typeToIR(f.params[i].type) << " %" << f.params[i].name;
    }
    sig << ") {\n";
    out += sig.str();
    allocaStack.clear(); valueStack.clear(); typeStack.clear();
    allocaStack.emplace_back(); valueStack.emplace_back(); typeStack.emplace_back();
    if (f.body) {
        out += "entry:\n";
        gen(*f.body);
    }
    out += "}\n\n";
}

void IRGen::gen(Block& b) {
    for (auto& s : b.items) gen(*s);
}

void IRGen::gen(Stmt& s) {
    if (auto e = dynamic_cast<ExprStmt*>(&s)) { (void)gen(*e->expr); return; }
    if (auto r = dynamic_cast<ReturnStmt*>(&s)) {
        if (r->expr) { auto v = gen(*r->expr); out += "  ret i32 " + v + "\n"; }
        else out += "  ret void\n";
        return;
    }
    if (auto b = dynamic_cast<Block*>(&s)) { gen(*b); return; }
    // for brevity we do not lower control flow yet; placeholder no-ops
    if (dynamic_cast<BreakStmt*>(&s) || dynamic_cast<ContinueStmt*>(&s)) return;
    if (auto d = dynamic_cast<Decl*>(&s)) {
        // allocate alloca + store init if any
        std::string irTy = typeToIR(d->varType);
        std::string tmp = newTmp();
        out += "  " + tmp + " = alloca " + irTy + "\n";
        if (!allocaStack.empty()) allocaStack.back()[d->name] = tmp;
        out += "  ; map " + d->name + " -> " + tmp + "\n";
        if (!valueStack.empty()) valueStack.back()[d->name] = tmp; // ptr alias
        if (!typeStack.empty()) typeStack.back()[d->name] = d->varType;
        if (d->init) {
            std::string val = gen(*d->init);
            out += "  store " + irTy + " " + val + ", " + irTy + "* " + tmp + "\n";
        }
        return;
    }
    // ignore other statements for minimal MVP
}

std::string IRGen::gen(Expr& e) {
    if (auto lit = dynamic_cast<IntegerLiteral*>(&e)) { return std::to_string(lit->value); }
    if (auto v = dynamic_cast<VarRef*>(&e)) {
        if (auto p = lookupAlloca(v->name)) {
            std::string t=newTmp();
            out += "  ; load from alloca of " + v->name + "\n";
            out += "  " + t + " = load i32, i32* " + *p + "\n";
            return t;
        }
        if (auto q = lookupValue(v->name)) {
            std::string t=newTmp();
            out += "  ; load from value map of " + v->name + "\n";
            out += "  " + t + " = load i32, i32* " + *q + "\n";
            return t;
        }
        // function parameter fallback
        out += "  ; fallback param " + v->name + "\n";
        return "%" + v->name;
    }
    if (auto idx = dynamic_cast<ArrayIndex*>(&e)) {
        // compute address: base + index * elementSize; treat as i32 arrays
        std::string basePtr = genAddress(*idx->base);
        std::string indexVal = gen(*idx->index);
        std::string gep = newTmp();
        out += "  " + gep + " = getelementptr i32, i32* " + basePtr + ", i32 " + indexVal + "\n";
        std::string loadv = newTmp();
        out += "  " + loadv + " = load i32, i32* " + gep + "\n";
        return loadv;
    }
    if (auto b = dynamic_cast<BinaryExpr*>(&e)) {
        std::string l = gen(*b->lhs); std::string r = gen(*b->rhs); std::string t=newTmp();
        const char* op = nullptr;
        switch (b->op) {
            case BinaryOp::Add: op="add"; break; case BinaryOp::Sub: op="sub"; break; case BinaryOp::Mul: op="mul"; break; case BinaryOp::Div: op="sdiv"; break; case BinaryOp::Mod: op="srem"; break;
            default: op="add"; // placeholder
        }
        out += "  " + t + " = " + op + " i32 " + l + ", " + r + "\n"; return t;
    }
    if (auto u = dynamic_cast<UnaryExpr*>(&e)) { return gen(*u->operand); }
    if (auto a = dynamic_cast<AssignExpr*>(&e)) {
        // try to store into a VarRef backed by alloca
        if (auto lv = dynamic_cast<VarRef*>(a->lhs.get())) {
            if (auto p = lookupAlloca(lv->name)) {
                std::string val = gen(*a->rhs);
                out += "  store i32 " + val + ", i32* " + *p + "\n";
                return val;
            }
        }
        if (auto la = dynamic_cast<ArrayIndex*>(a->lhs.get())) {
            std::string addr = genAddress(*la);
            std::string val = gen(*a->rhs);
            out += "  store i32 " + val + ", i32* " + addr + "\n";
            return val;
        }
        std::string rhs = gen(*a->rhs); return rhs;
    }
    return "0";
}

std::string IRGen::genAddress(Expr& e) {
    if (auto v = dynamic_cast<VarRef*>(&e)) {
        if (auto p = lookupAlloca(v->name)) return *p;
        if (auto q = lookupValue(v->name)) return *q;
        return "%" + v->name; // parameter address (already value, not address) – best-effort
    }
    if (auto idx = dynamic_cast<ArrayIndex*>(&e)) {
        std::string base = genAddress(*idx->base);
        std::string index = gen(*idx->index);
        std::string gep = newTmp();
        out += "  " + gep + " = getelementptr i32, i32* " + base + ", i32 " + index + "\n";
        return gep;
    }
    // fallback: compute and spill
    std::string val = gen(e);
    std::string tmp = newTmp();
    out += "  " + tmp + " = alloca i32\n  store i32 " + val + ", i32* " + tmp + "\n";
    return tmp;
}

std::string* IRGen::lookupAlloca(const std::string& name) {
    for (auto it = allocaStack.rbegin(); it != allocaStack.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return &f->second;
    }
    return nullptr;
}

std::string* IRGen::lookupValue(const std::string& name) {
    for (auto it = valueStack.rbegin(); it != valueStack.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return &f->second;
    }
    return nullptr;
}

const Type* IRGen::lookupType(const std::string& name) {
    for (auto it = typeStack.rbegin(); it != typeStack.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return &f->second;
    }
    return nullptr;
}

} // namespace cmini
