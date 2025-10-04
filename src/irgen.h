#pragma once
#include "ast.h"
#include <string>
#include <unordered_map>

namespace cmini {

// Minimal textual LLVM IR generator for a subset sufficient to test flow
struct IRGen {
    std::string out;
    int tmpCounter {0};
    std::vector<std::unordered_map<std::string,std::string>> allocaStack; // name -> alloca ptr
    std::vector<std::unordered_map<std::string,std::string>> valueStack;  // name -> last SSA value
    std::vector<std::unordered_map<std::string,Type>> typeStack;           // name -> declared type

    std::string gen(Program& p);

private:
    void gen(Function& f);
    std::string gen(Expr& e);
    std::string genAddress(Expr& e); // for lvalues
    void gen(Stmt& s);
    void gen(Block& b);

    std::string typeToIR(const Type& t);
    std::string newTmp();
    std::string* lookupAlloca(const std::string& name);
    std::string* lookupValue(const std::string& name);
    const Type* lookupType(const std::string& name);
};

} // namespace cmini
