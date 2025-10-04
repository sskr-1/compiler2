%{
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern int yyparse();
extern int yylineno;

// External declarations for lexer
extern int yylex(void* yylval);

void yyerror(const char *s) {
    std::cerr << "Parser error at line " << yylineno << ": " << s << std::endl;
}

/* ---------------- STR_LIST ---------------- */
struct str_list {
    std::vector<std::string> arr;
};
%}

/* ---------------- Semantic values ---------------- */
%union {
    std::string *str;
    int ival;
    double fval;
    str_list *strlist;
}

%{

/* ---------------- SYMBOL TABLE ---------------- */
struct Symbol {
    std::string name;
    std::string type;
};

std::vector<Symbol> symtab;

/* ---------------- TYPEDEF TABLE ---------------- */
struct TypedefEntry {
    std::string name;
    std::string type;
};

std::vector<TypedefEntry> typedef_table;

/* current declaration type to apply to init_declarator list */

/* ---------------- HELPERS ---------------- */
std::string* my_strdup(const char* s) {
    if (!s) return nullptr;
    return new std::string(s);
}

std::string* my_strdup(const std::string& s) {
    return new std::string(s);
}

std::string* my_strdup(std::string* s) {
    if (!s) return nullptr;
    return s;  // Just return the pointer as-is
}

void free_strlist(str_list* list) {
    delete list;
}

str_list* create_strlist() {
    return new str_list();
}

/* Symbol table helpers */
void add_symbol(const char* name, const char* type) {
    if (!name || !*name) return;
    std::string t = (type && *type) ? type : "int";
    
    // Check for duplicates before adding
    for (const auto& sym : symtab) {
        if (sym.name == name && sym.type == t) {
            return; // Skip if exact match already exists
        }
    }
    symtab.push_back({std::string(name), t});
}

void add_typedef(const char* name, const char* type) {
    if (!name || !*name) return;
    typedef_table.push_back({std::string(name), std::string(type)});
}

const char* resolve_typedef(const char* name) {
    for (int i = typedef_table.size()-1; i >= 0; i--) {
        if (typedef_table[i].name == name) return typedef_table[i].type.c_str();
    }
    return nullptr;
}

// Helper function to extract the actual identifier name from a declarator
std::string extract_identifier_name(const std::string& declarator) {
    std::string name = declarator;
    
    // Remove pointer symbols from the beginning
    while (!name.empty() && name[0] == '*') {
        name = name.substr(1);
    }
    
    // Remove array brackets and function parameters
    size_t pos = name.find('[');
    if (pos != std::string::npos) {
        name = name.substr(0, pos);
    }
    
    pos = name.find('(');
    if (pos != std::string::npos) {
        name = name.substr(0, pos);
    }
    
    return name;
}

// Helper function to determine the complete type including pointer/array info
std::string determine_complete_type(const std::string& base_type, const std::string& declarator) {
    std::string complete_type = base_type;
    
    // Count pointer symbols at the beginning
    int pointer_count = 0;
    size_t i = 0;
    while (i < declarator.length() && declarator[i] == '*') {
        pointer_count++;
        i++;
    }
    
    // Prepend pointer symbols to the base type
    for (int j = 0; j < pointer_count; j++) {
        complete_type = "*" + complete_type;
    }
    
    // Add ALL array information if present (handle multi-dimensional arrays)
    size_t pos = 0;
    while (pos < declarator.length()) {
        size_t bracket_pos = declarator.find('[', pos);
        if (bracket_pos == std::string::npos) {
            break;  // No more array brackets
        }
        
        size_t end_bracket = declarator.find(']', bracket_pos);
        if (end_bracket == std::string::npos) {
            break;  // Unmatched bracket
        }
        
        // Extract the complete array dimension including brackets
        std::string array_part = declarator.substr(bracket_pos, end_bracket - bracket_pos + 1);
        complete_type += array_part;
        
        // Move position to after this bracket pair
        pos = end_bracket + 1;
    }
    
    // Add function parameter information if present
    size_t paren_pos = declarator.find('(');
    if (paren_pos != std::string::npos) {
        size_t end_paren = declarator.find(')', paren_pos);
        if (end_paren != std::string::npos) {
            std::string func_part = declarator.substr(paren_pos, end_paren - paren_pos + 1);
            complete_type += func_part;
        }
    }
    
    return complete_type;
}

void prepend_pointer(std::string& type_buf, const char* ptr) {
    if (!ptr || !*ptr) return;
    if (type_buf.substr(0, 8) == "typedef ")
        type_buf = type_buf + ptr;
    else
        type_buf = std::string(ptr) + type_buf;
}

%}




/* ---------------- Tokens ---------------- */
%token KW_AUTO KW_BREAK KW_CASE KW_CHAR KW_CONTINUE KW_DEFAULT KW_DO KW_DOUBLE
%token KW_ELSE KW_FLOAT KW_FOR KW_GOTO KW_IF KW_INT KW_LONG KW_RETURN KW_SHORT
%token KW_SIGNED KW_SIZEOF KW_STATIC KW_STRUCT KW_SWITCH KW_TYPEDEF KW_UNSIGNED
%token KW_VOID KW_WHILE KW_MALLOC KW_FREE KW_PRINTF KW_SCANF
%token FUNC_DEF

%token OP_ASSIGN OP_PLUSEQ OP_MINUSEQ OP_MULTEQ OP_DIVEQ OP_MODEQ OP_ANDEQ OP_OREQ OP_XOREQ OP_SHLEQ OP_SHREQ
%token OP_NOT OP_GT OP_LT OP_PLUS OP_MINUS OP_MUL OP_DIV OP_MOD
%token OP_ARROW
%token OP_EQ OP_NE OP_GE OP_LE OP_AND OP_OR OP_INC OP_DEC
%token OP_SHL OP_SHR
%token OP_AND_BIT OP_OR_BIT OP_XOR OP_NEG OP_QUESTION OP_COLON

%token <ival> CONST_INT
%token <fval> CONST_FLOAT
%token <str> CONST_CHAR STRING_LITERAL IDENTIFIER

/* ---------------- Typed nonterminals ---------------- */
%type <str> declaration_specifiers type_specifier storage_class_specifier
%type <str> declarator direct_declarator init_declarator pointer typedef_alias
%type <str> constant_expression parameter_type_list parameter_list parameter_declaration
%type <strlist> init_declarator_list
%type struct_member_list struct_member
%type <str> abstract_declarator direct_abstract_declarator 
%type <str> conditional_expression logical_or_expression logical_and_expression
%type <str> inclusive_or_expression exclusive_or_expression and_expression
%type <str> equality_expression relational_expression shift_expression
%type <str> additive_expression multiplicative_expression cast_expression
%type <str> unary_expression postfix_expression primary_expression
%type <str> argument_expression_list assignment_expression
%type <str> expression type_name  /* <- added type_name and expression */
%type <str> typedef_name

/* ---------------- Precedence ---------------- */
%left ',' 
%right OP_ASSIGN OP_PLUSEQ OP_MINUSEQ OP_MULTEQ OP_DIVEQ OP_MODEQ OP_ANDEQ OP_OREQ OP_XOREQ OP_SHLEQ OP_SHREQ
%left OP_QUESTION OP_COLON
%left OP_OR
%left OP_AND
%left OP_OR_BIT
%left OP_XOR
%left OP_AND_BIT
%left OP_EQ OP_NE
%left OP_LT OP_GT OP_LE OP_GE
%left OP_SHL OP_SHR
%left OP_PLUS OP_MINUS
%left OP_MUL OP_DIV OP_MOD
%right OP_NOT OP_NEG OP_INC OP_DEC KW_SIZEOF
%left '.' OP_ARROW '[' 
%left '('
%right ')'
%precedence FUNC_DEF

%define parse.error verbose
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start program

%%

program
    : external_declaration_list
    ;

external_declaration_list
    : external_declaration_list external_declaration
    | /* empty */
    ;

external_declaration
    : function_definition
    | function_declaration
    | typedef_definition
    | struct_typedef_definition
    | declaration
    ;

struct_typedef_definition
    : KW_TYPEDEF KW_STRUCT IDENTIFIER '{' struct_member_list '}' IDENTIFIER ';' {
        std::string struct_type = "struct " + std::string(*$3);
        add_typedef($7->c_str(), struct_type.c_str());  // Store complete struct type
        delete $3; delete $7;
    }
    | KW_TYPEDEF KW_STRUCT '{' struct_member_list '}' IDENTIFIER ';' {
        // For anonymous structs, store as "struct"
        add_typedef($6->c_str(), "struct");
        delete $6;
    }
    | KW_TYPEDEF KW_STRUCT IDENTIFIER IDENTIFIER ';' {
        std::string struct_type = "struct " + std::string(*$3);
        add_typedef($4->c_str(), struct_type.c_str());
        delete $3; delete $4;
    }
;

typedef_definition
    : KW_TYPEDEF declaration_specifiers declarator ';' {
        // Get the base type from declaration specifiers
        std::string base_type = *$2;
        
        // Get the declarator (which may contain pointer info)
        std::string declarator_str = *$3;
        
        // Extract the actual identifier name (without pointer/array info)
        std::string actual_name = extract_identifier_name(declarator_str);
        
        // Determine the complete type including pointer/array info
        std::string complete_type = determine_complete_type(base_type, declarator_str);
        
        // Add the typedef with the actual name and complete type
        add_typedef(actual_name.c_str(), complete_type.c_str());
        
        delete $2; delete $3;
    }
    | KW_TYPEDEF declaration_specifiers init_declarator_list ';' {
        // Handle multiple typedefs in one statement (without pointer info)
        std::string base_type = *$2;
        
        // For each declarator, add to typedef table
        for (const auto& declarator_str : $3->arr) {
            // Extract the actual identifier name and determine the complete type
            std::string actual_name = extract_identifier_name(declarator_str);
            std::string complete_type = determine_complete_type(base_type, declarator_str);
            
            add_typedef(actual_name.c_str(), complete_type.c_str());
        }
        
        free_strlist($3);
        delete $2;
    }
    | KW_TYPEDEF declaration_specifiers ';' {
        // Handle typedefs without declarators (like struct typedefs)
        delete $2;
    }
;



function_definition
    : declaration_specifiers IDENTIFIER '(' parameter_type_list ')' compound_statement %prec FUNC_DEF {
        // Handle function definitions with parameters explicitly
        std::string func_name = *$2;
        // Add all functions as type "function"
        add_symbol(func_name.c_str(), "function");
        delete $1; delete $2; delete $4;
    }
    | declaration_specifiers IDENTIFIER '(' ')' compound_statement %prec FUNC_DEF {
        // Handle function definitions with no parameters explicitly to avoid conflicts
        std::string func_name = *$2;
        // Add all functions as type "function"
        add_symbol(func_name.c_str(), "function");
        delete $1; delete $2;
    }
    | declaration_specifiers declarator compound_statement %prec FUNC_DEF {
        // Handle complex function definitions (with pointers, arrays, etc.)
        std::string declarator_str = *$2;
        std::string actual_name = extract_identifier_name(declarator_str);
        // Add all functions as type "function"
        add_symbol(actual_name.c_str(), "function");
        delete $1; delete $2;
    }
    ;

function_declaration
    : declaration_specifiers IDENTIFIER '(' parameter_type_list ')' ';' %prec FUNC_DEF {
        // Handle function declarations with parameters explicitly
        std::string func_name = *$2;
        // Add all functions as type "function"
        add_symbol(func_name.c_str(), "function");
        delete $1; delete $2; delete $4;
    }
    | declaration_specifiers IDENTIFIER '(' ')' ';' %prec FUNC_DEF {
        // Handle function declarations with no parameters explicitly to avoid conflicts
        std::string func_name = *$2;
        // Add all functions as type "function"
        add_symbol(func_name.c_str(), "function");
        delete $1; delete $2;
    }
    | declaration_specifiers declarator ';' %prec FUNC_DEF {
        // Handle complex function declarations (with pointers, arrays, etc.)
        std::string declarator_str = *$2;
        std::string actual_name = extract_identifier_name(declarator_str);
        // Add all functions as type "function"
        add_symbol(actual_name.c_str(), "function");
        delete $1; delete $2;
    }
    ;


declaration
    : declaration_specifiers init_declarator_list ';' {
        std::string decl_type = *$1;
        str_list* list = $2;
        for (const auto& declarator_str : list->arr) {
            // Extract the actual identifier name and determine the complete type
            std::string actual_name = extract_identifier_name(declarator_str);
            std::string complete_type = determine_complete_type(decl_type, declarator_str);
            
            const char* resolved_type = resolve_typedef(complete_type.c_str());
            add_symbol(actual_name.c_str(), resolved_type ? resolved_type : complete_type.c_str());
        }
        free_strlist(list);
        delete $1;
      }
    | declaration_specifiers ';' {
        delete $1;
      }
;


/* ---------------- declaration_specifiers ---------------- */
declaration_specifiers
    : declaration_specifiers type_specifier {
          $$ = my_strdup(*$1 + " " + *$2);
          delete $1; delete $2;
      }
    | declaration_specifiers storage_class_specifier {
          $$ = my_strdup(*$1 + " " + *$2);
          delete $1; delete $2;
      }
    | type_specifier { $$ = $1; }
    | storage_class_specifier { $$ = $1; }
    | typedef_name { $$ = $1; } 
    ;

typedef_name
    : IDENTIFIER {
        // Check if this identifier is a typedef
        const char* resolved = resolve_typedef($1->c_str());
        if (resolved) {
            $$ = my_strdup(resolved);
        } else {
            // Not a typedef, treat as regular identifier
            $$ = $1;
        }
    }
    ;

storage_class_specifier
    : KW_STATIC { $$ = my_strdup("static"); }
    | KW_TYPEDEF { $$ = my_strdup("typedef"); }
    ;

typedef_alias
    : IDENTIFIER { $$ = my_strdup($1); }
    ;

type_specifier
    : KW_INT        { $$ = my_strdup("int"); }
    | KW_CHAR       { $$ = my_strdup("char"); }
    | KW_FLOAT      { $$ = my_strdup("float"); }
    | KW_DOUBLE     { $$ = my_strdup("double"); }
    | KW_VOID       { $$ = my_strdup("void"); }
    | KW_SIGNED type_specifier   { $$ = my_strdup("signed " + *$2); delete $2; }
    | KW_UNSIGNED type_specifier { $$ = my_strdup("unsigned " + *$2); delete $2; }
    | KW_SHORT type_specifier    { $$ = my_strdup("short " + *$2); delete $2; }
    | KW_LONG type_specifier     { $$ = my_strdup("long " + *$2); delete $2; }

    /* Struct types - FIXED */
    | KW_STRUCT IDENTIFIER '{' struct_member_list '}' {
          $$ = my_strdup("struct " + std::string(*$2)); 
          delete $2;
      }
    | KW_STRUCT IDENTIFIER {
          $$ = my_strdup("struct " + std::string(*$2)); 
          delete $2;
      }
    | KW_STRUCT '{' struct_member_list '}' { 
          $$ = my_strdup("struct"); 
      }
    | KW_STRUCT { $$ = my_strdup("struct"); }
    ;
/* ---------------- struct_member_list ---------------- */
struct_member_list
    : struct_member
    | struct_member_list struct_member
    ;

struct_member
    : declaration_specifiers init_declarator_list ';' {
        // Store the base type for struct members
        std::string member_type = *$1;
        
        str_list* list = $2;
        for (const auto& declarator_str : list->arr) {
            // Extract the actual identifier name and determine the complete type
            std::string actual_name = extract_identifier_name(declarator_str);
            std::string complete_type = determine_complete_type(member_type, declarator_str);
            
            add_symbol(actual_name.c_str(), complete_type.c_str());
        }
        free_strlist(list);
        delete $1;
      }
;



/* ---------------- init_declarator_list ---------------- */
init_declarator_list
    : init_declarator {
        $$ = new str_list;
        $$->arr.push_back(*$1);
        delete $1;
      }
    | init_declarator_list ',' init_declarator {
        $1->arr.push_back(*$3);
        delete $3;
        $$ = $1;
      }
    ;

// ---------------- init_declarator ----------------
init_declarator
    : declarator {
        // REMOVE the add_symbol calls from here
        // They're already handled in the declaration rule
        $$ = my_strdup($1);
    }
    | declarator OP_ASSIGN initializer {
        // REMOVE the add_symbol calls from here too
        $$ = my_strdup($1);
    }
    ;


// ---------------- declarator / pointer / direct_declarator ----------------
declarator
    : pointer direct_declarator {
          // Handle pointer types correctly - combine pointer with declarator
          std::string pointer_part = *$1;
          std::string declarator_part = *$2;
          
          // Combine pointer and declarator
          $$ = my_strdup(pointer_part + declarator_part);
          
          delete $1; delete $2;
      }
    | direct_declarator { $$ = $1; }
    ;

pointer
    : '*' { $$ = my_strdup("*"); }
    | '*' pointer {
        $$ = my_strdup("*" + std::string(*$2));
        delete $2;
    }
    ;

direct_declarator
    : IDENTIFIER { $$ = $1; }
    | '(' declarator ')' { $$ = $2; }
    | direct_declarator '[' ']' {
        $$ = my_strdup(*$1 + "[]");
        delete $1;
    }
    | direct_declarator '[' constant_expression ']' {
        $$ = my_strdup(*$1 + "[" + *$3 + "]");
        delete $1; delete $3;
    }
    | direct_declarator '(' parameter_type_list ')' {
        $$ = my_strdup(*$1 + "(" + *$3 + ")");
        delete $1; delete $3;
    }
    | direct_declarator '(' ')' {
    $$ = my_strdup(*$1 + "()");
    delete $1;
     }
    | direct_declarator '(' KW_VOID ')' {   /* <-- allow void as "no params" */
        $$ = my_strdup(*$1 + "(void)");
        delete $1;
    }
    ;

// Remove the conflicting rule that caused shift/reduce conflicts
// | direct_declarator '(' ')' { $$ = $1; }

// ---------------- parameters / type_name ----------------
parameter_type_list
    : parameter_list
    ;

parameter_list
    : parameter_declaration { $$ = $1; }
    | parameter_list ',' parameter_declaration {
        // Concatenate multiple parameters with commas
        $$ = my_strdup(*$1 + "," + *$3);
        delete $1; delete $3;
    }
    ;

parameter_declaration
    : declaration_specifiers declarator {
        // Extract the parameter name from the declarator
        std::string param_name = extract_identifier_name(*$2);
        // Always add parameter as "int param" to symbol table
        add_symbol(param_name.c_str(), "int param");
        // Keep original type information for parser
        $$ = my_strdup(*$1 + " " + *$2);
        delete $1; delete $2;
    }
    | declaration_specifiers {
        // No identifier given (like "int")
        // Add an anonymous param to symbol table
        add_symbol("<anon>", "int param");
        $$ = $1;
    }
    ;

type_name
    : specifier_qualifier_list abstract_declarator
    | specifier_qualifier_list
    ;

abstract_declarator
    : pointer
    | pointer abstract_declarator
    | direct_abstract_declarator
    ;

direct_abstract_declarator
    : '(' abstract_declarator ')' { $$ = $2; }
    | '[' CONST_INT ']' { 
          $$ = my_strdup("[" + std::to_string($2) + "]"); 
      }
    | '[' ']' { $$ = my_strdup("[]"); }
    | '(' parameter_type_list ')' { 
          $$ = my_strdup("(" + *$2 + ")"); 
          delete $2;
      }
    ;




specifier_qualifier_list
    : type_specifier
    ;

// ---------------- initializer ----------------
initializer
    : assignment_expression
    | '{' initializer_list '}'
    | '{' initializer_list ',' '}'
    ;

initializer_list
    : initializer
    | initializer_list ',' initializer
    ;

// ---------------- compound statement ----------------
compound_statement
    : '{' block_item_list '}'
    ;

block_item_list
    : /* empty */
    | block_item_list block_item
    ;

block_item
    : declaration
    | statement
    ;

// ---------------- statements ----------------
statement
    : labeled_statement
    | compound_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    | error ';' { yyerror("Syntax error in statement"); yyerrok; }
    ;

labeled_statement
    : IDENTIFIER OP_COLON statement {
        // Add the label to the symbol table
        add_symbol($1->c_str(), "label");
        delete $1;
    }
    | KW_CASE constant_expression OP_COLON statement
    | KW_DEFAULT OP_COLON statement
    ;

expression_statement
    : expression ';'
    | ';'
    ;

selection_statement
    : KW_IF '(' expression ')' statement
    | KW_IF '(' expression ')' statement KW_ELSE statement
    | KW_SWITCH '(' expression ')' statement
    ;

iteration_statement
    : KW_WHILE '(' expression ')' statement
    | KW_DO statement KW_WHILE '(' expression ')' ';'
    | KW_FOR '(' expression_statement expression_statement ')' statement
    | KW_FOR '(' expression_statement expression_statement expression ')' statement
    | KW_FOR '(' declaration expression_statement ')' statement
    | KW_FOR '(' declaration expression_statement expression ')' statement
    ;

jump_statement
    : KW_GOTO IDENTIFIER ';' {
        // Add reference to the goto label in symbol table
        add_symbol($2->c_str(), "label");
        delete $2;
    }
    | KW_CONTINUE ';'
    | KW_BREAK ';'
    | KW_RETURN expression ';'
    | KW_RETURN ';'
    ;

// ---------------- expressions ----------------
expression
    : assignment_expression
    | expression ',' assignment_expression
    ;

assignment_expression
    : conditional_expression
    | unary_expression assignment_operator assignment_expression
    ;

assignment_operator
    : OP_ASSIGN | OP_PLUSEQ | OP_MINUSEQ | OP_MULTEQ | OP_DIVEQ
    | OP_MODEQ | OP_ANDEQ | OP_OREQ | OP_XOREQ | OP_SHLEQ | OP_SHREQ
    ;

conditional_expression
    : logical_or_expression { $$ = $1; }
    | logical_or_expression OP_QUESTION expression OP_COLON conditional_expression { 
          $$ = my_strdup(*$1 + "?" + *$3 + ":" + *$5); 
          delete $1; delete $3; delete $5; 
      }
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OP_OR logical_and_expression
    ;

logical_and_expression
    : inclusive_or_expression
    | logical_and_expression OP_AND inclusive_or_expression
    ;

inclusive_or_expression
    : exclusive_or_expression
    | inclusive_or_expression OP_OR_BIT exclusive_or_expression
    ;

exclusive_or_expression
    : and_expression
    | exclusive_or_expression OP_XOR and_expression
    ;

and_expression
    : equality_expression
    | and_expression OP_AND_BIT equality_expression
    ;

equality_expression
    : relational_expression
    | equality_expression OP_EQ relational_expression
    | equality_expression OP_NE relational_expression
    ;

relational_expression
    : shift_expression
    | relational_expression OP_LT shift_expression
    | relational_expression OP_GT shift_expression
    | relational_expression OP_LE shift_expression
    | relational_expression OP_GE shift_expression
    ;

shift_expression
    : additive_expression
    | shift_expression OP_SHL additive_expression
    | shift_expression OP_SHR additive_expression
    ;

additive_expression
    : multiplicative_expression
    | additive_expression OP_PLUS multiplicative_expression
    | additive_expression OP_MINUS multiplicative_expression
    ;

multiplicative_expression
    : cast_expression
    | multiplicative_expression '*' cast_expression
    | multiplicative_expression OP_DIV cast_expression
    | multiplicative_expression OP_MOD cast_expression
    ;

cast_expression
    : unary_expression { $$ = $1; }
    | '(' type_name ')' cast_expression { 
          $$ = my_strdup("(" + *$2 + ")" + *$4); 
          delete $2; delete $4; 
      }



unary_expression
    : postfix_expression { $$ = $1; }
    | OP_INC unary_expression { $$ = my_strdup("++" + *$2); delete $2; }
    | OP_DEC unary_expression { $$ = my_strdup("--" + *$2); delete $2; }
    | OP_AND_BIT unary_expression { $$ = my_strdup("&" + *$2); delete $2; }
    | '*' unary_expression { $$ = my_strdup("*" + *$2); delete $2; }
    | OP_PLUS unary_expression { $$ = my_strdup("+" + *$2); delete $2; }
    | OP_MINUS unary_expression { $$ = my_strdup("-" + *$2); delete $2; }
    | OP_NEG unary_expression { $$ = my_strdup("~" + *$2); delete $2; }
    | OP_NOT unary_expression { $$ = my_strdup("!" + *$2); delete $2; }
    | KW_SIZEOF unary_expression { $$ = my_strdup("sizeof " + *$2); delete $2; }
    | KW_SIZEOF '(' type_name ')' { $$ = my_strdup("sizeof(" + *$3 + ")"); delete $3; }
    ;


postfix_expression
    : primary_expression { $$ = $1; }

    // array indexing
    | postfix_expression '[' expression ']' {
        $$ = my_strdup(*$1 + "[" + *$3 + "]");
        delete $1; delete $3;
    }

    // ----------- generic function calls -------------
    | IDENTIFIER '(' ')' {
        $$ = my_strdup(*$1 + "()");
        add_symbol($1->c_str(), "function");
        delete $1;
    }
    | IDENTIFIER '(' argument_expression_list ')' {
        $$ = my_strdup(*$1 + "(" + *$3 + ")");
        add_symbol($1->c_str(), "function");
        delete $1; delete $3;
    }

    // ----------- struct/ptr ops ---------------------
    | postfix_expression '.' IDENTIFIER {
        $$ = my_strdup(*$1 + "." + std::string(*$3));
        delete $1; delete $3;
    }
    | postfix_expression OP_ARROW IDENTIFIER {
        $$ = my_strdup(*$1 + "->" + std::string(*$3));
        delete $1; delete $3;
    }

    // ++ and --
    | postfix_expression OP_INC {
        $$ = my_strdup(*$1 + "++");
        delete $1;
    }
    | postfix_expression OP_DEC {
        $$ = my_strdup(*$1 + "--");
        delete $1;
    }

    // ----------- built-in keywords ------------------
    | KW_MALLOC '(' ')' {
        $$ = my_strdup("malloc()");
    }
    | KW_MALLOC '(' argument_expression_list ')' {
        $$ = my_strdup("malloc(" + *$3 + ")");
        delete $3;
    }
    | KW_FREE '(' ')' {
        $$ = my_strdup("free()");
    }
    | KW_FREE '(' argument_expression_list ')' {
        $$ = my_strdup("free(" + *$3 + ")");
        delete $3;
    }
    | KW_PRINTF '(' ')' {
        add_symbol("printf", "function");
        $$ = my_strdup("printf()");
    }
    | KW_PRINTF '(' argument_expression_list ')' {
        add_symbol("printf", "function");
        $$ = my_strdup("printf(" + *$3 + ")");
        delete $3;
    }
    | KW_SCANF '(' ')' {
        add_symbol("scanf", "function");
        $$ = my_strdup("scanf()");
    }
    | KW_SCANF '(' argument_expression_list ')' {
        add_symbol("scanf", "function");
        $$ = my_strdup("scanf(" + *$3 + ")");
        delete $3;
    }
    ;



primary_expression
    : IDENTIFIER { $$ = $1; }
    | CONST_INT { $$ = my_strdup(std::to_string($1)); }
    | CONST_FLOAT { $$ = my_strdup(std::to_string($1)); }
    | CONST_CHAR { $$ = $1; }
    | STRING_LITERAL { $$ = $1; }
    | '(' expression ')' { $$ = $2; }
    ;

argument_expression_list
    : assignment_expression { $$ = $1; }
    | argument_expression_list ',' assignment_expression {
        $$ = my_strdup(*$1 + "," + *$3);
        delete $1; delete $3;
    }
     
    ;

// ---------------- constant_expression ----------------
constant_expression
    : conditional_expression { $$ = $1; }


%%

// Implementation of the missing error method for the parser class
namespace yy {
    void parser::error(const std::string& msg) {
        std::cerr << "Parser error: " << msg << std::endl;
    }
}

int main() {
    yy::parser parser;
    if (parser.parse() == 0) {
        std::cout << "\n===== SYMBOL TABLE =====" << std::endl;
        for (const auto& symbol : symtab)
            std::cout << symbol.name << " -> " << symbol.type << std::endl;

        std::cout << "\n===== TYPEDEF TABLE =====" << std::endl;
        for (const auto& entry : typedef_table)
            std::cout << entry.name << " -> " << entry.type << std::endl;
    }
    return 0;
}
