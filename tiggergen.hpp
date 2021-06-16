#ifndef TIGGERGEN_HPP_
#define TIGGERGEN_HPP_
#include "eeyore.hpp"
#include "global.hpp"
#include "cfg.hpp"
#include "tigger.hpp"
#include "regalloc.hpp"
using namespace std;

class TiggerGenerator{
    public:
    TiggerGenerator(GlobalManager& _gm, FuncSet& _fc):
    gm_(_gm), fc_(_fc){}

    void BuildReg();

    void GenerateOn();

    void GenGlobal();
    void GenVarDec(EVarDecInst& inst);
    void GenFuncDef(EFuncDefInst& inst);
    void GenEnd(EEndInst& inst);
    void GenAssign(EAssignInst& inst);
    void GenIf(EIfInst& inst);
    void GenGoto(EGotoInst& inst);
    void GenLabel(ELabelInst& inst);
    void GenFuncCall(ECallInst& inst);
    void GenParam(EParamInst& inst);
    void GenReturn(EReturnInst& inst);
    void GenBinary(EBinaryInst& inst);
    void GenUnary(EUnaryInst& inst);

    OprPtr GetUseReg(EValPtr var, int _pos,  int _loc);
    OprPtr GetDefReg(EValPtr var, int _pos,  int _loc);
    void StoreVal(EValPtr var, int _pos, OprPtr _reg);
    void PreStoreVal(EValPtr var, int _pos, OprPtr _reg);
    void PreParam(ECallInst& inst, int num);

    void Dump(ofstream& ofs);
    void DumpRiscv(ofstream& ofs);
    
    private:
    TInstPtrList ginst_;
    TInstPtrList inst_;
    GlobalManager& gm_;
    FuncSet& fc_;
    EFuncDefPtr cur_func_;

    unordered_map<string, OprPtr> reg_map_;
    vector<OprPtr> rsv_reg_;

    bool is_if_ = false;
    int param_stack_;
    int stk_;
};

#endif