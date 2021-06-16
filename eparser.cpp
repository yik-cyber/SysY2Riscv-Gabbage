#include <iostream>
#include <cassert>
#include <cstring>
#include <utility>
#include <cctype>
#include <cstdlib>
#include "eeyore.hpp"
#include "eparser.hpp"
using namespace std;

#define PUSHINST(inst) \
    if(IsGlobal()) \
        gm_.PushInst(move(inst)); \
    else \
        cur_block_->PushInst(move(inst))

#define ADDTABLE(ident, var) \
    if(IsGlobal()) \
        gm_.AddTable(ident, var); \
    else \
        cur_func_->AddTable(ident, var);

#define OP_ELEM(token, str) {str, Operator::token} 
typedef unordered_map<string, Operator> OpMAP;
OpMAP operator_map_{
    OP_ELEM(Add, "+"), OP_ELEM(Sub, "-"), OP_ELEM(Mul, "*"), OP_ELEM(Div, "/"), OP_ELEM(Mod, "%"), 
    OP_ELEM(Eq, "=="), OP_ELEM(NotEq, "!="), OP_ELEM(Less, "<"), OP_ELEM(More, ">"), OP_ELEM(LessEq, "<="), OP_ELEM(MoreEq, ">="),
    OP_ELEM(Not, "!"), OP_ELEM(And, "&&"), OP_ELEM(Or, "||")
};
inline bool IsVarPrefix(char ch){
    return (ch == 't' || ch == 'T' || ch == 'p');
}

inline bool IsUnaryOp(char ch){
    return (ch == '-' || ch == '!');
}


void EParser::ParseNext() {
    if(in_.eof()){
        setend = true;
        return;
    }   
    //while(!in_.eof() && isspace(last_char_)) NextChar();  
    if(last_char_ == '\n'){
        ++line_;
        NextChar();
        return;
    }
    if(last_char_ == 'v'){
       ParseVarDecl(); 
       return;
    } 
    if(last_char_ == 'f'){
       ParseFuncDef(); 
       return;
    } 
    if(last_char_ == 'p'){
        if(PeekChar() == 'a'){
            ParseParam();
        }
        else ParseExpr();
        return;
    }
    if(last_char_ == 't' || last_char_ == 'T'){
        ParseExpr();
        return;
    }
    if(last_char_ == 'i'){
        ParseIf();
        return;
    }
    if(last_char_ == 'g'){
        ParseGoto();
        return;
    }
    if(last_char_ == 'l'){
        ParseL();
    }
    if(last_char_ == 'c'){
        ParseFuncCall(nullptr);
        return;
    }
    if(last_char_ == 'e'){
        ParseEnd();
        return;
    }
    if(last_char_ == 'r'){
        ParseReturn();
        return;
    }
    //setend = true;
}

void EParser::ParseFuncDef(){
    string ident;
    assert(last_char_ == 'f');
    NextChar();
    assert(last_char_ == '_');
    NextChar();
    while (isalnum(last_char_) || last_char_ == '_'){
        ident += last_char_;
        NextChar();
    }
    assert(last_char_ == '[');
    string num_str;
    NextChar();
    while(isdigit(last_char_)){
        num_str += last_char_;
        NextChar();
    }
    assert(last_char_ == ']');
    NextChar(); // eat ']'
    int num = strtoul(num_str.c_str(), NULL, 10);
    
    // 创建当前的函数，并同时设置循环分析器
    // 在初始化一个函数时，同时也要初始化block的编号
    Block::ResetBlockID();
    cur_func_ = make_shared<EFuncDefVal>(ident, num, line_);
    auto loop_analyzer = make_shared<CFGAnalyzer>(cur_func_);
    cur_func_->SetAnalyzer(loop_analyzer);
    fs_.AddFunc(cur_func_);
    gm_.AddFunc(ident, cur_func_);

    //创建block
    MakeBlock(false);
    
    // function header 不加入inst
    cur_block_->PushInst(make_shared<EFuncDefInst>(cur_func_ ,line_));
    
    // 设置上一个inst是否为branch
    is_last_branch_ = false;
    is_last_cut_ = false;
}

//定义并加入符号表
void EParser::ParseVarDecl(){
    // 上一个inst是branch inst
    if(is_last_branch_)
        MakeBlock(false);

    NextChar();
    assert(last_char_ == 'a'); NextChar();
    assert(last_char_ == 'r'); NextChar();
    string ident;

    if(isdigit(last_char_)){
        auto dim = ParseInt();
        ident += last_char_; NextChar();
        int id = ParseInt();
        auto array_val = make_shared<EArrayVal>(ident.c_str()[0], id, dim);
        ident += to_string(id);
        if(IsGlobal()){
            array_val->is_global_ = true;
            gm_.PushInst(make_shared<EVarDecInst>(array_val, true, line_));
        }
        else{
            array_val->SetStackPos(cur_func_->GetStackSize());
            cur_func_->AddStackSize(dim);
        } 
        ADDTABLE(ident, array_val);
        //auto decl = make_shared<VarDecInst>(array_val, true, line_);
        //PUSHINST(decl);
    }
    else{
        ident += last_char_; NextChar();
        int id = ParseInt();
        auto var_val = make_shared<EVarVal>(ident.c_str()[0], id);
        if(IsGlobal()) {
            var_val->is_global_ = true;
            gm_.PushInst(make_shared<EVarDecInst>(var_val, false, line_));
        }
        ident += to_string(id); 
        ADDTABLE(ident, var_val);      
        //auto decl = make_shared<VarDecInst>(var_val, false, line_);
        //PUSHINST(decl);
    }

    is_last_branch_ = false;
    is_last_cut_ = false;
    return;
}

void EParser::ParseExpr(){
    // 上一个inst是branch inst
    if(is_last_branch_)
        MakeBlock(false);

    auto dest = ParseLVal();
    assert(last_char_ == '=');
    NextChar();
    Operator op;
    if(IsUnaryOp(last_char_)){
        switch(last_char_){
            case '!':
                op = Operator::Not; break;
            case '-':
                op = Operator::Sub; break;
        }
        NextChar();
        auto rval = ParseRVal();
        EInstPtr inst;
        if(rval->val_type_ == EValType::Int){
            rval->Minus();
            if(last_char_ != '\n'){
            string op_str;
            op_str += last_char_;
            NextChar();
            if(last_char_ == '=' || last_char_ == '&' || last_char_ == '|'){
                op_str += last_char_;
                NextChar();
            }
            op = operator_map_[op_str]; // get op
            auto rhs = ParseRVal();
            if(rval->val_type_ == EValType::Int && rhs->val_type_ == EValType::Int){
                auto num = make_shared<EIntVal>(CalInt(rval->GetConstVal(), rhs->GetConstVal(), op));
                auto inst = make_shared<EAssignInst>(dest, num, line_);
                PUSHINST(inst);
            }
            else{
                auto inst = make_shared<EBinaryInst>(dest, rval, op, rhs, line_);
                PUSHINST(inst);
            }  
            }
            else{
                // t1 = -1
                auto inst = make_shared<EAssignInst>(dest, rval, line_);;
                PUSHINST(inst);
            }        
        }
        // not int
        else{
            auto inst = make_shared<EUnaryInst>(dest, op, rval, line_);
            PUSHINST(inst);
        }  
    }
    else if(last_char_ == 'c'){
        //把带call的赋值语句拆分成call和赋值
        ParseFuncCall(dest);
    }
    else{
        auto lhs = ParseRVal(); // symbol or num
        if(last_char_ == '['){
            NextChar();
            auto index = ParseRVal();
            assert(last_char_ == ']');
            NextChar(); // eat ']'
            auto rval = make_shared<EArrayElemVal>(lhs, index);
            auto inst = make_shared<EAssignInst>(dest, rval, line_);
            PUSHINST(inst);
        }
        else if(last_char_ == '\n'){
            auto inst = make_shared<EAssignInst>(dest, lhs, line_);
            PUSHINST(inst);
        }
        else{
            string op_str;
            op_str += last_char_;
            NextChar();
            if(last_char_ == '=' || last_char_ == '&' || last_char_ == '|'){
                op_str += last_char_;
                NextChar();
            }
            else if(last_char_ == '/'){
                HandleComment();
                return;
            }
            op = operator_map_[op_str]; // get op
            auto rhs = ParseRVal();
            if(lhs->val_type_ == EValType::Int && rhs->val_type_ == EValType::Int){
                auto num = make_shared<EIntVal>(CalInt(lhs->GetConstVal(), rhs->GetConstVal(), op));
                auto inst = make_shared<EAssignInst>(dest, num, line_);
                PUSHINST(inst);
            }
            else{
                auto inst = make_shared<EBinaryInst>(dest, lhs, op, rhs, line_);
                PUSHINST(inst);
            }            
        }
    }
    is_last_branch_ = false;
}

EValPtr EParser::ParseLVal(){
    auto var = ParseVar();
    if(last_char_ == '['){
        NextChar();
        auto index = ParseRVal();
        assert(last_char_ == ']');
        NextChar(); // eat ']'
        auto array_elem = make_shared<EArrayElemVal>(var, index);
        return array_elem;
    }
    else{
        return var;
    }
}


EValPtr EParser::ParseRVal(){
    if(last_char_ == '-'){
        NextChar();
        if(IsVarPrefix(last_char_)){
            auto var = ParseVar();
            return var;
        }
        else if(isdigit(last_char_)){
            auto num = ParseNum();
            num->Minus();
            return num;
        }      
    }
    else if(IsVarPrefix(last_char_)){
        auto var = ParseVar();
        return var;
    }
    else if(isdigit(last_char_)){
        auto num = ParseNum();
        return num;
    }
    else{
        return nullptr;
    }
}


EValPtr EParser::ParseVar(){
    string ident;
    ident += last_char_;
    NextChar(); // eat type
    int id = ParseInt();
    ident += to_string(id);
    

    auto val = cur_func_->GetVal(ident);
    if(val != nullptr){
        return val;
    }    
    val = gm_.GetVal(ident);
    if(val != nullptr){
        //为每个全局变量在每个函数内部创建一个备份
        EValPtr val_func;
        if(val->val_type_ == EValType::Var){
            val_func = make_shared<EVarVal>(val->GetType(), val->GetID());
            cur_func_->global_vars_.push_back(val_func);
        }
        else if(val->val_type_ == EValType::Array){
            val_func = make_shared<EArrayVal>(val->GetType(), val->GetID(), val->GetDim());
        }
        val_func->is_global_ = true;
        ADDTABLE(ident, val_func);
        //记录一哈函数中出现的全局变量   
        return val_func;
    }
    //这种只有函数参数的情况
    assert(ident[0] == 'p');
    val = make_shared<EVarVal>(ident.c_str()[0], id);
    cur_func_->AddTable(ident, val);
    return val;
}


int EParser::ParseInt(){
    assert(isdigit(last_char_) || last_char_ == '-');
    string digit;
    do{
        digit += last_char_;
        NextChar();
    }while(isdigit(last_char_));
    return strtoul(digit.c_str(), NULL, 10);
}


EValPtr EParser::ParseNum(){
    int num = ParseInt();
    return make_shared<EIntVal>(num);
}

void EParser::HandleComment(){
    assert(last_char_ == '/'); // the second /
    do{
        NextChar();
    }while(last_char_ != '\n');
    NextChar();
    return;
}

void EParser::ParseIf(){
    // 上一个inst是branch inst
    if(is_last_branch_)
        MakeBlock(false);

    NextChar();
    assert(last_char_ == 'f');
    NextChar();
    auto lhs = ParseRVal();
    string op_str; op_str += last_char_;
    NextChar();
    if(last_char_ == '&' || last_char_ == '|' || last_char_ == '='){
        op_str += last_char_;
        NextChar();
    }
    auto op = operator_map_[op_str];
    auto rhs = ParseRVal();
    assert(last_char_ == 'g');
    for(int i = 0; i < 4; i++) NextChar();
    auto label = ParseLabel();
    cur_block_->PushInst(make_shared<EIfInst>(lhs, op, rhs, label, line_));
    
    // set
    is_last_branch_ = true;
    is_last_cut_ = false;
}

EValPtr EParser::ParseLabel(){
    assert(last_char_ == 'l');
    NextChar();
    auto id = ParseInt();
    return make_shared<ELabelVal>(id);
}

void EParser::ParseGoto(){
    // 上一个inst是branch inst
    if(is_last_branch_)
        MakeBlock(false);

    for(int i = 0; i < 4; i++) NextChar();
    auto label = ParseLabel();
    cur_block_->PushInst(make_shared<EGotoInst>(label, line_));

    is_last_branch_ = true;
    is_last_cut_ = true;
}

void EParser::ParseL(){
    auto label = ParseLabel();
    assert(last_char_ == ':');
    NextChar();
    // 当前inst为label，创建一个新的block
    MakeBlock(true);
    cur_func_->AddLabeledBlock(label->GetID(), cur_block_);
    cur_block_->PushInst(make_shared<ELabelInst>(label, line_));
    is_last_branch_ = false;
    is_last_cut_ = false;
}

void EParser::ParseFuncCall(EValPtr _dest){
    // 上一个inst是branch inst
    if(is_last_branch_)
        MakeBlock(false);

    for(int i = 0; i < 4; i++) NextChar(); // eat call
    auto func = ParseFunc();
    auto call_inst = make_shared<ECallInst>(_dest, func, line_);
    cur_block_->PushInst(call_inst);
    //即将调用函数，弹出参数指令，设置指令指向的函数和对应的寄存器
    for(int i = func->GetParamNum() - 1; i >= 0; i--){
        auto param_inst = temp_params_.back();
        temp_params_.pop_back();
        param_inst->SetFunc(func);
        string reg = "a";
        reg += to_string(i);
        param_inst->SetReg(reg);
        param_inst->SetCall(call_inst);
        call_inst->AddVal(param_inst->GetParam());
        call_inst->AddParam(param_inst);
    }
    is_last_branch_ = false;
    is_last_cut_ = false;
}

void EParser::ParseParam(){
    // 上一个inst是branch inst
    if(is_last_branch_)
        MakeBlock(false);

    for(int i = 0; i < 5; ++i) NextChar(); // eat param
    auto rval = ParseRVal();
    auto inst = make_shared<EParamInst>(rval, line_);
    // 用一只栈记录调用指令
    temp_params_.push_back(inst);
    cur_block_->PushInst(inst);

    is_last_branch_ = false;
    is_last_cut_ = false;
}

void EParser::ParseReturn(){
    // 上一个inst是branch inst
    if(is_last_branch_)
        MakeBlock(false);

    for(int i = 0; i < 6; ++i) NextChar();
    auto rval = ParseRVal();
    cur_block_->PushInst(make_shared<EReturnInst>(line_, rval, cur_func_));        

    is_last_branch_ = false;
    is_last_cut_ = true;
}

EFuncDefPtr EParser::ParseFunc(){
    assert(last_char_ == 'f');
    NextChar(); NextChar(); // eat f_
    string ident;
    do{
        ident += last_char_;
        NextChar();
    }while(isalnum(last_char_) || last_char_ == '_');
    //现在全局符号表中寻找函数，找不到再加入
    auto val = gm_.GetFunc(ident);
    //函数必须要定义才能使用
    assert(val != nullptr);
    return val;
}

void EParser::ParseEnd(){
    // 上一个inst是branch inst
    if(is_last_branch_)
        MakeBlock(false);

    do{
        NextChar();
    }while(!in_.eof() && (isalnum(last_char_) || last_char_ == '_'));
    cur_func_->SetEndLoc(line_);
    // end不加入inst
    cur_block_->PushInst(make_shared<EEndInst>(cur_func_, line_));
    
    is_last_branch_ = false;
    is_last_cut_ = true;
    return;
}

// 在创建block的时候我们就保证没有空的block会被创建
// 1. 前一个inst是branch inst 
// 2. 当前inst是label inst
inline void EParser::MakeBlock(bool _labeled){
    if(IsGlobal()) return;
    auto block = make_shared<Block>(cur_func_, _labeled);
    // 当上一个block的last inst为Goto时，上一个block与当前block没有直接联系
    if(!is_last_cut_ && cur_block_){
        // 设置block的前驱
        block->SetPredecessor(cur_block_);
        block->AddPred(cur_block_);
        // block前驱数量+1
        block->IncForwardBlockNum();
        // 设置cur_block的后继
        cur_block_->SetSuccessor(block);
        cur_block_->AddSucs(block);
    }
    // 此时我们不处理跳转关系，在循环分析中再做处理
    cur_func_->PushBlock(block);
    cur_block_ = block; // 指向新创建的block
}

int EParser::CalInt(int x, int y, const Operator& op){
    switch(op){
        case Operator::Add:
            return x + y;
        case Operator::Sub:
            return x - y;
        case Operator::Mul:
            return x * y;
        case Operator::Div:
            return x / y;
        case Operator::Mod:
            return x % y;
        case Operator::Eq:
            return x == y;
        case Operator::Less:
            return x < y;
        case Operator::LessEq:
            return x <= y;
        case Operator::More:
            return x > y;
        case Operator::MoreEq:
            return x >= y;
        case Operator::Not:
            assert(false); break;
        case Operator::And:
            return x && y;
        case Operator::Or:
            return x || y;
        case Operator::NotEq:
            return x != y;
        default:
            assert(false);
    }
    return 0;
}