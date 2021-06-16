#include "regalloc.hpp"
#include "tiggergen.hpp"
using namespace std;

string real_reg_names2[] = {
        "a0", "a1", "a2", "a3", "a4", "a5", "a6",
        "a7",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", //15

        "s0", "s1", "s2", "s3", "s4", "s5", "s6",
        "s7", "s8",
};

string reserve_reg_names2[] = {
        "s9", "s10", "s11",
        "x0",
};

void TiggerGenerator::BuildReg(){
    for(auto reg_name: real_reg_names2){
        auto reg = make_shared<RegOpr>();
        reg->ident_ = reg_name;
        reg_map_[reg_name] = reg;
    }
    for(auto reg_name: reserve_reg_names2){
        auto reg = make_shared<RegOpr>();
        reg->ident_ =reg_name;
        rsv_reg_.push_back(reg);
        reg_map_[reg_name] = reg;
    }
}

// s9 s10 s11
OprPtr TiggerGenerator::GetUseReg(EValPtr var, int _pos, int _loc){
    if(var->val_type_ == EValType::Int){
        auto num = make_shared<NumOpr>(var->GetConstVal());
        if(is_if_){
            if(var->GetConstVal() == 0){
                return reg_map_["x0"];
            }
            else{
                auto reg = rsv_reg_[_loc];
                inst_.push_back(make_shared<TAssignInst>(reg, num));
                return reg;
            }
        }
        return num;
    }
    else if(var->is_global_){
        //全局变量用的时候直接加载
        auto reg = rsv_reg_[_loc];
        if(var->IsArray())
            inst_.push_back(make_shared<TLoadAddrGInst>(reg, var));
        else
            inst_.push_back(make_shared<TLoadGInst>(reg, var));

        return reg;     
    }
    //只可能是原生变量或者局部变量
    auto _in = var->interval_;
    if(_in->spill_use_pos_ <= _pos){
        //溢出
        auto reg = rsv_reg_[_loc];
        if(var->IsArray())
                inst_.push_back(make_shared<TLoadAddrSInst>(reg, var->GetStackPos()));
        else
                inst_.push_back(make_shared<TLoadSInst>(reg, _in->spill_stack_));
        return reg;
    }
    else{
        auto reg = reg_map_[_in->reg_->ident_];
        if(var->IsArray()){
            //数组的from一定是0
            if(!var->loaded_){
                var->loaded_ = true;
                //一个trick，直接+1，不影响后面
                _in->spill_stack_ = var->GetStackPos();
                inst_.push_back(make_shared<TLoadAddrSInst>(reg, _in->spill_stack_));                
            }
       }
       if(_in->spill_store_pos_ == _pos - 1){
           //prestore
           inst_.push_back(make_shared<TStoreInst>(reg, _in->spill_stack_));
       }
       return reg;
    }
}

OprPtr TiggerGenerator::GetDefReg(EValPtr var, int _pos,  int _loc){
    if(var->is_global_){
        auto reg = rsv_reg_[_loc];
        //inst_.push_back(make_shared<TLoadGInst>(reg, var));
        return reg;
    }
    auto _in = var->interval_;
    if(_in->spill_use_pos_ <= _pos){
        auto reg = rsv_reg_[_loc];
        return reg;
    }
    else{
        auto reg = reg_map_[_in->reg_->ident_];
        return reg;
    }
}



void TiggerGenerator::GenGlobal(){
    // only declarations
    for(auto inst: gm_.GetGlobalInst()){
        assert(inst->type_ == EInstType::VarDec);
        auto var = inst->GetVar();
        assert(var->is_global_);
        ginst_.push_back(make_shared<TVarDecInst>(var));
    }
}

void TiggerGenerator::GenVarDec(EVarDecInst& inst){
    return; 
}

void TiggerGenerator::GenFuncDef(EFuncDefInst& inst){
    auto func = inst.GetFunc();
    stk_ = (func->GetStackSize() / 4 + 1) * 16;
    inst_.push_back(make_shared<TFuncDefInst>(func, stk_));
    int store_pos = func->reserved_reg_stack_;
    for(auto reg: func->reserved_regs){
        inst_.push_back(make_shared<TStoreInst>(reg_map_[reg->ident_], store_pos++));
    }
}

void TiggerGenerator::GenEnd(EEndInst& inst){
    auto func = inst.GetFunc();
    /*
    int store_pos = func->reserved_reg_stack_;
    for(auto reg: func->reserved_regs){
        inst_.push_back(make_shared<TLoadSInst>(reg_map_[reg->ident_], store_pos++));
    }
    */
    inst_.push_back(make_shared<TEndInst>(func));

}


void TiggerGenerator::GenAssign(EAssignInst& inst){
    int pos = inst.pos_;
    auto dest = inst.GetDest();
    auto val = inst.GetVal();
    if(val->IsArrayElem()){ // s10 = s10[s11] 
        auto addr = val->GetAddr();
        auto addr_reg = GetUseReg(addr, inst.pos_, 1);
        auto index = val->GetIndex();
        auto index_reg = GetUseReg(index, inst.pos_, 2);
        auto dest_reg = GetDefReg(dest, inst.pos_, 1);
        inst_.push_back(make_shared<TBinaryInst>(dest_reg, addr_reg, Operator::Add, index_reg));
        inst_.push_back(make_shared<TAssignInst>(dest_reg, dest_reg, 1));
        // when store the value
        StoreVal(dest, inst.pos_, dest_reg);
    }
    else if(dest->IsArrayElem()){ //use
        auto addr = dest->GetAddr();
        auto addr_reg = GetUseReg(addr, inst.pos_, 1);
        auto index = dest->GetIndex();
        auto index_reg = GetUseReg(index, inst.pos_, 2);
        // t1 = t1 + t2
        inst_.push_back(make_shared<TBinaryInst>(rsv_reg_[1], addr_reg, Operator::Add, index_reg));
        if(val->val_type_ == EValType::Int){
            // t1[0] = 0
            auto num = make_shared<NumOpr>(val->GetConstVal());
            OprPtr reg;
            if(num->num_ == 0){
                reg = reg_map_["x0"];
            }
            else{
                reg = rsv_reg_[2];
                inst_.push_back(make_shared<TAssignInst>(reg, num));
            }
            inst_.push_back(make_shared<TAssignInst>(rsv_reg_[1], reg, 0));
        }
        else{
            // t1[0] = t2  
            auto val_reg = GetUseReg(val, inst.pos_, 2);
            inst_.push_back(make_shared<TAssignInst>(rsv_reg_[1], val_reg, 0));
        }    
        //赋值的是数组不用额外存储
    }
    else{
        if(val->val_type_ == EValType::Int){
            // t1 = 0
            auto num = make_shared<NumOpr>(val->GetConstVal());
            auto dest_reg = GetDefReg(dest, inst.pos_, 1);
            inst_.push_back(make_shared<TAssignInst>(dest_reg, num, -1));
            StoreVal(dest, inst.pos_, dest_reg);
        }
        else{
            // t1 = t2
            auto val_reg = GetUseReg(val, inst.pos_, 2);
            auto dest_reg = GetDefReg(dest, inst.pos_, 1);
            inst_.push_back(make_shared<TAssignInst>(dest_reg, val_reg, -1));
            StoreVal(dest, inst.pos_, dest_reg);
        }
    }
}

void TiggerGenerator::GenIf(EIfInst& inst){
    is_if_ = true;
    auto lhs = inst.GetLhs();
    auto rhs = inst.GetRhs();
    auto op = inst.GetOp();
    auto label = inst.GetLabel();
    auto lhs_reg = GetUseReg(lhs, inst.pos_, 1);
    auto rhs_reg = GetUseReg(rhs, inst.pos_, 2);
    inst_.push_back(make_shared<TIfInst>(lhs_reg, op, rhs_reg, label));
    is_if_ = false;
}

void TiggerGenerator::GenGoto(EGotoInst& inst){
    auto label = inst.GetLabel();
    inst_.push_back(make_shared<TGotoInst>(label));
}

void TiggerGenerator::GenLabel(ELabelInst& inst){
    auto label = inst.GetLabel();
    inst_.push_back(make_shared<TLabelInst>(label));
}

void TiggerGenerator::PreParam(ECallInst& inst, int num){
    int start = param_stack_;
    for(auto param: inst.params_){
        auto val = param->GetParam();
        auto val_reg = GetUseReg(val, inst.pos_, 1);
        if(val_reg->ident_[0] == 'a' && val_reg->ident_[1] - '0' < num){
            inst_.push_back(make_shared<TStoreInst>(val_reg, start));
            param->SetStackPos(start);
            start++;
        }
    }
}

void TiggerGenerator::GenFuncCall(ECallInst& inst){
    //由于全局变量不给分配寄存器，所以不用管了
    auto func = inst.GetFunc();
    int num = func->GetParamNum();
    int store_pos = inst.reserve_pos_;
    for(auto reg: inst.reserve_regs_){
        inst_.push_back(make_shared<TStoreInst>(reg_map_[reg->ident_], store_pos++));
    }
    PreParam(inst, num);    
    for(auto param: inst.params_){
        auto val = param->GetParam();
        if(param->GetStackPos() >= 0){
            inst_.push_back(make_shared<TLoadSInst>(reg_map_[param->GetReg()], param->GetStackPos()));
        }
        else{
            auto val_reg = GetUseReg(val, inst.pos_, 1);
            inst_.push_back(make_shared<TAssignInst>(reg_map_[param->GetReg()], val_reg));            
        }
    }
    inst_.push_back(make_shared<TCallInst>(func));
    if(inst.dest_){
        auto dest_reg = GetDefReg(inst.dest_, inst.pos_, 1);
        inst_.push_back(make_shared<TAssignInst>(dest_reg, reg_map_["a0"]));
    }
    store_pos = inst.reserve_pos_;
    for(auto reg: inst.reserve_regs_){
        inst_.push_back(make_shared<TLoadSInst>(reg_map_[reg->ident_], store_pos++));
    }
}

void TiggerGenerator::GenParam(EParamInst& inst){
    return; 
}

void TiggerGenerator::GenReturn(EReturnInst& inst){
    auto func = inst.GetFunc();
    int store_pos = func->reserved_reg_stack_;
    if(inst.val_){
        auto reg = GetUseReg(inst.val_, inst.pos_, 1);
        inst_.push_back(make_shared<TAssignInst>(reg_map_["a0"], reg));
    }    
    for(auto reg: func->reserved_regs){
        inst_.push_back(make_shared<TLoadSInst>(reg_map_[reg->ident_], store_pos++));
    }    
    inst_.push_back(make_shared<TReturnInst>(stk_));
}

void TiggerGenerator::GenBinary(EBinaryInst& inst){
    auto lhs = inst.GetLhs();
    auto rhs = inst.GetRhs();
    auto op = inst.GetOp();
    auto dest = inst.GetDest();
    auto lhs_reg = GetUseReg(lhs, inst.pos_, 1);
    auto rhs_reg = GetUseReg(rhs, inst.pos_, 2);
    auto dest_reg = GetDefReg(dest, inst.pos_, 1);
    if(lhs->val_type_ == EValType::Int){
        if(op == Operator::Sub){
            //t0 = 3 - t1
            inst_.push_back(make_shared<TBinaryInst>(dest_reg, rhs_reg, op, lhs_reg));
            inst_.push_back(make_shared<TUnaryInst>(dest_reg, op, dest_reg));
        }
        else{
            inst_.push_back(make_shared<TBinaryInst>(dest_reg, rhs_reg, op, lhs_reg));
        }
    }
    else{
        inst_.push_back(make_shared<TBinaryInst>(dest_reg, lhs_reg, op, rhs_reg));
    }
    StoreVal(dest, inst.pos_, dest_reg);
}

void TiggerGenerator::GenUnary(EUnaryInst& inst){
    auto rhs = inst.GetRhs();
    auto op = inst.GetOp();
    auto dest = inst.GetDest();
    auto rhs_reg = GetUseReg(rhs, inst.pos_, 1);
    auto dest_reg = GetUseReg(dest, inst.pos_, 2);
    inst_.push_back(make_shared<TUnaryInst>(dest_reg, op, rhs_reg));
    StoreVal(dest, inst.pos_, dest_reg);
}

void TiggerGenerator::StoreVal(EValPtr var, int _pos, OprPtr _reg){
    //
    _pos += 2;
    // 全局变量
    if(var->is_global_){
        inst_.push_back(make_shared<TLoadAddrGInst>(rsv_reg_[2], var));
        inst_.push_back(make_shared<TAssignInst>(rsv_reg_[2], _reg, 0));
    } 
    //局部和原生变量
    else{
        auto _in = var->interval_;
        if(_in->spill_store_pos_ <= _pos)
            inst_.push_back(make_shared<TStoreInst>(_reg, _in->spill_stack_));
    }
    return;
}

void TiggerGenerator::GenerateOn(){
    BuildReg();
    GenGlobal();
    for(auto func: fc_.funcs_){
        cur_func_ = func;
        param_stack_ = cur_func_->GetStackSize();
        cur_func_->AddStackSize(8);
        for(auto block: func->blocks_){
            for(auto inst: block->inst_){
                inst->GenerateTigger(*this);
            }
        }
    }
}

void TiggerGenerator::Dump(ofstream& ofs){
    for(auto inst: ginst_){
        inst->Dump(ofs);
    }
    for(auto inst: inst_){
        inst->Dump(ofs);
    }
}

void TiggerGenerator::DumpRiscv(ofstream& ofs){
    for(auto inst: ginst_){
        inst->DumpRiscv(ofs);
    }
    for(auto inst: inst_){
        inst->DumpRiscv(ofs);
    }    
}