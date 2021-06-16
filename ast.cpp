#include "ast.hpp"
#include "lexer.hpp"
#include "irgen.hpp"

#define OP_STR(token, str) {Operator::token, str} 
typedef unordered_map<Operator, string> Op2StrMap;
Op2StrMap op2str_map{
    OP_STR(Add, "+"), OP_STR(Sub, "-"), OP_STR(Mul, "*"), OP_STR(Div, "/"), OP_STR(Mod, "%"), 
    OP_STR(Addr, "&"), OP_STR(Inc, "++"),OP_STR(Dec, "--"), 
    OP_STR(Eq, "=="), OP_STR(NotEq, "!="), OP_STR(Less, "<"), OP_STR(More, ">"), OP_STR(LessEq, "<="), OP_STR(MoreEq, ">="),
    OP_STR(Not, "!"), OP_STR(And, "&&"), OP_STR(Or, "||"),
    OP_STR(Assign, "="), OP_STR(AddAssign, "+="), OP_STR(SubAssign, "-="), OP_STR(MulAssign, "*="),OP_STR(DivAssign, "/="), OP_STR(ModAssgin, "%=")

};


/**
 * print the AST
*/
void VarDeclAST::print(ofstream& ofs){
    type->print(ofs); ofs << " ";
    for(int i = 0; i < defs.size(); i++){
        defs[i]->print(ofs); ofs << ", ";
    }
    ofs << "\n";
}


void VarDefAST::print(ofstream& ofs){
    ofs << ident_;
    for(int i = 0; i < dims_.size(); i++){
        ofs << "[";
        dims_[i]->print(ofs);
        ofs << "]";
    }
    if(init_val_){
        ofs << " = ";
        init_val_->print(ofs);
    }
}

void InitValAST::print(ofstream& ofs){
    ofs << "{ ";
    for(int i = 0; i < init_vals_.size(); i++){
        init_vals_[i]->print(ofs);
        ofs << ", ";
    }
    ofs << "}";
}

void FuncDeclAST::print(ofstream& ofs){
    type_->print(ofs);
    ofs << " " << ident_ << "("; 
    for(int i = 0; i < paras_.size(); i++){
        paras_[i]->print(ofs);
        ofs << ", ";
    }
    ofs << ")" << endl;
}

void FuncDefAST::print(ofstream& ofs){
    type_->print(ofs);
    ofs << " " << ident_ << "(";
    for(int i = 0; i < paras_.size(); i++){
        paras_[i]->print(ofs);
        ofs << ", ";
    }
    ofs << ")\n";
    block_->print(ofs);
}

void BlockAST::print(ofstream& ofs){
    ofs << "{\n";
    for(int i = 0; i < block_items_.size(); i++){
        block_items_[i]->print(ofs);
        ofs << "\n";
    }
    ofs << "}\n";
}

void FuncParaAST::print(ofstream& ofs){
    type_->print(ofs);
    ofs << " " << ident_;
    for(int i = 0; i < dims_.size(); i++){
        ofs << "[";
        if(dims_[i]) dims_[i]->print(ofs);
        ofs << "]";
    }
}

void IfElseAST::print(ofstream& ofs){
    ofs << "if(";
    cond_->print(ofs);
    ofs << ")\n";
    then_->print(ofs);
    ofs << "\n";
    if(else_then_){
        ofs << "else\n";
        else_then_->print(ofs);
    }
}

void WhileAST::print(ofstream& ofs){
    ofs << "while(";
    cond_->print(ofs); ofs << ")\n";
    stmt_->print(ofs);
}

void ControlAST::print(ofstream& ofs){
    switch(type_){
        case Keyword::Break:
            ofs << "break "; break;
        case Keyword::Continue:
            ofs << "continue "; break;
        case Keyword::Return:
            ofs << "return ";
            if(expr_) expr_->print(ofs);
    }
}

void FuncCallAST::print(ofstream& ofs){
    ofs << ident_ << "(";
    for(int i = 0; i < paras_.size(); i++){
        paras_[i]->print(ofs);
        ofs << ", ";
    }
    ofs << ")";
}

void LValAST::print(ofstream& ofs){
    ofs << ident_;
    for(int i = 0; i < index_.size(); i++){
        ofs << "[";
        index_[i]->print(ofs);
        ofs << "]";
    }
}

void UnaryExpAST::print(ofstream& ofs){
    ofs << "(";
    switch(unary_op_){
        case Operator::Add:
                ofs << "+"; break;
        case Operator::Sub:
            ofs << "-"; break;
        case Operator::Not:
            ofs << "!"; break;
    }
    expr_->print(ofs);
    ofs << ")";
}

void BinaryExpAST::print(ofstream& ofs){
    lhs_->print(ofs);
    ofs << op2str_map[op_];
    rhs_->print(ofs);
}

/**
 * calculate value
*/

int VarDefAST::GetTotDimNum(IRGenerator& _irgen){
    if(dims_.empty()) return -1; //not an array
    assert(dim_num_.empty()); //only calculate once
    tot_dim_num_ = 1;
    for(const auto& expr: dims_){
        int cur_dim = expr->GenerateInt(_irgen);
        dim_num_.push_back(cur_dim);
        tot_dim_num_ *= cur_dim;
    }
    dim_num_.push_back(1); // 1 in the end
    int tmp = 1;
    for(int i = dim_num_.size() - 1; i >= 0; i--){
        tmp *= dim_num_[i];
        acc_num_.push_back(tmp);
    }
    reverse(acc_num_.begin(), acc_num_.end()); //reverse
    // e.g. a[2][3], 6 3 1
    return tot_dim_num_;
}

int IntAST::GenerateInt(IRGenerator& _irgen){
    return val_;
}

//不会真有人用指针作为LVal值吧
int LValAST::GenerateInt(IRGenerator& _irgen){
    auto val_ptr = _irgen.GetVarVal(ident_); //val can be var
    if(!val_ptr->IsArray()){
        return val_ptr->GetConstVal();
    }
    auto dims = val_ptr->GetDims();
    auto accs = val_ptr->GetAccs();
    index_num_ = 0;
    assert(val_ptr->GetDimNum() == index_.size());
    for(int i = 0; i < index_.size(); i++){
        int cur_index = index_[i]->GenerateInt(_irgen);
        index_num_ += cur_index * accs[i+1];
    }
    auto val = val_ptr->GetConstVal(index_num_);
    return val;
}

int UnaryExpAST::GenerateInt(IRGenerator& _irgen){
    val_ = expr_->GenerateInt(_irgen);
    switch(unary_op_){
        case Operator::Sub:
            val_ = -val_; break;
        case Operator::Not:
            val_ = !val_; break;
    }
    return val_;
}


int BinaryExpAST::GenerateInt(IRGenerator& _irgen){
    auto lhs = lhs_->GenerateInt(_irgen);
    auto rhs = rhs_->GenerateInt(_irgen);
    switch(op_){
        case Operator::Add:
            val_ = lhs + rhs; break;
        case Operator::Sub:
            val_ = lhs - rhs; break;
        case Operator::Mul:
            val_ = lhs * rhs; break;
        case Operator::Div:
            val_ = lhs / rhs; break;
        case Operator::Mod:
            val_ = lhs % rhs; break;
        case Operator::Less:
            val_ = lhs < rhs; break;
        case Operator::More:
            val_ = lhs > rhs; break;
        case Operator::LessEq:
            val_ = lhs < rhs; break;
        case Operator::MoreEq:
            val_ = lhs > rhs; break;
        case Operator::Eq:
            val_ = lhs < rhs; break;
        case Operator::NotEq:
            val_ = lhs > rhs; break;
        case Operator::And:
            val_ = lhs && rhs; break;
        case Operator::Or:
            val_ = lhs || rhs; break;
        default:
            assert(false);
    }
    return val_;
}

int InitValAST::HandleInitVal(IRGenerator& _irgen, shared_ptr<ArrayVal> _val_ptr, int _cur_dim, int _cur_index){
    // vals vector
    //vector<ValPtr>& store = _val_ptr->GetVals();
    // 当前括号内应该放多少元素
    int tot_set_num = _val_ptr->GetAccs()[_cur_dim];
    const auto& func_ptr = _irgen.CurFuncPtr();
    // 已经放置的元素个数
    int k = 0; 
    for(const auto& ast: init_vals_){
        if(ast->IsLiteral()){
            auto elem_val_ptr = ast->GenerateIR(_irgen);
            if(elem_val_ptr->IsArrayElem()){
                auto tp = make_shared<VarVal>(VarType::t,func_ptr->AddtSlot());
                func_ptr->PushDecl(make_shared<VarDecInst>(tp, false));
                func_ptr->PushInst(make_shared<AssignInst>(tp, elem_val_ptr));
                elem_val_ptr = move(tp);
            }
            //store.push_back(ast->GenerateIR(_irgen));
            //auto tp = make_shared<VarVal>(VarType::t, _irgen.CurFuncPtr()->AddtSlot());
            int idx = (_cur_index + k) * 4;
            auto idx_val_ptr = make_shared<IntVal>(idx);
            /*
            func_ptr->PushInst(make_shared<BinaryInst>(tp, make_shared<IntVal>(_cur_index + k), 
                                           Operator::Mul, make_shared<IntVal>(4)));
            */
            
            auto array_elem = make_shared<ArrayElemVal>(_val_ptr, idx_val_ptr);
            func_ptr->PushInst(make_shared<AssignInst>(array_elem, elem_val_ptr));
            k++;
        }
        else{
            k += ast->HandleInitVal(_irgen, _val_ptr, _cur_dim + 1, k);
        }
    }
    while(k < tot_set_num){
        //store.push_back(0);
        k++;
    }
    return tot_set_num;
}

int InitValAST::HandleInitInt(IRGenerator& _irgen, shared_ptr<ArrayVal> _val_ptr, int _cur_dim, int _cur_index){
    // vals vector
    // 当前括号内应该放多少元素
    int tot_set_num = _val_ptr->GetAccs()[_cur_dim];
    vector<int>& vals = _val_ptr->GetVals();
    const auto& func_ptr = _irgen.CurFuncPtr();
    // 已经放置的元素个数
    int k = 0; 
    for(const auto& ast: init_vals_){
        if(ast->IsLiteral()){
            auto elem_val = ast->GenerateInt(_irgen);
            vals.push_back(elem_val);
            auto array_elem = make_shared<ArrayElemVal>(_val_ptr, make_shared<IntVal>((_cur_index+k)*4));
            func_ptr->PushInst(make_shared<AssignInst>(array_elem, make_shared<IntVal>(elem_val)));
            k++;
        }
        else{
            k += ast->HandleInitInt(_irgen, _val_ptr, _cur_dim + 1, k);
        }
    }
    while(k < tot_set_num){
        vals.push_back(0);
        k++;
    }
    return tot_set_num;
}

/**
 * generate IR
*/

ValPtr VarDeclAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr VarDefAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

/*
ValPtr InitValAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}
*/

ValPtr FuncDefAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr BlockAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}


ValPtr FuncParaAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr IfElseAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr WhileAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr ControlAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr FuncCallAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr LValAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr UnaryExpAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr BinaryExpAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}

ValPtr IntAST::GenerateIR(IRGenerator& _irgen){
    return _irgen.GenerateOn(*this);
}