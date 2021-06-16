#include <stack>
#include <cassert>
#include "ast.hpp"
#include "token.hpp"
#include "parser.hpp"
using namespace std;

#define OP_PRI(op, pr) {Operator::op, pr}
unordered_map<Operator, int> op_priority{
    OP_PRI(Assign, 0), OP_PRI(Or, 1), OP_PRI(And, 2), OP_PRI(Eq, 3), OP_PRI(NotEq, 3), OP_PRI(Less, 4),
    OP_PRI(More, 4), OP_PRI(LessEq, 4), OP_PRI(MoreEq, 4), OP_PRI(Add, 5), OP_PRI(Sub, 5),
    OP_PRI(Mul, 6), OP_PRI(Div, 6), OP_PRI(Mod, 6)        
};

ASTPtr Parser::ParseType(){
    if(IsTokenKey(Keyword::Const)){
        NextToken();
        assert(IsTokenKey(Keyword::Int));
        NextToken();
        return make_unique<ConstIntTypeAST>(Type::ConstInt);
    }
    else if(IsTokenKey(Keyword::Int)){
        NextToken();
        return make_unique<IntTypeAST>(Type::Int);
    }
    else if(IsTokenKey(Keyword::Void)){
        NextToken();
        return make_unique<VoidTypeAST>(Type::Void);
    }
    assert(false);
    return nullptr;
}

/**
 * 可选择的判断情况要返回nullptr
 * 必然情况就不用了，反正都会识别出来，除非代码有错（然而并没有进行完备的检查
 * 
*/
//parse functions and variables. return nullptr when not matched
ASTPtr Parser::ParseFuncVar(){
    if(IsTokenKey(Keyword::Const)){
        auto type = ParseType();

        ASTPtrList defs;

        while(!IsTokenSemic()){
            auto ident = lexer_.IdVal();
            NextToken(); //eat ident
            auto def = ParseVarDef(true, ident); //also eat ','
            defs.push_back(move(def));
        }

        NextToken(); //eat '';'
        return make_unique<VarDeclAST>(move(type), move(defs));    
    }
    else if(IsTokenKey(Keyword::Int) || IsTokenKey(Keyword::Void)){
        auto type = ParseType();
        
        if(IsTokenSemic()){
            NextToken();
            return nullptr; // int;
        }

        assert(IsTokenIdent());
        auto ident = lexer_.IdVal();

        NextToken();
        if(IsTokenPair(Pair::LParen)){ //function
            auto paras = ParseFuncParas(); //ParsePara eats the '()'
            if(IsTokenPair(Pair::LBrace)){
                auto block = ParseBlock(); //ParseBlock eats the '{}'
                return make_unique<FuncDefAST>(ident, move(paras), move(block), move(type));
            }
            else if(IsTokenSemic()){ //just a difinition
                NextToken(); //eat ';'
                return make_unique<FuncDeclAST>(ident, move(paras), move(type));
            }
        }
        else if(IsTokenOp(Operator::Assign) ||IsTokenComma() || IsTokenPair(Pair::LBracket) || IsTokenSemic()){
            //ident 后面 , ; [] 
            ASTPtrList defs;
            auto def = ParseVarDef(true, ident); //also eat ','
            defs.push_back(move(def));

            while(!IsTokenSemic()){
                assert(IsTokenIdent()); //must be ident
                auto ident = lexer_.IdVal();
                NextToken(); //eat ident
                auto def = ParseVarDef(true, ident);
                //the last one
                defs.push_back(move(def));
            }

            NextToken(); //eat ';'
            return make_unique<VarDeclAST>(move(type), move(defs));
        }
        else{
            //可以报错了吧可以报错了吧
            assert(false);
        }
    }
    return nullptr ; // not a declaration of vars or function
}

//调用的时候一定回返回一个def，否则代码有错误
ASTPtr Parser::ParseVarDef(bool exist_id, string& _ident){
    //The first one
    string ident;
    if(!exist_id){
        assert(IsTokenIdent()); //我直接进行一个assert
        ident = lexer_.IdVal();
        NextToken();
    }
    else{
        ident = _ident;
    }
    //array
    ASTPtrList dims;  
    while(IsTokenPair(Pair::LBracket)){
        NextToken(); //eat [
        auto expr = ParseExpr();
        assert(expr);
        dims.push_back(move(expr));
        assert(IsTokenPair(Pair::RBracket));
        NextToken();//eat ']'
    }
    // initval
    if(IsTokenOp(Operator::Assign)){
        NextToken(); //eat =
        auto init_val = ParseInitVal();
        if(IsTokenComma()) NextToken(); //eat ,
        return make_unique<VarDefAST>(_ident, move(dims), move(init_val));
    }
    else{
        if(IsTokenComma()) NextToken(); //eat ,
        return make_unique<VarDefAST>(_ident, move(dims), nullptr);
    }
}


ASTPtr Parser::ParseInitVal(){
    // 没有考虑括号不匹配的情况orz
    if(IsTokenPair(Pair::LBrace)){
        ASTPtrList initvals;
        NextToken(); //eat {
        while(!IsTokenPair(Pair::RBrace)){
            auto initval = ParseInitVal();
            initvals.push_back(move(initval));
            if(IsTokenComma()) NextToken(); //eat ,
        }
        NextToken(); //eat }
        return make_unique<InitValAST>(move(initvals));
    }
    else{
        return ParseExpr();
    }
}

//eat ()
ASTPtrList Parser::ParseFuncParas(){
    assert(IsTokenPair(Pair::LParen));
    NextToken(); //eat (
    
    ASTPtrList paras;
    while(!IsTokenPair(Pair::RParen)){
        auto para = ParseParaDecl();
        paras.push_back(move(para));
        if(IsTokenComma()) NextToken();
    }
    NextToken(); //eat )

    return move(paras);
}

ASTPtr Parser::ParseParaDecl(){
    assert(IsTokenKey(Keyword::Int));
    auto type = ParseType();
    //The first one
    assert(IsTokenIdent());
    auto ident = lexer_.IdVal();
    NextToken(); //eat ident
    //array
    ASTPtrList dims;
    //eat '[]', but it shuold be marked
    if(IsTokenPair(Pair::LBracket)){
        NextToken();
        assert(IsTokenPair(Pair::RBracket));
        NextToken();
        dims.push_back(nullptr);
    }  
    while(IsTokenPair(Pair::LBracket)){
        NextToken(); //eat [
        auto expr = ParseExpr();
        assert(expr);
        dims.push_back(move(expr));
        assert(IsTokenPair(Pair::RBracket));
        
        NextToken();//eat ']'
    }
    // initval
    /* The function decline of sysY does not allow para initialization
    ASTPtr init_val = nullptr;
    if(IsTokenOp(Operator::Assign)){
        NextToken();
        init_val = ParseInitVal();
    }
    */  
    //eat ','
    //if(IsTokenComma()) NextToken();
    return make_unique<FuncParaAST>(move(type), ident, move(dims));

    //考虑到调用情况应该是必须要有ident的？
}

ASTPtr Parser::ParseBlock(){
    assert(IsTokenPair(Pair::LBrace));
    NextToken(); //eat '{'
    ASTPtrList items;
    //declaration does not conflict with stmt
    while(!IsTokenPair(Pair::RBrace)){
        if(auto item = ParseFuncVar()){
            items.push_back(move(item));
        }
        else if(auto item = ParseStmt()){
            items.push_back(move(item));
        }
        else{
            assert(0);
        }
    }
    NextToken();//eat '}'
    return make_unique<BlockAST>(move(items));
}

ASTPtr Parser::ParseStmt(){
    if(cur_token_ == Token::Keyword){
        switch(lexer_.KeyVal()){
            case Keyword::If:
                return ParseIfElse(); break;
            case Keyword::While:
                return ParseWhile(); break;
            case Keyword::Break:
            case Keyword::Continue:
            case Keyword::Return:
                //eat ; in function
                return ParseControl(); break;
        }
    }
    else if(IsTokenPair(Pair::LBrace)){
         return ParseBlock();
    }
    else if(IsTokenSemic()){
        return nullptr;
    }
    else{
        //lval or (
        auto expr = ParseExpr();
        assert(IsTokenSemic());
        NextToken(); //eat ;  
        return move(expr);      
    }
    return nullptr;
}

//4.18 continue the check work
ASTPtr Parser::ParseIfElse(){
    assert(IsTokenKey(Keyword::If));
    NextToken(); //eat if
    assert(IsTokenPair(Pair::LParen));
    NextToken(); //eat '('

    auto cond = ParseExpr();

    assert(IsTokenPair(Pair::RParen));
    NextToken(); //eat ')'
    auto then_stmt = ParseStmt();

    if(IsTokenKey(Keyword::Else)){
        NextToken();
        auto else_then_stmt = ParseStmt();
        return make_unique<IfElseAST>(move(cond), move(then_stmt), move(else_then_stmt));
    }
    else{
        return make_unique<IfElseAST>(move(cond), move(then_stmt), nullptr);
    }
}

ASTPtr Parser::ParseWhile(){
    assert(IsTokenKey(Keyword::While));
    NextToken();
    assert(IsTokenPair(Pair::LParen));
    NextToken();
    auto cond = ParseExpr();
    assert(IsTokenPair(Pair::RParen));
    NextToken();
    auto stmt = ParseStmt();
    return make_unique<WhileAST>(move(cond), move(stmt));
}

//eat ;
ASTPtr Parser::ParseControl(){
    //NextToken(); //eat ident
    if(IsTokenKey(Keyword::Return)){
        NextToken(); //eat return 
        if(auto expr = ParseExpr()){
            assert(IsTokenSemic());
            NextToken();
            return make_unique<ControlAST>(Keyword::Return, move(expr));
        }
        else{
            assert(IsTokenSemic());
            NextToken();
            return make_unique<ControlAST>(Keyword::Return, nullptr);
        } 
    }
    else{
        NextToken(); //eat
        assert(IsTokenSemic());
        NextToken();
        return make_unique<ControlAST>(lexer_.KeyVal(), nullptr);
    } 
}
 
/**
 * return nullptr
 * 将中缀表达式转化为后缀表达式构造后缀表达式的语法树
 * 先生成后缀表达式再转化为语法树，还是在生成后缀表达式的同时构建语法树
 * 1 + 3 * 5
 * 1 3 5 * +
 * 当弹出一个操作符时，从expr_stack弹出两个ASTPtr，生成一个ASTPtr，再压入栈中
 * 实现参考了MIMIC
*/
ASTPtr Parser::ParseExpr(){
    /*
    if(lexer_.LineNum() == 18){
        cout << "here";
    }
    */
    auto lhs = ParseUnaryExpr();
    if(lhs == nullptr) return nullptr;
    stack<ASTPtr> exprs;
    stack<Operator> ops;

    exprs.push(move(lhs));
    
    while(cur_token_ == Token::Opeator){
        //这里并没有处理异常情况，理论上应该是双目运算符
        auto op = lexer_.OpVal();
        while(!ops.empty() && op_priority[ops.top()] >= op_priority[op]){
            auto bop = move(ops.top()); ops.pop();
            auto rhs = move(exprs.top()); exprs.pop();
            auto lhs = move(exprs.top()); exprs.pop();
            auto binary_expr = make_unique<BinaryExpAST>(move(lhs), move(rhs), bop);
            exprs.push(move(binary_expr));
        }
        ops.push(op);
        NextToken();
        //理论上也是必须要有expr的
        auto lhs = ParseUnaryExpr();
        if(!lhs) return nullptr;
        exprs.push(move(lhs));      
    }

    while(!ops.empty()){
        auto op = ops.top(); ops.pop();

        auto rhs = move(exprs.top()); exprs.pop();
        auto lhs = move(exprs.top()); exprs.pop();
        auto binary_expr = make_unique<BinaryExpAST>(move(lhs), move(rhs), op);
        exprs.push(move(binary_expr));    
    }
    assert(exprs.size() == 1);
    return move(exprs.top());
}

//return nullptr
ASTPtr Parser::ParseUnaryExpr(){
    if(IsTokenOp(Operator::Add) || IsTokenOp(Operator::Sub) || IsTokenOp(Operator::Not)){
        auto unary_op = lexer_.OpVal();
        NextToken(); //eat unary operator
        auto unary_expr = ParseUnaryExpr();
        return make_unique<UnaryExpAST>(unary_op, move(unary_expr));
    }
    else if(IsTokenPair(Pair::LParen)){
        NextToken(); //eat (
        auto unary_expr = ParseExpr();
        assert(IsTokenPair(Pair::RParen));
        NextToken();
        return make_unique<UnaryExpAST>(Operator::Blank, move(unary_expr));
    }
    else if(IsTokenIdent()){
        string ident = lexer_.IdVal();
        NextToken();
        if(cur_token_ == Token::Pair && lexer_.PairVal() == Pair::LParen){
            auto fun_call = ParseFuncCall(ident); //eat ()
            //assert(lexer_.PairVal() == Pair::RParen);
            //NextToken();
            return make_unique<UnaryExpAST>(Operator::Blank, move(fun_call));
        }
        else {
            // a[] or a 同样我没做其他检查，迟早要完蛋
            auto lval = ParseLVal(ident);
            return make_unique<UnaryExpAST>(Operator::Blank, move(lval));
        }
    }
    else if(cur_token_ == Token::ConstInt){
        NextToken();
        auto val = make_unique<IntAST>(lexer_.IntVal());
        return make_unique<UnaryExpAST>(Operator::Blank, move(val));
    }
    else{
        //NextToken();
        return nullptr; //空的单目表达式
    }
}

ASTPtr Parser::ParseLVal(string _ident){
    //调用的时候ident一定存在，此时已经指向了下一个token
    ASTPtrList indexs;
    if(IsTokenPair(Pair::LBracket)){ //etc a[...]
        do{
            NextToken(); //eat [
            auto index = ParseExpr();
            indexs.push_back(move(index));
            assert(IsTokenPair(Pair::RBracket));
            NextToken(); //eat ]
        }while(IsTokenPair(Pair::LBracket));
        //if(IsTokenPair(Pair::RBracket)) NextToken(); //eat ]
        return make_unique<LValAST>(_ident, move(indexs));       
    }
    else{ //etc a
        return make_unique<LValAST>(_ident, move(indexs));
    }

    //maybe it is a

}

ASTPtr Parser::ParseFuncCall(string _ident){
    assert(IsTokenPair(Pair::LParen));
    NextToken();
    ASTPtrList paras;

    while(!IsTokenPair(Pair::RParen)){
       auto para = ParseExpr();//这里需要expr能返回一个nullptr
       //能不能不犯傻逼错误
       if(para) paras.push_back(move(para));
       if(IsTokenComma()) NextToken();
    }
    //eat ')'
    NextToken();
    return make_unique<FuncCallAST>(_ident,lexer_.LineNum(), move(paras));
}
/*
光荣退休
ASTPtr Parser::ParseAssign(){
    auto ident = IdentAST(lexer_.IdVal());
    NextToken();
    assert(IsTokenOp(Operator::Assign));
    NextToken();
    auto expr = ParseExpr();
    return make_unique<AssignAST>(move(ident), move(expr));
}
*/