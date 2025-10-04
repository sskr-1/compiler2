#include "ast.h"
#include <sstream>

namespace cmini {

Type Type::voidTy() { Type t; t.base = BaseType::Void; return t; }
Type Type::intTy() { Type t; t.base = BaseType::Int; return t; }

std::string Type::toString() const {
    std::ostringstream os;
    switch (base) {
        case BaseType::Void: os << "void"; break;
        case BaseType::Int: os << "int"; break;
        case BaseType::Char: os << "char"; break;
        case BaseType::Float: os << "float"; break;
    }
    for (int i=0;i<pointerLevels;++i) os << "*";
    for (size_t n : arrayDims) os << "[" << n << "]";
    return os.str();
}

Symbol* Scope::lookupLocal(const std::string& n) {
    auto it = table.find(n);
    if (it == table.end()) return nullptr;
    return &it->second;
}

Symbol* Scope::lookup(const std::string& n) {
    for (Scope* s = this; s != nullptr; s = s->parent) {
        auto it = s->table.find(n);
        if (it != s->table.end()) return &it->second;
    }
    return nullptr;
}

void Scope::insert(const std::string& n, const Symbol& s) {
    table[n] = s;
}

} // namespace cmini
