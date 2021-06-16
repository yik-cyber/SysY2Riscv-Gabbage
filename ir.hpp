#ifndef IR_H_
#define IR_H_

#include <ostream>
#include <memory>
#include <cassert>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <utility>
#include <unordered_map>
#include <cstddef>

#include "token.hpp"
using namespace std;

class InstBase;

class ValueBase;
class VarVal;

class FuncDef;

class CondBranch;
using BranchPtr = shared_ptr<CondBranch>;

//Values
enum class VarType{
    T, t, p, None
};

// Ptr
using InstPtr = shared_ptr<InstBase>;
using InstPtrList = vector<InstPtr>;
using ValPtr = shared_ptr<ValueBase>;
using ValPtrList = vector<ValPtr>;
using FuncDefPtr = shared_ptr<FuncDef>;

//instructions
class InstBase {
 public:
  virtual ~InstBase() = default;
  virtual void Dump(stringstream &ofs){
      assert(false);
  };
};

//values
class ValueBase {
 public:
  virtual ~ValueBase() = default;

  // dump
  virtual void Dump(stringstream& ofs) {assert(false);};

  // getter
  virtual VarType GetType() {assert(false);}
  virtual int GetID(){ assert(false);}
  
  virtual vector<int>& GetDims() {assert(false);}
  virtual vector<int>& GetAccs() {assert(false);}

  virtual int GetConstVal() {assert(false);}
  virtual int GetConstVal(int _idx) {assert(false);}
  virtual ValPtr& GetVal() {assert(false);}
  
  virtual int GetTotDimNum() {assert(false);}
  virtual int GetDimNum() {assert(false);}
  virtual bool IsArray() {return false;}
  virtual bool IsArrayElem() {return false;}
  virtual bool IsLval() {return false;}
};


class VarDecInst: public InstBase{
    public:
    VarDecInst(ValPtr _var, bool _is_array, bool _is_temp = true):
    var_(move(_var)), is_array_(_is_array), is_temp_(_is_temp){}

    // dump
    void Dump(stringstream &ofs);

    private:
    ValPtr var_;
    bool is_temp_;
    bool is_array_;
};

class AssignInst: public InstBase{
    public:
    AssignInst(ValPtr _dest, ValPtr _val):
    dest_(move(_dest)), val_(move(_val)) {}

    void Dump(stringstream &ofs);

    private:
    ValPtr dest_;
    ValPtr val_;
};

class IfInst: public InstBase {
    public:
    IfInst(ValPtr _cond, ValPtr _false_label):
    cond_(move(_cond)), false_label_(move(_false_label)) {}

    void Dump(stringstream &ofs);
    void SetTrueLabel(ValPtr true_label){
        true_label_ = true_label;
    }

    private:
    ValPtr cond_;
    ValPtr false_label_;
    ValPtr true_label_;
};

class BranchInst: public InstBase {
    public:
    BranchInst(BranchPtr _branch):branch_(move(_branch)){}

    private:
    BranchPtr branch_;  
};

class GotoInst: public InstBase{
    public:
    GotoInst(ValPtr _label):label_(move(_label)){}

    void Dump(stringstream &ofs);

    private:
    ValPtr label_;
};

class LabelInst: public InstBase {
    public:
    LabelInst(ValPtr _label):label_(move(_label)) {}

    void Dump(stringstream &ofs);

    private:
    ValPtr label_;
};


class CallInst: public InstBase {
    public:
    CallInst(ValPtr _dest, ValPtr _func, ValPtrList _params):
    dest_(move(_dest)), func_(move(_func)), params_(move(_params)) {}

    void Dump(stringstream &ofs);

    private:
    ValPtr dest_;
    ValPtr func_;
    ValPtrList params_;
};

class ReturnInst: public InstBase {
    public:
    ReturnInst(ValPtr _val):val_(move(_val)) {}

    void Dump(stringstream &ofs);

    private:
    ValPtr val_;
};

class BinaryInst: public InstBase {
    public:
    BinaryInst(ValPtr _dest, ValPtr _lhs, Operator _op, ValPtr _rhs):
    dest_(move(_dest)), lhs_(move(_lhs)), op_(_op), rhs_(move(_rhs)) {}

    void Dump(stringstream &ofs);

    private:
    Operator op_;
    ValPtr dest_;
    ValPtr lhs_;
    ValPtr rhs_;
};

class UnaryInst: public InstBase {
    public:
    UnaryInst(ValPtr _dest, Operator _op, ValPtr _rhs):
    dest_(move(_dest)), op_(move(_op)), rhs_(move(_rhs)) {}

    void Dump(stringstream &ofs);

    private:
    Operator op_;
    ValPtr dest_;
    ValPtr rhs_;
};



//单个变量的初值在定义时就可以直接赋予
class VarVal: public ValueBase {
    public:
    VarVal(VarType _type, int _id):
    type_(_type), id_(_id){}

    void Dump(stringstream& ofs);

    // getter
    VarType GetType() {return type_;}
    int GetID() {return id_;}
    ValPtr& GetVal() {return val_;}
    int GetConstVal() {return val_->GetConstVal(); }
    bool IsLval() {return true;}


    // setter
    void SetVal(ValPtr _val) {val_ = move(_val);}
    void SetType(VarType _type) { type_ = _type; }

    private:
    VarType type_;
    int id_;
    ValPtr val_;
};


class ArrayElemVal: public ValueBase{
    public:
    ArrayElemVal(VarType _type, int _id, ValPtr _index):
    type_(_type), id_(_id), index_(_index) {}
    ArrayElemVal(ValPtr _addr, ValPtr _index):addr_(move(_addr)),
    index_(move(_index)){}

    void Dump(stringstream& ofs);

    bool IsArrayElem(){
        return true;
    }
    bool IsLval() {return true;}

    private:
    VarType type_;
    int id_;

    ValPtr addr_;
    ValPtr index_;
};


//非全局数组初始化时一律采用临时变量，以防第一次使用时再声明导致声明的值变化
// e.g. a[2] = {b1, b2}; b1 = c1; putint(a[0]);
/**
 * n = getint();
 * putint(a[n])
*/
class ArrayVal: public ValueBase {
    public:
    ArrayVal(VarType _type, int _id, vector<int> _dims, vector<int> _accs, int _tot_dim_num):
    type_(_type), id_(_id), dims_(move(_dims)),accs_(move(_accs)), tot_dim_num_(_tot_dim_num){
        dim_num_ = dims_.size() - 1;
    }

    void Dump(stringstream& ofs);

    // getter
    VarType GetType() {return type_;}
    int GetID() {return id_;}
    int GetTotDimNum() {return tot_dim_num_; }

    int GetDimNum() {return dim_num_;}

    vector<int>& GetDims() {return dims_;}
    vector<int>& GetAccs() {return accs_;}
    vector<int>& GetVals() {return vals_;}
    int GetConstVal(int _idx) {return vals_[_idx];}

    bool IsLval() {return true;}
    bool IsArray() {return true;}


    private:
    VarType type_;
    int id_;
    int tot_dim_num_; // numbers

    int dim_num_;    // dimension
    
    vector<int> dims_; //1 int the end
    vector<int> accs_;
    vector<int> vals_; //for global array
};

class FuncArgVal: public ValueBase {
    public:
    FuncArgVal(VarType _type, int _id, vector<int>& _dims):
    type_(type_), id_(_id),dims_(_dims){}
    FuncArgVal(VarType _type, int _id):
    type_(type_), id_(_id){}

    void Dump(stringstream& ofs);

    // getter
    int GetID(){return id_;}
    int GetDimNum() {return dim_num_;}
    bool IsArray() {return dims_.empty();}
    bool IsLval() {return true;}

    private:
    VarType type_;
    int id_;
    int dim_num_;
    vector<int> dims_; // 1 in the end
};

class FuncCallVal: public ValueBase {
    public:
    FuncCallVal(string& _ident):ident_(_ident){}

    void Dump(stringstream& ofs);

    // getter
    string& GetIdent(){return ident_;}
    bool IsLval() {return false;}

    private:
    string ident_;
};


class LabelVal : public ValueBase {
    public:
    LabelVal() : id_(next_id_++) {}

    void Dump(stringstream& ofs);

    // getter
    int GetID() {return id_;}
    bool IsLval() {return false;}

    private:
    static int next_id_;
    int id_;
};

// integer
class IntVal : public ValueBase {
    public:
    IntVal(int val) : val_(val) {}

    void Dump(stringstream& ofs);
    
    // getter
    int GetConstVal() {return val_;}
    bool IsLval() {return false;}

    private:
    int val_;
};


//etc f_main [0]
class FuncDef{
    public:
    FuncDef(Type _rtype, string _ident, int _arg_num):rtype_(_rtype),
    ident_(_ident), arg_num_(_arg_num){
        t_slot_id = 0;
        p_slot_id = 0; 
        // 全局变量的作用域直到程序结束，函数内变量作用域到函数结尾
        // 函数内的变量不会覆盖函数外的全局变量
        T_slot_id = global_T_slot_id;
        is_global = false;
        has_return = false;
    }

    void Dump(stringstream &ofs);

    // setter
    void PushInst(InstPtr _inst){
        insts_.push_back(_inst);
    }
    void PushDecl(InstPtr _decl_inst){
        decl_insts_.push_back(_decl_inst);
    }
    void SetGlobal() {
        is_global = true;
    }
    
    void PushCondLabel(ValPtr _cond){ cond_label_stack.push_back(_cond);}
    ValPtr TopCondLabel() {return cond_label_stack.back();}
    void PopCondLabel() {cond_label_stack.pop_back(); }
    void PushEndLabel(ValPtr _end){ end_label_stack.push_back(_end); }
    ValPtr TopEndLabel() { return end_label_stack.back(); }
    void PopEndLabel() {end_label_stack.pop_back(); }

    void SetThenLabel(ValPtr _then) {then_label_ = move(_then);}
    ValPtr GetThenLabel() {return then_label_;}
    void SetElseThenLabel(ValPtr _else_then_) {else_then_label_ = move(_else_then_);}
    ValPtr GetElseThenLabel() {return else_then_label_;}
    bool IsInCond() {return in_cond;}
    void SetInCond() {in_cond = true;}
    void UnsetInCond() {in_cond = false;}

    void SetNextCheck(ValPtr _next_check) {next_check_ = move(_next_check);}
    ValPtr GetNextCheck() {return next_check_;}

    void SetGlobalFunc(FuncDefPtr _global){
        global_ = _global;
    }
    
    //三种变量应该分别维护三个slot，全局变量如何处理
    int AddTSlot() {
        if(is_global)
            return global_T_slot_id++;
        else
            return T_slot_id++;
    }
    int AddpSlot() {return p_slot_id++;}
    int AddtSlot() {return t_slot_id++;}

    void Settype(Type _rtype) {rtype_ = _rtype;}

    void ResetGlobalSlot(){
        global_T_slot_id = 0;
    }
    void UpdateGlobalSlot(int last_func_T_slot) {
        global_T_slot_id = last_func_T_slot;
    }
    void SetMainLocalSlot(int global_t_slot_id){
        t_slot_id = global_t_slot_id;
    }    
    void SetReturnTrue(){
        has_return = true;
    }
    
    //getter
    bool IsGlobal() {return is_global;}
    Type GetRType(){ return rtype_;}
    bool HasReturn() {return has_return;}
    int GetTSlot() {return T_slot_id;}
    int GetpSlot() {return p_slot_id;}
    int GettSlot() {return t_slot_id;}
    int GetGlobalTSlot() {return global_T_slot_id;}
    InstPtrList& GetDeclInst() {return decl_insts_;}
    InstPtrList& GetInst() {return insts_;}

    private:
    Type rtype_;
    string ident_;
    int arg_num_;
    
    static int global_T_slot_id;
    int T_slot_id;
    int t_slot_id;
    int p_slot_id;
    InstPtrList decl_insts_;
    InstPtrList insts_;

    //label stack for loop
    ValPtrList cond_label_stack;
    ValPtrList end_label_stack;

    //label for if
    ValPtr then_label_;
    ValPtr else_then_label_;
    ValPtr next_check_;

    bool is_global;
    bool has_return;
    bool in_cond;

    FuncDefPtr global_ = nullptr;
};

class CondBranch{
    public:
    CondBranch(ValPtr _label_1, ValPtr _label_2):
    label_1_(move(_label_1)), label_2_(move(_label_2)){}

    // setters
    void PushInst_1(InstPtr _inst){
        block_1_ints_.push_back(move(_inst));
    }
    void PushInst_2(InstPtr _inst){
        block_2_ints_.push_back(move(_inst));
    }

    ValPtr GetLabel_1() {return label_1_;}
    ValPtr GetLabel_2() {return label_2_;}

    private:
    BranchPtr father_;

    ValPtr label_1_;
    ValPtr label_2_;

    InstPtrList block_1_ints_;
    InstPtrList block_2_ints_;
};

#endif