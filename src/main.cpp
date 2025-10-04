#include <fstream>
#include <iostream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "irgen.h"

using namespace cmini;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: cmini <file> [ -o out.ll ]\n";
        return 1;
    }
    std::string inPath = argv[1];
    std::string outPath = "out.ll";
    for (int i=2;i<argc;i++) {
        std::string a = argv[i];
        if (a=="-o" && i+1<argc) { outPath = argv[++i]; }
    }

    std::ifstream in(inPath);
    if (!in) { std::cerr << "cannot open: " << inPath << "\n"; return 1; }
    std::ostringstream ss; ss << in.rdbuf();

    Lexer lex(ss.str());
    Parser parser(lex);
    std::unique_ptr<Program> prog;
    try { prog = parser.parseProgram(); }
    catch (const std::exception&) {}

    if (!parser.errors().empty()) {
        for (const auto& e : parser.errors()) std::cerr << e << "\n";
        return 1;
    }

    Semantic sem; sem.analyze(*prog);
    if (!sem.diags.ok()) {
        for (auto& m : sem.diags.messages) std::cerr << "error: " << m << "\n";
        return 1;
    }

    IRGen ir; std::string text = ir.gen(*prog);
    std::ofstream out(outPath);
    out << text;
    std::cout << "wrote " << outPath << "\n";
    return 0;
}
