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
        return source_[pos_++];
    }
    
    const char      *source_;
    int             pos_;
    std::string     text_;
    double          numVal_;
    int             lastChar_;
};