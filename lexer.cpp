#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include "lexer.hpp"
using namespace std;

#define KEY_ELEM(str, token) {str, Keyword::token}
typedef unordered_map<string, Keyword> KeyMAP;
KeyMAP keyword_map{
    KEY_ELEM("void", Void), KEY_ELEM("const", Const), KEY_ELEM("int", Int), KEY_ELEM("if", If),
    KEY_ELEM("else", Else), KEY_ELEM("while", While), KEY_ELEM("break", Break), KEY_ELEM("continue", Continue),
    KEY_ELEM("return", Return) 
};
KeyMAP::iterator key_it_;


#define OP_ELEM(token, str) {str, Operator::token} 
typedef unordered_map<string, Operator> OpMAP;
OpMAP operator_map{
    OP_ELEM(Add, "+"), OP_ELEM(Sub, "-"), OP_ELEM(Mul, "*"), OP_ELEM(Div, "/"), OP_ELEM(Mod, "%"), 
    OP_ELEM(Addr, "&"), OP_ELEM(Inc, "++"),OP_ELEM(Dec, "--"), 
    OP_ELEM(Eq, "=="), OP_ELEM(NotEq, "!="), OP_ELEM(Less, "<"), OP_ELEM(More, ">"), OP_ELEM(LessEq, "<="), OP_ELEM(MoreEq, ">="),
    OP_ELEM(Not, "!"), OP_ELEM(And, "&&"), OP_ELEM(Or, "||"),
    OP_ELEM(Assign, "="), OP_ELEM(AddAssign, "+="), OP_ELEM(SubAssign, "-="), OP_ELEM(MulAssign, "*="),OP_ELEM(DivAssign, "/="), OP_ELEM(ModAssgin, "%=")

};
OpMAP::iterator op_it_;
unordered_set<string> op_prefix_{
    "+", "-", "*", "/", "%", "&", "=", "!", "<", ">", "|"
};
unordered_set<string>::iterator op_pre_it_;

/*
#define OP_PRI(op, pr) {Operator::op, pr}
unordered_map<Operator, int> op_priority{
    OP_PRI(Assign, 0), OP_PRI(Or, 1), OP_PRI(And, 2), OP_PRI(Eq, 3), OP_PRI(NotEq, 3), OP_PRI(Less, 4),
    OP_PRI(More, 4), OP_PRI(LessEq, 4), OP_PRI(MoreEq, 4), OP_PRI(Add, 5), OP_PRI(Sub, 5),
    OP_PRI(Mul, 6), OP_PRI(Div, 6), OP_PRI(Mod, 6)        
};
*/

#define PAIR_ELEM(token, str) {str, Pair::token}
typedef unordered_map<string, Pair> PairMAP;
PairMAP pair_map{
    PAIR_ELEM(LParen, "("), PAIR_ELEM(RParen, ")"), PAIR_ELEM(LBracket, "["),
    PAIR_ELEM(RBracket, "]"), PAIR_ELEM(LBrace, "{"), PAIR_ELEM(RBrace, "}")
};
PairMAP::iterator pair_it_;

Token Lexer::Next_Token(){
    while(!in_.eof() && isspace(last_char_)) NextChar();

    if(in_.eof()) return Token::End;

    if(last_char_ == '_' || isalpha(last_char_)) return HandleID();

    if(isdigit(last_char_)) return HandleNumber();

    if(last_char_ == '"') return HandleString();

    if(last_char_ == '\'') return HandleChar();

    string tp;
    tp += last_char_;
    //match one of the prefix
    // /* // confict, remenber to handle :)
    if((op_pre_it_ = op_prefix_.find(tp)) != op_prefix_.end()) return HandleOperator();
    
    tp.clear();
    tp += last_char_;
    if((pair_it_ = pair_map.find(tp)) != pair_map.end()){
        pair_val_ = pair_it_->second;
        NextChar();
        return Token::Pair;
    }

    if(last_char_ == ';'){
       NextChar();
       return Token::Semic; 
    } 
    if(last_char_ == ','){
        NextChar();
        return Token::Comma;
    }

    if(in_.eof()) return Token::End;

    other_val_ = last_char_;
    NextChar();
    return Token::Other;
}

Token Lexer::HandleComment(string prefix_){
    //the third elems
    //one line comment
    if(prefix_[1] == '/'){
        NextChar(); //eat the second /
        while(!in_.eof() && last_char_ != '\n') NextChar();
        NextChar(); //eat '\n'
        //++line_; 4.20
        return Next_Token();
    }
    else if(prefix_[1] == '*'){
        //eat the first *
        char first, second;
        NextChar();
        first = last_char_;
        while(!in_.eof()){
            NextChar();
            second = last_char_;
            if(first == '*' && second == '/') break;
            first = second;
        }
        //while (!in_.eof() && last_char_ != '*') NextChar();
        //while(!in_.eof() && last_char_ == '*') NextChar();
        NextChar(); //skip the /
        return Next_Token();
    }
    //assert(false);
    return Token::End;
}




Token Lexer::HandleID(){
    string ident;
    while(last_char_ == '_' || isalnum(last_char_)){
        ident += last_char_;
        if(in_.eof()) break;
        NextChar();
    }
    // if it is a keyword ? 
    if((key_it_ = keyword_map.find(ident)) != keyword_map.end()){
        key_val_ = key_it_->second;
        return Token::Keyword;
    }
    // ident
    id_val_ = move(ident);
    return Token::Ident;
}

Token Lexer::HandleNumber(){
    string number;
    char* end_pos;
    if(last_char_ == '0'){
        NextChar();
        if(last_char_ == 'x' || last_char_ == 'X'){
            NextChar();
            do{
                number += last_char_;
                NextChar();
            }while(!in_.eof() && isxdigit(last_char_) );
            int_val_ = strtol(number.c_str(), &end_pos, 16);
        }
        else if(isdigit(last_char_)){
            do{
                number += last_char_;
                NextChar();
            }while(!in_.eof() && isdigit(last_char_));
            int_val_ = strtol(number.c_str(), &end_pos, 8);
        }
        else{
            //just zero
            int_val_ = 0;
        }
    }
    else{
        do{
            number += last_char_;
            NextChar();
        }while(!in_.eof() && isdigit(last_char_));
        int_val_ = strtol(number.c_str(), &end_pos, 10);
    }
    return Token::ConstInt;
}

char SpecialChar(char second){
    char ret;
    switch(second){
        case 'a':
            ret = '\a';break;
        case 'b':
            ret = '\b';break;
        case 'f':
            ret = '\f';break;
        case 'n':
            ret = '\n';break;
        case 't':
            ret = '\t';break;
        case 'v':
            ret = '\v';break;
        case 'r':
            ret = '\r';break;
        case '\\':
            ret = '\\';break;
        case '\'':
            ret = '\'';break;
        case '"':
            ret = '\"';break;
        case '0':
            ret = '\0';break;
        default:
            printf("unmatched char.\n");
    }
    return ret;  
}

Token Lexer::HandleString(){
    string str;
    NextChar();
    while(last_char_ != '"'){
        if(last_char_ == '\\'){
            NextChar();
            str += SpecialChar(last_char_);
        }
        else str += last_char_;
        if(!in_.eof()) NextChar();
    }
    str_val_ = move(str);
    return Token::String;
}

Token Lexer::HandleChar(){
    char ch;
    if(!in_.eof()) NextChar();
    if(last_char_ != '\''){
        if(last_char_ == '/'){
            NextChar();
            ch = SpecialChar(last_char_);
        }
        else ch = last_char_;
    }
    char_val_ = move(ch);
    return Token::Char;
}

Token Lexer::HandleOperator(){
    string op;
    op += last_char_;
    NextChar();
    op += last_char_; //add the second char

    if((op_it_ = operator_map.find(op)) != operator_map.end()){
        op_val_ = operator_map[op];
        NextChar();;
        return Token::Opeator;
    }
    else{
        //single op or comment
        // / and // /* conficts
        if(op[0] == '/' && (op[1] == '/' || op[1] == '*')) return HandleComment(op);
        op_val_ = operator_map[op.substr(0, op.length() - 1)];
        //NextChar(); already eat the second one
        return Token::Opeator;
    }
}