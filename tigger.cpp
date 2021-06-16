#include "tigger.hpp"
using namespace std;

#define SPACE(ofs) \
    ofs << "    "

#define ALIGN(ofs) \
    ofs << "  "

const int mini16 = -(1 << 15);
const int maxi16 = (1 << 15) - 1;
const int mini12 = -(1 << 12);
const int maxi12 = (1 << 12) - 1;
const int mini10 = -(1 << 9);
const int maxi10 = (1 << 9) - 1;

auto s9 = make_shared<RegOpr>("s9");


inline void TDumpOp(ofstream& ofs, const Operator& op){
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


void NumOpr::Dump(ofstream& ofs){
    ofs << num_;
}

void RegOpr::Dump(ofstream& ofs){
    ofs << ident_;
}

void TFuncDefInst::Dump(ofstream& ofs){
    ofs << "f_" << func_->GetIdent() << " [" << func_->GetParamNum() << "] ";
    ofs << " [" << func_->GetStackSize() << "] \n";
}

void TFuncDefInst::DumpRiscv(ofstream& ofs){
    ofs << "  .text\n";
    ofs << "  .align 2\n";
    ofs << "  .global " << func_->GetIdent()  << "\n";
    ofs << "  .type " << func_->GetIdent() << ", @function\n";
    ofs << func_->GetIdent() << ":\n";
    if(stk_ < mini12 || stk_ > maxi12){
        ALIGN(ofs);
        ofs << "li s9, " << -stk_ << "\n";
        ALIGN(ofs);
        ofs << "add sp, sp, s9\n";
    }
    else{
        ALIGN(ofs);
        ofs << "addi    sp, sp, " << -stk_ << "\n";
    }
    int tmp = stk_ - 4;
    if(tmp > maxi12 || tmp < mini12){
        ALIGN(ofs);
        ofs << "li s9, " << tmp << "\n";
        ALIGN(ofs);
        ofs << "add s9, sp, s9\n";
        ALIGN(ofs);
        ofs << "sw ra, (s9)\n";
    }
    else{
        ALIGN(ofs);
        ofs << "sw ra, " << tmp << "(sp)\n";
    }
}

void TEndInst::Dump(ofstream& ofs){
    ofs << "end f_" << func_->GetIdent() << "\n"; 
}

void TEndInst::DumpRiscv(ofstream& ofs){
    ALIGN(ofs);
    ofs << ".size   " << func_->GetIdent() << ", .-" << func_->GetIdent() << "\n";
}

void TVarDecInst::Dump(ofstream& ofs){
    var_->TDump(ofs);
    assert(var_->is_global_);
    if(var_->IsArray()){
        ofs << " = malloc " << var_->GetDim() << "\n";
    } 
    else{
        ofs << " = 0\n";
    }
}

void TVarDecInst::DumpRiscv(ofstream& ofs){
    if(var_->IsArray()){
        ALIGN(ofs);
        ofs << ".comm ";var_->TDump(ofs); ofs << ", " << var_->GetDim() <<", 4\n";
        return;
    }
    ALIGN(ofs);
    ofs << ".global   "; var_->TDump(ofs); ofs << "\n";
    ALIGN(ofs);
    ofs << ".section  .sdata\n";
    ALIGN(ofs);
    ofs << ".align    2\n";
    ALIGN(ofs);
    ofs << ".type     "; var_->TDump(ofs); ofs << ", @object\n";
    ALIGN(ofs);
    ofs << ".size     "; var_->TDump(ofs); ofs << ", 4\n";
    var_->TDump(ofs); ofs << ":\n";
    ALIGN(ofs);
    ofs << ".word     0\n";
}

void TAssignInst::Dump(ofstream& ofs){
    SPACE(ofs);
    if(array_ < 0){
        dest_->Dump(ofs);
        ofs << " = ";
        val_->Dump(ofs);
    }
    else if(array_ == 0){
        dest_->Dump(ofs); ofs << "[0] = ";
        val_->Dump(ofs);
    }
    else if(array_ == 1){
        dest_->Dump(ofs); ofs << " = ";
        val_->Dump(ofs); ofs << "[0]";
    }
    ofs << "\n";
}

void TAssignInst::DumpRiscv(ofstream& ofs){
    if(array_ < 0 && dest_ == val_)
        return;
    ALIGN(ofs);
    if(array_ < 0){
        if(val_->type_ == OprType::Num){
            ofs << "li "; dest_->Dump(ofs); ofs << ", "; val_->Dump(ofs);
        }
        else{
            ofs << "mv "; dest_->Dump(ofs); ofs << ", "; val_->Dump(ofs);
        }
    }
    else if(array_ == 0){
        ofs << "sw "; val_->Dump(ofs); ofs << ", ("; dest_->Dump(ofs); ofs << ")";
    }
    else if(array_ == 1){
        ofs << "lw "; dest_->Dump(ofs); ofs << ", ("; val_->Dump(ofs); ofs << ")";
    }
    ofs << "\n";
}

void TIfInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "if ";
    lhs_->Dump(ofs); ofs << " ";
    TDumpOp(ofs, op_); ofs << " ";
    rhs_->Dump(ofs);
    ofs << " goto ";
    label_->Dump(ofs);
    ofs << "\n";
}

void TIfInst::DumpRiscv(ofstream& ofs){
    ALIGN(ofs);
    switch(op_){
        case Operator::Less:
            ofs << "blt"; break;
        case Operator::More:
            ofs << "bgt"; break;
        case Operator::LessEq:
            ofs << "ble"; break;
        case Operator::MoreEq:
            ofs << "bge"; break;
        case Operator::NotEq:
            ofs << "bne"; break;
        case Operator::Eq:
            ofs << "beq"; break;
    }
    ofs << " ";
    lhs_->Dump(ofs); ofs << ", ";
    rhs_->Dump(ofs); ofs << ", .";
    label_->Dump(ofs);
    ofs << "\n";
}

void TGotoInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "goto ";
    label_->Dump(ofs);
    ofs << "\n";
}

void TGotoInst::DumpRiscv(ofstream& ofs){
    ALIGN(ofs);
    ofs << "j .";
    label_->Dump(ofs);
    ofs << "\n";
}

void TLabelInst::Dump(ofstream& ofs){
    label_->Dump(ofs);
    ofs << ":\n";
}

void TLabelInst::DumpRiscv(ofstream& ofs){
    ofs << ".";
    label_->Dump(ofs);
    ofs << ":\n";
}

void TCallInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "call f_" << func_->GetIdent() << "\n";
}

void TCallInst::DumpRiscv(ofstream& ofs){
    ALIGN(ofs);
    ofs << "call " << func_->GetIdent();
    ofs << "\n";
}

void TReturnInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "return\n";
}

void TReturnInst::DumpRiscv(ofstream& ofs){
    int tmp = stk_ - 4;
    if(tmp > maxi12 || tmp < mini12){
        ALIGN(ofs);
        ofs << "li s9, " << tmp << "\n";
        ALIGN(ofs);
        ofs << "add s9, sp, s9\n";
        ALIGN(ofs);
        ofs << "lw ra, (s9)\n";
    }
    else{
        ALIGN(ofs);
        ofs << "lw ra, " << tmp << "(sp)\n";
    }
    if(stk_ < mini12 || stk_ > maxi12){
        ALIGN(ofs);
        ofs << "li s9, " << stk_ << "\n";
        ALIGN(ofs);
        ofs << "add sp, sp, s9\n";
    }
    else{
        ALIGN(ofs);
        ofs << "addi sp, sp, " << stk_ << "\n";
    }

    ALIGN(ofs);
    ofs << "ret\n";
}

void TBinaryInst::Dump(ofstream& ofs){
    SPACE(ofs);
    dest_->Dump(ofs);
    ofs << " = ";
    lhs_->Dump(ofs); ofs << " ";
    TDumpOp(ofs, op_); ofs << " ";
    rhs_->Dump(ofs); ofs << "\n";
}

void TBinaryInst::DumpRiscv(ofstream& ofs){
    
    if(rhs_->type_ == OprType::Num){
        int num = rhs_->GetNum();
        if(num < mini12 || num > maxi12){
            ALIGN(ofs);
            ofs << "li s9, " << num << "\n";
            rhs_ = s9;
            goto flag;
        }
        else if(op_ == Operator::Add){
            ALIGN(ofs);
            ofs << "addi "; dest_->Dump(ofs); ofs << ", "; lhs_->Dump(ofs); ofs << ", " << num << "\n";
        }
        else if(op_ == Operator::Less){
            ALIGN(ofs);
            ofs << "slti "; dest_->Dump(ofs); ofs << ", "; lhs_->Dump(ofs); ofs << ", "; rhs_->Dump(ofs);
            ofs << "\n";
        }
        else{
            ALIGN(ofs);
            ofs << "li s9, " << num << "\n";
            rhs_ = s9;
            goto flag;
        }
    }
    else{
        flag:
        ALIGN(ofs);
        switch (op_){
            case Operator::Add:
                ofs << "add "; break;
            case Operator::Sub:
                ofs << "sub "; break;
            case Operator::Mul:
                ofs << "mul "; break;
            case Operator::Div:
                ofs << "div "; break;
            case Operator::Mod:
                ofs << "rem "; break;
            case Operator::MoreEq:
            case Operator::Less:
                ofs << "slt "; break;
            case Operator::LessEq:
            case Operator::More:
                ofs << "sgt "; break;
            case Operator::And:
                ofs << "snez "; break;
            case Operator::Or:
                ofs << "or "; break;
            case Operator::NotEq:
            case Operator::Eq:
                ofs << "xor "; break;
        }
        if(op_ == Operator::And){
            dest_->Dump(ofs); ofs << ", "; lhs_->Dump(ofs); ofs << "\n";
            ALIGN(ofs);
            ofs << "snez s9, "; rhs_->Dump(ofs); ofs << "\n";
            ALIGN(ofs);
            ofs << "and "; dest_->Dump(ofs); ofs << ", "; dest_->Dump(ofs); ofs << ", s9\n";
            return;
        }
        dest_->Dump(ofs); ofs << ", "; lhs_->Dump(ofs); ofs << ", "; rhs_->Dump(ofs); ofs << "\n";
        if(op_ == Operator::Or || op_ == Operator::NotEq){
            ALIGN(ofs);
            ofs << "snez "; dest_->Dump(ofs); ofs << ", "; dest_->Dump(ofs);
        }
        else if(op_ == Operator::Eq || op_ == Operator::LessEq || op_ == Operator::MoreEq){
            ALIGN(ofs);
            ofs << "seqz "; dest_->Dump(ofs); ofs << ", "; dest_->Dump(ofs); 
        }
        ofs << "\n";
    }
}

void TUnaryInst::Dump(ofstream& ofs){
    SPACE(ofs);
    dest_->Dump(ofs);
    ofs << " = ";
    TDumpOp(ofs, op_);
    rhs_->Dump(ofs);
    ofs << "\n";
}

void TUnaryInst::DumpRiscv(ofstream& ofs){
    ALIGN(ofs);
    if(op_ == Operator::Sub){
        ofs << "neg "; dest_->Dump(ofs); ofs << ", "; rhs_->Dump(ofs);
    }
    else{
        ofs << "seqz "; dest_->Dump(ofs); ofs << ", "; rhs_->Dump(ofs);
    }
    ofs << "\n";
}

void TLoadGInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "load ";
    val_->TDump(ofs); ofs << " ";
    dest_->Dump(ofs);
    ofs << "\n";
}

void TLoadGInst::DumpRiscv(ofstream& ofs){
    ALIGN(ofs);
    ofs << "lui "; dest_->Dump(ofs); ofs << ", \%hi("; val_->TDump(ofs); ofs << ")\n";
    ALIGN(ofs);
    ofs << "lw "; dest_->Dump(ofs); ofs << ", \%lo("; val_->TDump(ofs); ofs << ")("; dest_->Dump(ofs);  ofs << ")\n";
}

void TLoadSInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "load " << num_ << " ";
    dest_->Dump(ofs);
    ofs << "\n";
}

void TLoadSInst::DumpRiscv(ofstream& ofs){
    if(num_ < mini10 || num_ > maxi10){
        ALIGN(ofs);
        ofs << "li s9, " << num_ * 4 << "\n";
        ALIGN(ofs);
        ofs << "add s9, s9, sp\n";
        ALIGN(ofs);
        ofs << "lw "; dest_->Dump(ofs); ofs << ", (s9)\n";
    }
    else{
        ALIGN(ofs);
        ofs << "lw "; dest_->Dump(ofs); ofs << ", " << num_*4 << "(sp)\n";
    } 
}

void TLoadAddrGInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "loadaddr ";
    val_->TDump(ofs); ofs << " ";
    dest_->Dump(ofs);
    ofs << "\n";
}

void TLoadAddrGInst::DumpRiscv(ofstream& ofs){
    ALIGN(ofs);
    ofs << "la "; dest_->Dump(ofs); ofs << ", "; val_->TDump(ofs); ofs << "\n";
}

void TLoadAddrSInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "loadaddr " << num_ << " ";
    dest_->Dump(ofs);
    ofs << "\n";
}

void TLoadAddrSInst::DumpRiscv(ofstream& ofs){
    if(num_ < mini10 || num_ > maxi10){
        ALIGN(ofs);
        ofs << "li s9, " << num_ << "\n";
        ALIGN(ofs);
        ofs << "add "; dest_->Dump(ofs); ofs << ", s9, sp\n";
    }
    else{
        ALIGN(ofs);
        ofs << "addi "; dest_->Dump(ofs); ofs << ", sp, " << num_ * 4 << "\n";        
    }
    

}

void TStoreInst::Dump(ofstream& ofs){
    SPACE(ofs);
    ofs << "store ";
    reg_->Dump(ofs); ofs << " ";
    ofs << num_ << "\n";
}

void TStoreInst::DumpRiscv(ofstream& ofs){
    if(num_ < mini10 || num_ > maxi10){
        ALIGN(ofs);
        ofs << "li s9, " << 4 * num_ << "\n";
        ALIGN(ofs);
        ofs << "add s9, s9, sp\n";
        ALIGN(ofs);
        ofs << "sw "; reg_->Dump(ofs); ofs << ", (s9)\n";

    }
    else{
        ALIGN(ofs);
        ofs << "sw "; reg_->Dump(ofs); ofs << ", " << num_ * 4 << "(sp)\n";        
    }
}