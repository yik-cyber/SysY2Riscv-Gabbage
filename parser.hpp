#ifndef SYSY_PARSER_H_
#define SYSY_PARSER_H_

#include <string_view>
#include <functional>
#include <initializer_list>
#include <cstddef>

#include "lexer.hpp"
#include "ast.hpp"
#include "token.hpp"

class Parser{
    public:
    Parser(Lexer& _lexer):lexer_(_lexer){
        NextToken(); 
    }

    ASTPtr ParserNext(){
        return cur_token_ == Token::End ? nullptr : ParseFuncVar();
    }

    private:
    
    //next token
    void NextToken(){
        cur_token_ = lexer_.Next_Token();
    }

    //token type
    bool IsTokenKey(Keyword _key){
        return cur_token_ == Token::Keyword && lexer_.KeyVal() == _key;
    }
    bool IsTokenOp(Operator _op){
        return cur_token_ == Token::Opeator && lexer_.OpVal() == _op;
    }
    bool IsTokenChar(char _c){
        return cur_token_ == Token::Char && lexer_.CharVal() == _c;
    }
    bool IsTokenIdent(){
        return cur_token_ == Token::Ident;
    }
    bool IsTokenNumber(){
        return cur_token_ == Token::ConstInt;
    }
    bool IsTokenPair(Pair _pair){
        return cur_token_ == Token::Pair && lexer_.PairVal() == _pair;
    }
    bool IsTokenSemic(){
        return cur_token_ == Token::Semic;
    }
    bool IsTokenComma(){
        return cur_token_ == Token::Comma;
    }


    //parser
    ASTPtr ParseType();
    ASTPtr ParseFuncVar(); //determine to parse which one
    ASTPtr ParseVarDecl();
    ASTPtr ParseVarDef(bool exist_id, string& _ident);
    ASTPtr ParseInitVal();
    ASTPtr ParseFunDef();
    ASTPtrList ParseFuncParas();
    ASTPtr ParseParaDecl();

    ASTPtr ParseBlock();
    ASTPtr ParseStmt();
    //ASTPtr ParseAssign(ASTPtr lval_);
    ASTPtr ParseIfElse();
    ASTPtr ParseWhile();
    //continue and break handled in the If and While
    ASTPtr ParseControl();
    ASTPtr ParseExpr();
    ASTPtr ParseUnaryExpr();
    ASTPtr ParseLVal(string _ident);
    ASTPtr ParseFuncCall(string _ident);



    Lexer& lexer_;
    Token cur_token_;
};
#endif