#ifndef TIGGER_HPP_
#define TIGGER_HPP_
#include <ostream>
#include <memory>
#include <cassert>
#include <utility>
#include <vector>
#include <string>
#include <fstream>
#include <utility>
#include <unordered_map>
#include <cstddef>
#include <set>
#include "token.hpp"
#include "eeyore.hpp"
#include "regalloc.hpp"
using namespace std;

enum class OprType{
    Num,
    Reg
};

class OprBase{
    public:
    virtual ~OprBase() = default;
    virtual void Dump(ofstream& ofs) {assert(false);}
    virtual void DumpRiscv(ofstream& ofs) {assert(false);}
    virtual int GetNum() {assert(false);}
     
    OprType type_;
    string ident_;
    bool is_array_;
    int size_;
};
using OprPtr = shared_ptr<OprBase>;

class NumOpr: public OprBase {
    public:
    NumOpr(int _num):num_(_num) {
        type_ = OprType::Num;
    }
    void Dump(ofstream& ofs);
    int GetNum() {return num_;}

    int num_;
};

class RegOpr: public OprBase{
    public:
    RegOpr(){
        type_ = OprType::Reg;
    }
    RegOpr(const string ident){
        ident_ = ident;
        type_ = OprType::Reg;
    }
    void Dump(ofstream& ofs);
    
    //RegisterPtr reg_;
};


//instructions
class TInstBase {
public:
    virtual ~TInstBase() = default;
    virtual void Dump(ofstream &ofs){
        assert(false);
    };
    virtual void DumpRiscv(ofstream& ofs){
        assert(false);
    }
};
using TInstPtr = shared_ptr<TInstBase>;
using TInstPtrList = vector<TInstPtr>;

class TFuncDefInst: public TInstBase{
    public:
    TFuncDefInst(EFuncDefPtr _func, int _stk):
    func_(move(_func)), stk_(_stk){}
    void Dump(ofstream& ofs);
    void DumpRiscv(ofstream& ofs);

    private:
    EFuncDefPtr func_;
    int stk_ ;
};

class TEndInst: public TInstBase{
    public:
    TEndInst(EFuncDefPtr _func):
    func_(move(_func)){}
    void Dump(ofstream& ofs);
    void DumpRiscv(ofstream& ofs);

    private:
    EFuncDefPtr func_;
};

class TVarDecInst: public TInstBase{
public:
    TVarDecInst(EValPtr _var):
            var_(move(_var)){}

    // dump
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

private:
    EValPtr var_;
};

class TAssignInst: public TInstBase{
public:
    TAssignInst(OprPtr _dest, OprPtr _val, int _array = -1):
            dest_(move(_dest)), val_(move(_val)),array_(_array) {}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

private:
    OprPtr dest_;
    OprPtr val_;
    int array_;
    int loc_;
};

class TIfInst: public TInstBase {
public:
    TIfInst(OprPtr _lhs, Operator _op, OprPtr _rhs, EValPtr _label):
            lhs_(move(_lhs)),op_(_op), rhs_(move(_rhs)), label_(move(_label)) {}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

private:
    OprPtr lhs_;
    Operator op_;
    OprPtr rhs_;
    EValPtr label_;
};

class TGotoInst: public TInstBase{
public:
    TGotoInst(EValPtr _label):label_(move(_label)){}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

private:
    EValPtr label_;
};

class TLabelInst: public TInstBase {
public:
    TLabelInst(EValPtr _label):label_(move(_label)){}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

private:
    EValPtr label_;
};


class TCallInst: public TInstBase {
public:
    TCallInst(EFuncDefPtr _func):
    func_(move(_func)){}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

private:
    EFuncDefPtr func_;
};

class TReturnInst: public TInstBase {
public:
    TReturnInst(int _stk):
    stk_(_stk){}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);


private:
    int stk_;
};

class TBinaryInst: public TInstBase {
public:
    TBinaryInst(OprPtr _dest, OprPtr _lhs, Operator _op, OprPtr _rhs):
            dest_(move(_dest)), lhs_(move(_lhs)), op_(_op), rhs_(move(_rhs)){}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

private:
    Operator op_;
    OprPtr dest_;
    OprPtr lhs_;
    OprPtr rhs_;
};

class TUnaryInst: public TInstBase {
public:
    TUnaryInst(OprPtr _dest, Operator _op, OprPtr _rhs):
            dest_(move(_dest)), op_(move(_op)), rhs_(move(_rhs)){}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

private:
    Operator op_;
    OprPtr dest_;
    OprPtr rhs_;
};

class TLoadGInst: public TInstBase {
    public:
    TLoadGInst(OprPtr _dest, EValPtr _val):
    dest_(move(_dest)), val_(move(_val)){}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

    private:
    OprPtr dest_;
    EValPtr val_;
};

class TLoadSInst: public TInstBase {
    public:
    TLoadSInst(OprPtr _dest, int _num):
    dest_(move(_dest)), num_(_num){}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

    private:
    OprPtr dest_;
    int num_;
};

class TLoadAddrGInst: public TInstBase {
    public:
    TLoadAddrGInst(OprPtr _dest, EValPtr _val):
    dest_(move(_dest)), val_(move(_val)) {}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

    private:
    OprPtr dest_;
    EValPtr val_;
};

class TLoadAddrSInst: public TInstBase {
    public:
    TLoadAddrSInst(OprPtr _dest, int _num):
    dest_(move(_dest)), num_(_num) {}
    void Dump(ofstream &ofs);
    void DumpRiscv(ofstream& ofs);

    private:
    OprPtr dest_;
    int num_;
};

class TStoreInst: public TInstBase {
    public:
    TStoreInst(OprPtr _reg, int _num):
    reg_(move(_reg)), num_(_num) {}
    void Dump(ofstream &ofs);

    void DumpRiscv(ofstream& ofs);

    private:
    OprPtr reg_;
    int num_;
};

#endif


