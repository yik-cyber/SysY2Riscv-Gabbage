#include "irgen.hpp"
using namespace std;

void IRGenerator::Dump(stringstream &ofs){
    for(auto inst: global_decls_){
        inst->Dump(ofs);
    }
    for (const auto &it : func_vector_){
        if(it->IsGlobal()) continue;
        it->Dump(ofs);
    }
}

ValPtr IRGenerator::GetVarVal(string &_ident){
    for (int i = var_tabel_stack_.size() - 1; i >= 0; i--){
        if (var_tabel_stack_[i].find(_ident) != var_tabel_stack_[i].end()){
            return var_tabel_stack_[i][_ident];
        }
    }
    //Not find the value
    assert(false);
    return nullptr;
}

FuncDefPtr IRGenerator::GetFuncPtr(string &_ident){
    if (auto func_ptr = func_tabel_[_ident])
        return func_ptr;
    else if (auto func_ptr = lib_func_tabel_[_ident])
        return func_ptr;
    assert(false);
    return nullptr;
}

ValPtr IRGenerator::GenerateOn(FuncDefAST &ast){
    auto type = ast.GetType();
    auto ident = ast.GetIdent();
    //add to tabel
    cur_func_ptr_ = make_shared<FuncDef>(type, ident, ast.GetParaNum());
    func_tabel_.insert({ast.GetIdent(), cur_func_ptr_});
    //
    if(ident == "main"){
        // you wenti
        cur_func_ptr_->SetGlobalFunc(func_vector_.front());
        cur_func_ptr_->SetMainLocalSlot(func_vector_.front()->GettSlot());
    }
    //func stack
    func_vector_.push_back(cur_func_ptr_);
    func_stack_.push_back(cur_func_ptr_);
    //create a new var tabel
    var_tabel_stack_.push_back({});
    cur_var_tabel_it_ = var_tabel_stack_.end() - 1;
    //cur_var_tabel_ = var_tabel_stack_.back();
    //add args. do not need declarations
    for (const auto &para : ast.GetParas()){
        para->GenerateIR(*this); //add to tabel
    }
    //block
    ast.GetBlock()->GenerateIR(*this);
    //pop var tabel
    var_tabel_stack_.pop_back();
    cur_var_tabel_it_ = var_tabel_stack_.end() - 1;
    //cur_var_tabel_ = var_tabel_stack_.back();
    if (!cur_func_ptr_->HasReturn()){
        cur_func_ptr_->PushInst(make_shared<ReturnInst>(nullptr));
    }
    int lastTslot = cur_func_ptr_->GetTSlot();
    func_stack_.pop_back();
    cur_func_ptr_ = func_stack_.back();
    if (IsGlobal()){
        cur_func_ptr_->UpdateGlobalSlot(lastTslot);
    }
    return nullptr;
}

//func def para
ValPtr IRGenerator::GenerateOn(FuncParaAST &ast){
    int id = cur_func_ptr_->AddpSlot();
    const auto &ident = ast.GetIdent();
    const auto &dims = ast.GetDims();
    if (dims.empty()){
        // var
        auto val_ptr = make_shared<VarVal>(VarType::p, id);
        // add to tabel
        cur_var_tabel_it_->insert({ident, val_ptr});
    }
    else{
        vector<int> dim_num;
        vector<int> acc_num;
        // ignore []
        dim_num.push_back(0);
        for (int i = 1; i < dims.size(); i++){
            // constant
            int t = dims[i]->GenerateInt(*this);
            dim_num.push_back(t);
        }
        dim_num.push_back(1);

        int acc = 1;
        for (int i = dim_num.size() - 1; i >= 1; i--){
            acc *= dim_num[i];
            acc_num.push_back(acc);
        }
        acc_num.push_back(0);
        reverse(acc_num.begin(), acc_num.end());
        auto val_ptr = make_shared<ArrayVal>(VarType::p, id, dim_num, acc_num, -1);
        cur_var_tabel_it_->insert({ident, val_ptr});
    }
    return nullptr;
}

ValPtr IRGenerator::GenerateOn(BlockAST &ast){
    //create a new var tabel
    var_tabel_stack_.push_back({});
    cur_var_tabel_it_ = var_tabel_stack_.end() - 1;
    //cur_var_tabel_ = var_tabel_stack_.back();
    //stmts
    for (const ASTPtr &stmt : ast.GetStmts()){
        stmt->GenerateIR(*this);
    }
    // pop var tabel
    var_tabel_stack_.pop_back();
    cur_var_tabel_it_ = var_tabel_stack_.end() - 1;
    //cur_var_tabel_ = var_tabel_stack_.back();

    return nullptr;
}

ValPtr IRGenerator::GenerateOn(VarDeclAST &ast){
    for (const auto &def : ast.GetDefs()){
        auto val_ptr = def->GenerateIR(*this);
    }
    return nullptr;
}

ValPtr IRGenerator::GenerateOn(VarDefAST &ast){
    //add to tabel
    int id = cur_func_ptr_->AddTSlot();
    string ident = ast.GetIdent();
    // elemts num
    int tot_dim_num = ast.GetTotDimNum(*this);

    if (cur_func_ptr_->IsGlobal()){

        if (tot_dim_num < 0){
            // normal var
            auto val_ptr = make_shared<VarVal>(VarType::T, id);
            // initval
            if (auto &init_val_ast = ast.GetInitVal()){
                assert(init_val_ast != nullptr);
                // exp
                auto init_val = init_val_ast->GenerateInt(*this);
                auto init_val_ptr = make_shared<IntVal>(init_val);
                cur_func_ptr_->PushInst(make_shared<AssignInst>(val_ptr, init_val_ptr));
                val_ptr->SetVal(init_val_ptr);
            }
            // not initialized, do nothing
            cur_var_tabel_it_->insert({ident, val_ptr});
            global_decls_.push_back(make_shared<VarDecInst>(val_ptr, false, false));
            return val_ptr;
        }
        else{
            // array
            auto val_ptr = make_shared<ArrayVal>(VarType::T, id, ast.GetDimNum(), ast.GetAccNum(), tot_dim_num);
            // initialization
            if (auto &init_val_ast = ast.GetInitVal()){
                assert(init_val_ast != nullptr);
                // get values for array, return the set number
                int check = init_val_ast->HandleInitInt(*this, val_ptr, 0, 0);
                assert(check == val_ptr->GetTotDimNum());
            }
            //do nothing
            cur_var_tabel_it_->insert({ident, val_ptr});
            global_decls_.push_back(make_shared<VarDecInst>(val_ptr, true, false));
        }
    }
    else{

        if (tot_dim_num < 0){
            // normal var
            auto val_ptr = make_shared<VarVal>(VarType::T, id);
            // initval
            if (auto &init_val_ast = ast.GetInitVal()){
                assert(init_val_ast != nullptr);
                // exp
                auto init_val = init_val_ast->GenerateIR(*this);
                cur_func_ptr_->PushInst(make_shared<AssignInst>(val_ptr, init_val));
                val_ptr->SetVal(init_val);
            }
            // not initialized, do nothing
            cur_var_tabel_it_->insert({ident, val_ptr});
            cur_func_ptr_->PushDecl(make_shared<VarDecInst>(val_ptr, false, false));
            return val_ptr;
        }
        else{
            // array
            auto val_ptr = make_shared<ArrayVal>(VarType::T, id, ast.GetDimNum(), ast.GetAccNum(), tot_dim_num);
            // initialization
            if (auto &init_val_ast = ast.GetInitVal()){
                assert(init_val_ast != nullptr);
                // get values for array, return the set number
                int check = init_val_ast->HandleInitVal(*this, val_ptr, 0, 0);
                assert(check == val_ptr->GetTotDimNum());
            }
            //do nothing
            cur_var_tabel_it_->insert({ident, val_ptr});
            cur_func_ptr_->PushDecl(make_shared<VarDecInst>(val_ptr, true, false));
        }
    }
    return nullptr;
}

ValPtr IRGenerator::GenerateOn(IfElseAST &ast){
    // if ... else ...
    auto then_label = make_shared<LabelVal>();
    auto else_label = make_shared<LabelVal>();
    // branch
    auto branch = make_shared<CondBranch>(then_label, else_label);
    branch_stack_.push_back(branch);
    //cur_func_ptr_->PushInst(make_shared<BranchInst>(branch)); //before then
    // cond
    cur_func_ptr_->SetInCond();
    auto cond = ast.GetCond()->GenerateIR(*this);
    cur_func_ptr_->UnsetInCond();
    branch_stack_.pop_back();
    assert(branch_stack_.empty());
    if (cond != nullptr){
        if(cond->IsArrayElem()){
            auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
            cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
            cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, cond));
            cond = move(tp);
        }
        cur_func_ptr_->PushInst(make_shared<IfInst>(cond, else_label));
    }
    //then
    cur_func_ptr_->PushInst(make_shared<LabelInst>(then_label));
    ast.GetThen()->GenerateIR(*this);
    const auto &else_then = ast.GetElseThen();
    if (else_then != nullptr){ //if ... else ... end ...
        auto end_label = make_shared<LabelVal>();
        cur_func_ptr_->PushInst(make_shared<GotoInst>(end_label)); //end if
        cur_func_ptr_->PushInst(make_shared<LabelInst>(else_label));
        else_then->GenerateIR(*this); // else then
        cur_func_ptr_->PushInst(make_shared<LabelInst>(end_label));
    }
    else{ //if ... end ...
        cur_func_ptr_->PushInst(make_shared<LabelInst>(else_label));
    }
    return nullptr;
}

ValPtr IRGenerator::GenerateOn(WhileAST &ast){
    // cond label
    auto cond_label_ptr = make_shared<LabelVal>();
    cur_func_ptr_->PushCondLabel(cond_label_ptr);
    cur_func_ptr_->PushInst(make_shared<LabelInst>(cond_label_ptr));
    // body label
    auto body_label_ptr = make_shared<LabelVal>();
    // end label
    auto end_label_ptr = make_shared<LabelVal>();
    cur_func_ptr_->PushEndLabel(end_label_ptr);
    // add label
    cur_func_ptr_->SetThenLabel(body_label_ptr);
    cur_func_ptr_->SetElseThenLabel(end_label_ptr);
    // branch
    auto branch = make_shared<CondBranch>(body_label_ptr, end_label_ptr); // true and false
    branch_stack_.push_back(branch);
    // cond = !cond
    cur_func_ptr_->SetInCond();
    auto cond_val_ptr = ast.GetCond()->GenerateIR(*this);
    cur_func_ptr_->UnsetInCond();
    branch_stack_.pop_back();
    // if !cond goto end label
    if (cond_val_ptr != nullptr){
        if(cond_val_ptr->IsArrayElem()){
            auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
            cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
            cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, cond_val_ptr));
            cond_val_ptr = move(tp);
        }
        cur_func_ptr_->PushInst(make_shared<IfInst>(cond_val_ptr, end_label_ptr));
    }
    // stmt
    cur_func_ptr_->PushInst(make_shared<LabelInst>(body_label_ptr));
    ast.GetStmt()->GenerateIR(*this);
    // goto cond label
    cur_func_ptr_->PushInst(make_shared<GotoInst>(cond_label_ptr));
    // end label
    cur_func_ptr_->PushInst(make_shared<LabelInst>(end_label_ptr));
    // pop labels
    cur_func_ptr_->PopCondLabel();
    cur_func_ptr_->PopEndLabel();

    return nullptr;
}

ValPtr IRGenerator::GenerateOn(ControlAST &ast){
    auto control_type = ast.GetControlType();
    switch (ast.GetControlType()){
    case Keyword::Break:
        cur_func_ptr_->PushInst(make_shared<GotoInst>(cur_func_ptr_->TopEndLabel()));
        break;
    case Keyword::Continue:
        cur_func_ptr_->PushInst(make_shared<GotoInst>(cur_func_ptr_->TopCondLabel()));
        break;
    case Keyword::Return:
        const auto &return_exp = ast.GetExpr();
        cur_func_ptr_->SetReturnTrue();
        if (return_exp != nullptr){
            auto val_ptr = return_exp->GenerateIR(*this);
            if (val_ptr->IsArrayElem()){
                auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
                cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
                cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, val_ptr));
                cur_func_ptr_->PushInst(make_shared<ReturnInst>(tp));
            }
            else
                cur_func_ptr_->PushInst(make_shared<ReturnInst>(val_ptr));
        }
        else
            cur_func_ptr_->PushInst(make_shared<ReturnInst>(nullptr));
    }
    return nullptr;
}

ValPtr IRGenerator::GenerateOn(LValAST &ast){
    auto val_ptr = GetVarVal(ast.GetIdent());
    const auto &index = ast.GetIndex();
    int index_num = index.size();
    if (index.empty()){
        // just a normal var or the initial address of an array
        return val_ptr;
    }
    // index
    // 变量的作用域
    auto pos = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
    //auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
    cur_func_ptr_->PushDecl(make_shared<VarDecInst>(pos, false));
    //cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
    // pos = 0
    cur_func_ptr_->PushInst(make_shared<AssignInst>(pos, make_shared<IntVal>(0)));
    // calculate pos
    vector<int> &acc_num = val_ptr->GetAccs();
    for (int i = 0; i < index_num; i++){
        auto cur_index_val_ptr = index[i]->GenerateIR(*this);
        // index is an integer?
        if (cur_index_val_ptr->IsArrayElem()){ // a[b[2]]
            auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
            cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
            cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, cur_index_val_ptr));
            cur_index_val_ptr = move(tp);
        }
        auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
        cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
        cur_func_ptr_->PushInst(make_shared<BinaryInst>(tp, cur_index_val_ptr, Operator::Mul,
                                                        make_shared<IntVal>(acc_num[i + 1])));
        cur_func_ptr_->PushInst(make_shared<BinaryInst>(pos, pos, Operator::Add, tp));
    }
    // pos = pos * 4
    cur_func_ptr_->PushInst(make_shared<BinaryInst>(pos, pos, Operator::Mul, make_shared<IntVal>(4)));
    if (val_ptr->GetDimNum() == index_num){
        // return a val
        return make_shared<ArrayElemVal>(val_ptr, pos);
    }
    else{
        // return an address
        // pos = a + pos
        cur_func_ptr_->PushInst(make_shared<BinaryInst>(pos, pos, Operator::Add, val_ptr));
        return pos;
    }
}

ValPtr IRGenerator::GenerateOn(IntAST &ast){
    return make_shared<IntVal>(ast.GetVal());
}

ValPtr IRGenerator::GenerateOn(UnaryExpAST &ast){
    auto val = ast.GetExpr()->GenerateIR(*this);
    ValPtr tp;
    switch(ast.GetOp()){                   // ! and -
        case Operator::Sub:
        tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
        cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
        cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, val));
        val = move(tp);       
        cur_func_ptr_->PushInst(make_shared<UnaryInst>(val, Operator::Sub, val));
        break;
        case Operator::Not:
        tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
        cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
        cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, val));
        val = move(tp);  
        cur_func_ptr_->PushInst(make_shared<UnaryInst>(val, Operator::Not, val));
        break;
    }
    return val;
}

ValPtr IRGenerator::GenerateOn(BinaryExpAST &ast)
{
    auto tmp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
    cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tmp, false));
    auto op = ast.GetOp();
    if (cur_func_ptr_->IsInCond() && (op == Operator::And || op == Operator::Or)){ //handle cond
        //auto this_rhs_label = make_shared<LabelVal>();
        //auto this_end_label = make_shared<LabelVal>();
        auto cur_branch_ = branch_stack_.back();
        auto last_true_label = cur_branch_->GetLabel_1();
        auto last_false_label = cur_branch_->GetLabel_2(); //last end label
        //auto new_branch = make_shared<CondBranch>(this_rhs_label, this_end_label);
        //branch_stack_.push_back(new_branch);
        auto new_label = make_shared<LabelVal>(); //new label
        if (op == Operator::And)
        {
            // new branch
            auto branch = make_shared<CondBranch>(new_label, last_false_label);
            branch_stack_.push_back(branch);

            auto lhs = ast.GetLhs()->GenerateIR(*this);
            branch_stack_.pop_back();

            if (lhs && lhs->IsArrayElem())
            {
                auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
                cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
                cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, lhs));
                lhs = move(tp);
            }
            if(lhs){
                auto lhs_inst = make_shared<IfInst>(lhs, last_false_label);
                lhs_inst->SetTrueLabel(new_label);
                cur_func_ptr_->PushInst(lhs_inst); //shortcuit
            }

            cur_func_ptr_->PushInst(make_shared<LabelInst>(new_label));

            //set rhs
            auto branch_2 = make_shared<CondBranch>(last_true_label, last_false_label);
            branch_stack_.push_back(branch_2);

            auto rhs = ast.GetRhs()->GenerateIR(*this);
            branch_stack_.pop_back();

            if (rhs && rhs->IsArrayElem())
            {
                auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
                cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
                cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, rhs));
                rhs = move(tp);
            }
            if(rhs){
                auto rhs_inst = make_shared<IfInst>(rhs, last_false_label);
                rhs_inst->SetTrueLabel(last_true_label);  
                cur_func_ptr_->PushInst(rhs_inst);              
            }

            //cur_func_ptr_->PushInst(make_shared<BinaryInst>(tmp, lhs, op, rhs));
        }
        else if (op == Operator::Or)
        {
            // new branch
            auto branch = make_shared<CondBranch>(last_true_label, new_label);
            branch_stack_.push_back(branch);

            auto lhs = ast.GetLhs()->GenerateIR(*this);
            branch_stack_.pop_back();

            if (lhs && lhs->IsArrayElem()){
                auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
                cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
                cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, lhs));
                lhs = move(tp);
            }
            //auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
            //cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
            //cur_func_ptr_->PushInst(make_shared<UnaryInst>(tp, Operator::Not, lhs));
            if(lhs){
                auto lhs_inst = make_shared<IfInst>(lhs, new_label);
                lhs_inst->SetTrueLabel(last_true_label);
                cur_func_ptr_->PushInst(lhs_inst);
            }

            cur_func_ptr_->PushInst(make_shared<LabelInst>(new_label));

            //set rhs
            auto branch_2 = make_shared<CondBranch>(last_true_label, last_false_label);
            branch_stack_.push_back(branch_2);

            auto rhs = ast.GetRhs()->GenerateIR(*this);
            branch_stack_.pop_back();
            if (rhs && rhs->IsArrayElem())
            {
                auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
                cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
                cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, rhs));
                rhs = move(tp);
            }
            if(rhs){
                auto rhs_inst = make_shared<IfInst>(rhs, last_false_label);
                rhs_inst->SetTrueLabel(last_true_label);
                cur_func_ptr_->PushInst(rhs_inst);
            }
            
            //cur_func_ptr_->PushInst(make_shared<BinaryInst>(tmp, lhs, op, rhs));
        }
        //cur_func_ptr_->PushInst(make_shared<LabelInst>(this_end_label)); //end
        //branch_stack_.pop_back();                                        //pop
        return nullptr;
    }
    else{
        auto lhs = ast.GetLhs()->GenerateIR(*this);
        auto rhs = ast.GetRhs()->GenerateIR(*this);
        if (op != Operator::Assign){
            // rval cant be array elem
            if (lhs->IsArrayElem()){
                auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
                cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
                cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, lhs));
                lhs = move(tp);
            }
            if (rhs->IsArrayElem()){
                auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
                cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
                cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, rhs));
                rhs = move(tp);
            }
            cur_func_ptr_->PushInst(make_shared<BinaryInst>(tmp, lhs, ast.GetOp(), rhs));
            return tmp;
            // += -= not handled yet
        }
        else{
            if (lhs->IsArrayElem() && rhs->IsArrayElem()){ //t[] = t[]
                auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
                cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
                cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, rhs));
                rhs = move(tp);
            }
            cur_func_ptr_->PushInst(make_shared<AssignInst>(lhs, rhs));
            return nullptr;
        }
    }
}

ValPtr IRGenerator::GenerateOn(FuncCallAST &ast){
    auto ident = ast.GetIdent();
    auto func_ptr = GetFuncPtr(ident); //get return type
    ValPtr func_val_ptr;
    if(ident == "starttime" || ident == "stoptime"){
        ident = "_sysy_" + ident;
        func_val_ptr = make_shared<FuncCallVal>(ident);
        auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
        cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
        auto line = make_shared<IntVal>(ast.GetLine());
        cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, line));
        auto para_val_list = ValPtrList();
        para_val_list.push_back(line);
        cur_func_ptr_->PushInst(make_shared<CallInst>(nullptr, func_val_ptr, para_val_list));
        return nullptr; //no dest
    }
    else{
        func_val_ptr = make_shared<FuncCallVal>(ident);
    }
    auto para_val_list = ValPtrList();
    for (const auto &para : ast.GetParas()){ //param
        auto val = para->GenerateIR(*this);
        if (val->IsArrayElem()){
            auto tp = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
            cur_func_ptr_->PushDecl(make_shared<VarDecInst>(tp, false));
            cur_func_ptr_->PushInst(make_shared<AssignInst>(tp, val));
            val = move(tp);
        }
        para_val_list.push_back(val);
    }
    if (func_ptr->GetRType() == Type::Int){
        auto dest = make_shared<VarVal>(VarType::t, cur_func_ptr_->AddtSlot());
        cur_func_ptr_->PushDecl(make_shared<VarDecInst>(dest, false));
        cur_func_ptr_->PushInst(make_shared<CallInst>(dest, func_val_ptr, para_val_list));
        return dest;
    }
    else{
        cur_func_ptr_->PushInst(make_shared<CallInst>(nullptr, func_val_ptr, para_val_list));
        return func_val_ptr;
    }
}
