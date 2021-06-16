#include "ir.hpp"
using namespace std;

int LabelVal::next_id_ = 0;
int FuncDef::global_T_slot_id = 0;

#define OUTPUT_TYEPE \
    switch(type_){ \
        case VarType::p:\
            ofs << "p"; break;\
        case VarType::T:\
            ofs << "T"; break;\
        case VarType::t:\
            ofs << "t"; break;\
        default:\
            assert(false);\
    }

inline void print_op(stringstream& ofs, const Operator& op){
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
        case Operator::Assign:
            ofs << "="; break;
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
        case Operator::NotEq:
            ofs << "!="; break;
        case Operator::Not:
            ofs << "!" ; break;
        case Operator::And:
            ofs << "&&"; break;
        case Operator::Or:
            ofs << "||"; break;
        default:
            assert(false);
    }
}

void FuncDef::Dump(stringstream& ofs){

        ofs << "f_" << ident_ << " ";
        ofs << "[" << arg_num_ << "]\n";

        if(global_){
            for(const auto& inst: global_->GetDeclInst()){
                inst->Dump(ofs);
            }
        }
        for(const auto& inst: decl_insts_){
            inst->Dump(ofs);
        }
        if(global_){
            for(const auto inst: global_->GetInst()){
                inst->Dump(ofs);
            }
        }
        for(const auto& inst: insts_){
            inst->Dump(ofs);
        }
        ofs << "end f_" << ident_ << "\n";        
}



void VarDecInst::Dump(stringstream& ofs){
    ofs << "    ";
    ofs << "var ";
    if(is_array_){
        ofs << var_->GetTotDimNum() * 4 << " ";
    }
    var_->Dump(ofs);
    ofs << "\n";
}

void AssignInst::Dump(stringstream& ofs){
    ofs << "    ";
    dest_->Dump(ofs);
    ofs << " = ";
    val_->Dump(ofs);
    ofs << "\n";
}

void IfInst::Dump(stringstream& ofs){
    ofs << "    ";
    ofs << "if ";
    cond_->Dump(ofs); ofs << " == 0";
    ofs << " goto ";
    false_label_->Dump(ofs);
    ofs << "\n";
    if(true_label_){
        ofs << "    goto ";
        true_label_->Dump(ofs);
    }
}

void GotoInst::Dump(stringstream& ofs){
    ofs << "    ";
    ofs << "goto ";
    label_->Dump(ofs);
    ofs << "\n";
}

void LabelInst::Dump(stringstream& ofs){
    label_->Dump(ofs);
    ofs << ":";
    ofs << "\n";
}

void CallInst::Dump(stringstream& ofs){
    for(const auto& para: params_){
        ofs << "    ";
        ofs << "param ";
        para->Dump(ofs);
        ofs << "\n";
    }
    ofs << "    ";
    if(dest_){
        dest_->Dump(ofs);
        ofs << " = ";
    }
    ofs << "call ";
    func_->Dump(ofs);
    ofs << "\n";
}

void ReturnInst::Dump(stringstream& ofs){
    ofs << "    ";
    ofs << "return";
    if(val_){
        ofs << " ";
        val_->Dump(ofs);
    }
    ofs << "\n";
}

void BinaryInst::Dump(stringstream& ofs){
    ofs << "    ";
    dest_->Dump(ofs);
    ofs << " = ";
    lhs_->Dump(ofs); ofs << " ";
    print_op(ofs, op_); ofs << " ";
    rhs_->Dump(ofs);
    ofs << "\n";
}

void UnaryInst::Dump(stringstream& ofs){
    ofs << "    ";
    dest_->Dump(ofs);
    ofs << " = ";
    print_op(ofs, op_);
    rhs_->Dump(ofs);
    ofs << "\n";
}

void VarVal::Dump(stringstream& ofs){
    OUTPUT_TYEPE;
    ofs << id_;
}


void ArrayElemVal::Dump(stringstream& ofs){
    addr_->Dump(ofs);
    ofs << "[";
    index_->Dump(ofs);  
    ofs << "]"; 
}

void ArrayVal::Dump(stringstream& ofs){
    OUTPUT_TYEPE;
    ofs << id_;
}

void FuncCallVal::Dump(stringstream& ofs){
    ofs << "f_" << ident_;
}

void LabelVal::Dump(stringstream& ofs){
    ofs << "l" << id_;
}

void IntVal::Dump(stringstream& ofs){
    ofs << val_;
}