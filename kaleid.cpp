#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

enum Token {
    T_EOF       = -1,
    T_DEF       = -2,
    T_EXTERN    = -3,
    T_IDENT     = -4,
    T_NUMBER    = -5
};

class Lexer {
public:
    Lexer(const char *source) {
        source_ = source;
        pos_ = 0;
        lastChar_ = ' ';
    }
    
    int tok() {
        while (isspace(lastChar_))
            lastChar_ = next();
        
        if (isalpha(lastChar_)) {
            text_ = lastChar_;
            while (isalnum((lastChar_ = next())))
                text_ += lastChar_;
                
            if (text_ == "def") return T_DEF;
            if (text_ == "extern") return T_EXTERN;
            return T_IDENT;
        }
        
        if (isdigit(lastChar_) || lastChar_ == '.') {
            std::string numStr;
            do {
                numStr += lastChar_;
                lastChar_ = next();
            } while (isdigit(lastChar_) || lastChar_ == '.');
            
            numVal_ = strtod(numStr.c_str(), 0);
            return T_NUMBER;
        }
        
        if (lastChar_ == '#') {
            do {
                lastChar_ = getchar();
            } while (lastChar_ != 0 && lastChar_ != '\n' && lastChar_ != '\r');
            
            if (lastChar_ != EOF)
                return tok();
        }
        
        if (lastChar_ == 0)
            return T_EOF;
            
        int thisChar = lastChar_;
        lastChar_ = next();
        
        return thisChar; 
    }
    
    std::string tokenText() { return text_; }
    double tokenNumVal() { return numVal_; }
    
private:
    
    int next() {
        return getchar();
//        return source_[pos_++];
    }
    
    const char      *source_;
    int             pos_;
    std::string     text_;
    double          numVal_;
    int             lastChar_;
};

//
// AST

class ExprAST {
public:
    virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {
    double val_;
public:
    NumberExprAST(double val) : val_(val) {}
};

class VariableExprAST : public ExprAST {
    std::string name_;
public:
    VariableExprAST(const std::string &name) : name_(name) {}
};

class BinaryExprAST : public ExprAST {
    char op_;
    ExprAST *lhs_, *rhs_;
public:
    BinaryExprAST(char op, ExprAST *l, ExprAST *r)
        : op_(op), lhs_(l), rhs_(r) {}
};

class CallExprAST : public ExprAST {
    std::string callee_;
    std::vector<ExprAST*> args_;
public:
    CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
        : callee_(callee), args_(args) {}
};

class PrototypeAST {
    std::string name_;
    std::vector<std::string> args_;
public:
    PrototypeAST(const std::string &name, const std::vector<std::string> &args)
        : name_(name), args_(args) {}
};

class FunctionAST {
    PrototypeAST *proto_;
    ExprAST *body_;
public:
    FunctionAST(PrototypeAST *proto, ExprAST *body)
        : proto_(proto), body_(body) {}
};


//
// Parser

class Parser {
public:
    Parser(Lexer *lexer) : lexer_(lexer) {
        binopPrecedence_['<'] = 10;
        binopPrecedence_['+'] = 20;
        binopPrecedence_['-'] = 20;
        binopPrecedence_['*'] = 40;
    }
    
    FunctionAST *parseDefinition() {
        next();
        PrototypeAST *proto = parsePrototype();
        if (proto == 0) return 0;
        
        if (ExprAST *e = parseExpression())
            return new FunctionAST(proto, e);
            
        return 0;
    }
    
    PrototypeAST *parseExtern() {
        next();
        return parsePrototype();
    }
    
    FunctionAST *parseTopLevelExpr() {
        if (ExprAST *e = parseExpression()) {
            PrototypeAST *proto = new PrototypeAST("", std::vector<std::string>());
            return new FunctionAST(proto, e);
        }
        return 0;
    }
    
private:
    
    ExprAST *parseNumberExpr() {
        ExprAST *result = new NumberExprAST(lexer_->tokenNumVal());
        next();
        return result;
    }
    
    ExprAST *parseParenExpr() {
        next();
        ExprAST *exp = parseExpression();
        if (!exp) return 0;
        
        if (tok_ != ')')
            return error("expected: ')'");
        
        next();
        return exp;
    }
    
    ExprAST *parseIdentifierExpr() {
        std::string name = lexer_->tokenText();
        
        next();
        
        if (tok_ != '(')
            return new VariableExprAST(name);
            
        next();
        std::vector<ExprAST*> args;
        if (tok_ != ')') {
            while (1) {
                ExprAST *arg = parseExpression();
                if (!arg) return 0;
                args.push_back(arg);
                
                if (tok_ == ')') break;
                
                if (tok_ != ',')
                    return error("expected ')' or ',' in argument list");
                    
                next();
            }
        }
        
        next();
        
        return new CallExprAST(name, args);
    }
    
    ExprAST *parsePrimary() {
        switch (tok_) {
            case T_IDENT:
                return parseIdentifierExpr();
            case T_NUMBER:
                return parseNumberExpr();
            case '(':
                return parseParenExpr();
            default:
                return error("unknown token when expecting an expression");
        }
    }
    
    ExprAST *parseExpression() {
        ExprAST *lhs = parsePrimary();
        if (!lhs) return 0;
        
        return parseBinOpRHS(0, lhs);
    }
    
    ExprAST *parseBinOpRHS(int minPrec, ExprAST *lhs) {
        while (1) {
            int prec = precedence();
            if (prec < minPrec)
                return lhs;
                
            int op = tok_;
            next();
            
            ExprAST *rhs = parsePrimary();
            if (!rhs) return 0;
            
            int nextPrec = precedence();
            if (prec < nextPrec) {
                rhs = parseBinOpRHS(prec + 1, rhs);
                if (rhs == 0) return 0;
            }
            
            lhs = new BinaryExprAST(op, lhs, rhs);
        }
    }
    
    PrototypeAST *parsePrototype() {
        if (tok_ != T_IDENT)
            return errorP("expected function name in prototype");
        
        std::string fnName = lexer_->tokenText();
        next();
        
        if (tok_ != '(')
            return errorP("expected '(' in prototype");
            
        std::vector<std::string> params;
        while (next() == T_IDENT)
            params.push_back(lexer_->tokenText());
        
        if (tok_ != ')')
            return errorP("expected ')' in prototype");
            
        next();
        
        return new PrototypeAST(fnName, params);
    }
    
    int next() { return tok_ = lexer_->tok(); }
    
    int precedence() {
        if (!isascii(tok_)) {
            return -1;
        }
        
        int prec = binopPrecedence_[tok_];
        if (prec <= 0) return -1;
        return prec;
    }
    
    ExprAST *error(const char *str) { fprintf(stderr, "error: %s\n", str); return 0; }
    PrototypeAST *errorP(const char *str) { error(str); return 0; }
    FunctionAST *errorF(const char *str) { error(str); return 0; }
    
private:
    Lexer *lexer_;
    int tok_;
    std::map<char,int> binopPrecedence_;
    
};

static Lexer *lexer;
static Parser *parser;

static void handleDefinition() {
    if (parser->parseDefinition()) {
        fprintf(stderr, "parsed a function definition\n");
    } else {
        lexer->tok();
    }
}

static void handleExtern() {
    if (parser->parseExtern()) {
        fprintf(stderr, "parsed an extern\n");
    } else {
        lexer->tok();
    }
}

static void handleTopLevelExpression() {
    if (parser->parseTopLevelExpr()) {
        fprintf(stderr, "parsed a top-level expr\n");
    } else {
        lexer->tok();
    }
}

static void mainLoop() {
    while (1) {
        fprintf(stderr, "ready> ");
        switch (lexer->tok()) {
            case T_EOF:         return;
            case ';':           lexer->tok(); break;
            case T_DEF:         handleDefinition(); break;
            case T_EXTERN:      handleExtern(); break;
            default:            handleTopLevelExpression(); break;
        }
    }
}


int main(int argc, char *argv[]) {
    lexer = new Lexer("");
    parser = new Parser(lexer);
    
    fprintf(stderr, "ready> ");
    lexer->tok();
    
    mainLoop();
    return 0;
}