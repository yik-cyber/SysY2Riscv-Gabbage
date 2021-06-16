#include "eeyore.hpp"
#include "tiggergen.hpp"
using namespace std;

int Block::next_block_id_ = 0;

#define LINE(ofs) \
    ofs << loc_ << " " << pos_;
#define SPACE(ofs) \
    ofs << "    "


void EFuncDefVal::AddLabeledBlock(int _label, BlockPtr _block){
    auto it = label2block_.find(_label);
    if(it != label2block_.end())
        assert(false);
    else
        label2block_[_label] = move(_block);
}

BlockPtr EFuncDefVal::GetLabeledBlock(int label) {
    auto it = label2block_.find(label);
    if(it == label2block_.end()){
        assert(false);
        return nullptr;
    }  
    else
        return it->second;
}

inline void DumpOp(ofstream& ofs, const Operator& op){
    switch(op){
        case Operator::Add:
            ofs << "+"; break;
        case Operator::Sub:
            ofs << "-"; break;
        case Operator::Mul:
            ofs << "*"; break;
        case Operator::Div:
            ofs << "/"; break;
        case Operator::Mod:
            ofs << "%"; break;
        case Operator::Eq:
            ofs << "=="; break;
        case Operator::Less:
            ofs << "<" ; break;
        case Operator::LessEq:
            ofs << "<="; break;
        case Operator::More:
            ofs << ">" ; break;
        case Operator::MoreEq:
            ofs << ">="; break;
        case Operator::Not:
            ofs << "!" ; break;
        case Operator::And:
            ofs << "&&"; break;
        case Operator::Or:
            ofs << "||"; break;
        case Operator::NotEq:
            ofs << "!="; break;
        default:
            assert(false);
    }
}

void EFuncDefInst::Dump(ofstream& ofs){
    LINE(ofs); 
    ofs << "f_";
    ofs << func_->GetIdent() << " [" << func_->GetParamNum() << "]\n";
}

void EEndInst::Dump(ofstream& ofs){
    LINE(ofs);
    ofs << "end f_" << func_->GetIdent() << "\n";
}

void EVarDecInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    ofs << "var ";
    if(is_array_){
        ofs << var_->GetDim() << " ";
    }
    var_->Dump(ofs);
    ofs << "\n";
}

void EAssignInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    dest_->Dump(ofs);
    ofs << " = ";
    val_->Dump(ofs);
    ofs << "\n";
}

void EIfInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    ofs << "if ";
    lhs_->Dump(ofs); ofs << " ";
    DumpOp(ofs, op_); ofs << " ";
    rhs_->Dump(ofs); ofs << " goto ";
    label_->Dump(ofs); ofs << "\n";
}

void EGotoInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    ofs << "goto ";
    label_->Dump(ofs); 
    ofs << "\n";
}

void ELabelInst::Dump(ofstream& ofs){
    LINE(ofs);
    label_->Dump(ofs);
    ofs << ":\n";
}

void ECallInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    if(dest_){
        dest_->Dump(ofs); ofs << " = ";
    }
    ofs << "call ";
    func_->Dump(ofs);
    ofs << "\n";
}

void EParamInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    ofs << "param ";
    param_->Dump(ofs);
    ofs << "\n";
}

void EReturnInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    ofs << "return ";
    if(val_){
        val_->Dump(ofs);
    }
    ofs << "\n";
}

void EBinaryInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    dest_->Dump(ofs); ofs << " = ";
    lhs_->Dump(ofs); ofs << " ";
    DumpOp(ofs, op_); ofs << " ";
    rhs_->Dump(ofs);
    ofs << "\n";
}

void EUnaryInst::Dump(ofstream& ofs){
    LINE(ofs); SPACE(ofs);
    dest_->Dump(ofs); ofs << " = ";
    DumpOp(ofs, op_);
    rhs_->Dump(ofs);
    ofs << "\n";
}

void EVarVal::Dump(ofstream& ofs){
    ofs << type_;
    ofs << id_;
    if(interval_ != nullptr){
        interval_->Dump(ofs);
    }
}

void EVarVal::TDump(ofstream& ofs){
    ofs << "v";
    ofs << id_;
}

void EArrayElemVal::Dump(ofstream& ofs){
    addr_->Dump(ofs);
    ofs << "["; index_->Dump(ofs); ofs << "]";
}

void EArrayVal::Dump(ofstream& ofs){
    ofs << type_;
    ofs << id_; 
    if(interval_ != nullptr){
        interval_->Dump(ofs);
    }  
}

void EArrayVal::TDump(ofstream& ofs){
    ofs << "v";
    ofs << id_;
}

void ELabelVal::Dump(ofstream& ofs){
    ofs << "l" << id_;
}

void EIntVal::Dump(ofstream& ofs){
    ofs << val_;
}

void EFuncDefVal::Dump(ofstream& ofs){
    ofs << "f_" << ident_;
}

void EFuncDefVal::DumpBlock(ofstream& ofs){
    for(const auto& block: blocks_){
        block->Dump(ofs);
    }
}

void ERegVal::Dump(ofstream& ofs){
    ofs << reg_;
    if(interval_){
        interval_->Dump(ofs);
    }
}

void EFuncDefVal::BuildBlockGragh(){
    for(auto block: blocks_){
        auto inst = block->GetLastInst();
        if(inst == nullptr)
            continue;
        if(inst->IsBranch()){
            auto branch_block = GetLabeledBlock(inst->GetGotoLabelID());
            if(branch_block->GetBlockID() <= block->GetBlockID()){
                block->SetBackward(branch_block);
                block->AddSucs(branch_block);
                branch_block->AddPred(block);
            }
            else{
                block->SetForward(branch_block);
                block->AddSucs(branch_block);
                branch_block->AddPred(block);
                // branch可由block向下跳转到达，incoming前驱数+1
                branch_block->IncForwardBlockNum();
            }
        }
    }
}


void Block::Dump(ofstream& ofs){
    /*
    ofs << "***block: " << block_id_ << "***\n";
    ofs << "***loop index: " << loop_index_ << "***\n***loop index set:";
    for(const auto& index: loop_indexes_){
        ofs << index << " ";
    }
    ofs << "***\n";
    ofs << "***loop depth: " << loop_depth_ << "***\n";
    */
    for(const auto& inst: inst_){
        inst->Dump(ofs);
    }
    ofs << "\n";
}


EValPair EAssignInst::VisitOpr(){
    vector<EValPtr> defs;
    vector<EValPtr> uses;
    if(dest_->IsArrayElem()){
        uses.push_back(dest_->GetAddr());
        uses.push_back(dest_->GetIndex());
        uses.push_back(val_);
        return {defs, uses};
    }
    if(val_->IsArrayElem()){
        uses.push_back(val_->GetAddr());
        uses.push_back(val_->GetIndex());
        defs.push_back(dest_);
        return {defs, uses};
    }
    else{
        uses.push_back(val_);
        defs.push_back(dest_);
        return {defs, uses};
    }
}

EValPair EIfInst::VisitOpr(){
    vector<EValPtr> defs;
    vector<EValPtr> uses;
    uses.push_back(lhs_);
    uses.push_back(rhs_);
    return {defs, uses};
}

EValPair ECallInst::VisitOpr(){
    //assert(dest_ == nullptr);
    vector<EValPtr> defs;
    vector<EValPtr> uses;
    if(dest_){
        defs.push_back(dest_);
    }
    for(auto param: vals_){
        uses.push_back(param);
    }
    return {defs, uses};
}

EValPair EParamInst::VisitOpr(){
    vector<EValPtr> defs;
    vector<EValPtr> uses;
    uses.push_back(param_);
    return {defs, uses};
}

EValPair EReturnInst::VisitOpr(){
    vector<EValPtr> defs;
    vector<EValPtr> uses;
    if(val_ != nullptr){
        uses.push_back(val_);
    }
    return {defs, uses};
}

EValPair EBinaryInst::VisitOpr(){
    vector<EValPtr> defs;
    vector<EValPtr> uses;
    defs.push_back(dest_);
    uses.push_back(lhs_);
    uses.push_back(rhs_);
    return {defs, uses};
}

EValPair EUnaryInst::VisitOpr(){
    vector<EValPtr> defs;
    vector<EValPtr> uses;
    defs.push_back(dest_);
    uses.push_back(rhs_);
    return {defs, uses};
}


void EValueBase::Resolving(bool _is_use, int _pos, BlockPtr block, BlockPtr backward_block){
    if(backward_block == nullptr ||val_type_ == EValType::Int ||  
       interval_ == nullptr || interval_->spill_store_pos_ == 2e8)
        return;
    if(interval_->spill_store_pos_ > block->to_ && interval_->to_ >= backward_block->from_){
        //这里还是得想办法弄一哈
        interval_->SetSpillStoreUse(_pos, _is_use);
        if(interval_->spill_stack_ == -1){
            interval_->spill_stack_ = block->func_->GetStackSize();
            block->func_->AddStackSize(1);
        }
    }      
}

void EArrayElemVal::Resolving(bool _is_use, int _pos, BlockPtr block, BlockPtr backward_block){
    addr_->Resolving(true, _pos, block, backward_block);
    index_->Resolving(true, _pos, block, backward_block);
}


void EAssignInst::Resolving(BlockPtr block, BlockPtr backward_block){
    dest_->Resolving(false, pos_, block, backward_block);
    val_->Resolving(true, pos_, block, backward_block);
}

void EIfInst::Resolving(BlockPtr block, BlockPtr backward_block){
    lhs_->Resolving(true, pos_,block, backward_block);
    rhs_->Resolving(true, pos_,block, backward_block);
}

void EParamInst::Resolving(BlockPtr block, BlockPtr backward_block){
    param_->Resolving(true, pos_,block, backward_block);
}

void ECallInst::Resolving(BlockPtr block, BlockPtr backward_block){
    for(auto val: vals_){
        val->Resolving(true, pos_, block, backward_block);
    }
}

void EReturnInst::Resolving(BlockPtr block, BlockPtr backward_block){
    return; //return 不会进行到后续block，直接结束
}

void EBinaryInst::Resolving(BlockPtr block, BlockPtr backward_block){
    lhs_->Resolving(true, pos_,block, backward_block);
    rhs_->Resolving(true, pos_,block, backward_block);
    dest_->Resolving(false, pos_,block, backward_block);
}

void EUnaryInst::Resolving(BlockPtr block, BlockPtr backward_block){
    rhs_->Resolving(true, pos_, block, backward_block);
    dest_->Resolving(false, pos_, block, backward_block);
}

void EFuncDefInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenFuncDef(*this);
}

void EEndInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenEnd(*this);
}

void EVarDecInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenVarDec(*this);
}

void EAssignInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenAssign(*this);
}

void EIfInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenIf(*this);
}

void EGotoInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenGoto(*this);
}

void ELabelInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenLabel(*this);
}

void ECallInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenFuncCall(*this);
}

void EParamInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenParam(*this);
}

void EReturnInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenReturn(*this);
}

void EBinaryInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenBinary(*this);
}

void EUnaryInst::GenerateTigger(TiggerGenerator& tgen){
    tgen.GenUnary(*this);
}