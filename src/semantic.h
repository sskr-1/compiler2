#pragma once
#include "ast.h"
#include <memory>
#include <string>

namespace cmini {

struct Diagnostics {
    std::vector<std::string> messages;
    void error(const std::string& m) { messages.push_back(m); }
    bool ok() const { return messages.empty(); }
};

struct Semantic {
    Diagnostics diags;
    Scope global;

    void analyze(Program& p);

private:
    void analyze(Function& f);
    void analyze(Block& b, Scope& scope);
    void analyze(Stmt& s, Scope& scope, const Type& retTy);
    Type analyze(Expr& e, Scope& scope);
};

} // namespace cmini
