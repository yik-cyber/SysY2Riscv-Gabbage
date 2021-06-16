#ifndef EEYORE_HPP
#define EEYORE_HPP

#include <ostream>
#include <memory>
#include <cassert>
#include <utility>
#include <optional>
#include <vector>
#include <string>
#include <fstream>
#include <utility>
#include <unordered_map>
#include <cstddef>
#include <set>
#include "token.hpp"
using namespace std;

class EInstBase;
class EValueBase;
class EVarVal;
class EFuncDefVal;
using EFuncDefPtr = shared_ptr<EFuncDefVal>;

class Block;
using BlockPtr = shared_ptr<Block>;
using BlockPtrList = vector<BlockPtr>;
using LabelMap = unordered_map<int, BlockPtr>;

class CFGAnalyzer;
using CFGAnalyzerPtr = shared_ptr<CFGAnalyzer>;

class Interval;
using IntervalPtr = shared_ptr<Interval>;
class Register;
using RegisterPtr = shared_ptr<Register>;
using RegisterPtrList = vector<RegisterPtr>;
class GlobalVar;
using GlovalVarPtr = shared_ptr<GlobalVar>;

// Ptr
using EInstPtr = shared_ptr<EInstBase>;
using EInstPtrList = vector<EInstPtr>;
using EValPtr = shared_ptr<EValueBase>;
using EValPtrList = vector<EValPtr>;

class TiggerGenerator;

enum class EInstType{
    FucDef,
    End,
    VarDec,
    Assign,
    If,
    Goto,
    Label,
    Call,
    Param,
    Ret,
    Binary,
    Unary
};

//instructions
using EValPair = pair<vector<EValPtr>, vector<EValPtr> >;
class EInstBase {
public:
    virtual ~EInstBase() = default;
    virtual void Dump(ofstream &ofs){
        assert(false);
    };
    virtual void GenerateTigger(TiggerGenerator& tgen) {assert(false);}
    virtual bool IsBranch() {return false;}
    virtual bool IsGoto() {return false;}
    virtual int GetGotoLabelID() { assert(false); return -1;}
    virtual EValPair VisitOpr() {assert(false);}
    virtual bool NeedReg() {assert(false);}
    bool NeedTemp() {return false;}
    virtual EValPtr GetVar() {assert(false);}
    virtual EValPtr GetParam() {assert(false);}
    virtual string& GetReg() {assert(false);}
    virtual void Resolving(BlockPtr block, BlockPtr backward_block) {return;}
    virtual void SetReservePos(int pos) {assert(false);}
    virtual void AddReserveReg(RegisterPtr reg) {assert(false);}
    virtual void SetStackPos(int pos) {assert(false);}
    virtual int GetStackPos() {assert(false);}
    
    int pos_;
    EInstType type_;
};

enum class EValType{
    Var,
    Array,
    ArrayElem,
    Label,
    Int,
    Reg
};

//values
class EValueBase {
    public:
    EValueBase(){
        
    }
    virtual ~EValueBase() = default;
    // dump
    virtual void Dump(ofstream& ofs) {assert(false);};
    virtual void TDump(ofstream& ofs) {assert(false);}
    virtual void GenerateTigger(TiggerGenerator& tgen) {assert(false);}
    virtual void Resolving(bool _is_use, int _pos,BlockPtr block, BlockPtr backward_block);

    // getter
    virtual char GetType() {assert(false); return 0;}
    virtual int GetID(){ assert(false); return -1;}
    virtual int GetDim() {assert(false); return -1;}
    virtual EValPtr GetAddr() {assert(false);}
    virtual EValPtr GetIndex() {assert(false);}
    virtual int GetConstVal() {assert(false);}
    virtual int GetStackPos() {assert(false);}
    virtual void Minus() {assert(false);}

    virtual bool IsArrayElem() {return false;}
    virtual bool IsArray() {return false;}
    virtual bool NeedAlloc() {
        if(is_global_)
            return false;
        else
            return true;
    }
    virtual string& GetReg() {assert(false);}
    
    IntervalPtr interval_ = nullptr;
    bool is_global_ = false;
    bool loaded_ = false;
    bool is_spilled_ = false;
    EValType val_type_;
    //OprPtr gvar_ = nullptr; // point to a global var
};

class EFuncDefInst: public EInstBase{
    public:
    EFuncDefInst(EFuncDefPtr _func, int _loc):
    func_(move(_func)), loc_(_loc){type_ = EInstType::FucDef;}
    void Dump(ofstream& ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return false;}
    
    //
    EFuncDefPtr GetFunc() {return func_;}
    private:
    EFuncDefPtr func_;
    int loc_;
};

class EEndInst: public EInstBase{
    public:
    EEndInst(EFuncDefPtr _func, int _loc):
    func_(move(_func)), loc_(_loc){type_ = EInstType::End;}
    void Dump(ofstream& ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return false;}
    
    //
    EFuncDefPtr GetFunc() {return func_;}
    private:
    EFuncDefPtr func_;
    int loc_;
};

class EVarDecInst: public EInstBase{
public:
    EVarDecInst(EValPtr _var, bool _is_array, int _loc):
            var_(move(_var)), is_array_(_is_array), loc_(_loc){type_ = EInstType::VarDec;}

    // dump
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return false;}

    //
    bool IsArray(){return is_array_;}
    EValPtr GetVar() {return var_;}

private:
    EValPtr var_;
    bool is_array_;
    int loc_;
};

class EAssignInst: public EInstBase{
public:
    EAssignInst(EValPtr _dest, EValPtr _val, int _loc):
            dest_(move(_dest)), val_(move(_val)), loc_(_loc) {type_ = EInstType::Assign;}
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return true;}
    EValPair VisitOpr();
    bool NeedTemp() {return dest_->IsArrayElem() || val_->IsArrayElem();}
    void Resolving(BlockPtr block, BlockPtr backward_block);
    //
    EValPtr GetDest() {return dest_;}
    EValPtr GetVal() {return val_;}
private:
    EValPtr dest_;
    EValPtr val_;
    int loc_;
};

class EIfInst: public EInstBase {
public:
    EIfInst(EValPtr _lhs, Operator _op, EValPtr _rhs, EValPtr _label, int _loc):
            lhs_(move(_lhs)),op_(_op), rhs_(move(_rhs)), label_(move(_label)), loc_(_loc) {type_ = EInstType::If;}
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return true;}
    EValPair VisitOpr();
    void Resolving(BlockPtr block, BlockPtr backward_block);

    // getters
    bool IsBranch() {return true;}
    int GetGotoLabelID() {return label_->GetID();}

    //
    EValPtr GetLhs() {return lhs_;}
    EValPtr GetRhs() {return rhs_;}
    Operator GetOp() {return op_;}
    EValPtr GetLabel() {return label_;}

private:
    EValPtr lhs_;
    Operator op_;
    EValPtr rhs_;
    EValPtr label_;
    int loc_;
};

class EGotoInst: public EInstBase{
public:
    EGotoInst(EValPtr _label, int _loc):label_(move(_label)), loc_(_loc){type_ = EInstType::Goto;}
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return false;}

    // getters
    bool IsBranch() {return true;}
    bool IsGoto() {return true;}
    EValPtr GetLabel() {return label_;}
    int GetGotoLabelID() {return label_->GetID();}

private:
    EValPtr label_;
    int loc_;
};

class ELabelInst: public EInstBase {
public:
    ELabelInst(EValPtr _label, int _loc):label_(move(_label)), loc_(_loc) {type_ = EInstType::Label;}
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return false;}
    EValPtr GetLabel() {return label_;}

private:
    EValPtr label_;
    int loc_;
};


class ECallInst: public EInstBase {
public:
    ECallInst(EValPtr _dest, EFuncDefPtr _func, int _loc):
            dest_(move(_dest)), func_(move(_func)), loc_(_loc){type_ = EInstType::Call;}
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return true;}
    EValPair VisitOpr();
    EFuncDefPtr GetFunc() {return func_;}
    void AddVal(EValPtr _val) {vals_.push_back(_val);}
    void AddParam(EInstPtr _param) {params_.push_back(move(_param));}
    void Resolving(BlockPtr block, BlockPtr backward_block);
   
    RegisterPtrList reserve_regs_;
    int reserve_pos_;
    void SetReservePos(int pos) {reserve_pos_ = pos;}
    void AddReserveReg(RegisterPtr reg) {reserve_regs_.push_back(move(reg));}
    
    EInstPtrList params_;
    EValPtrList vals_;
    EValPtr dest_;

private:
    EFuncDefPtr func_;   
    int loc_;
};

class EParamInst: public EInstBase {
public:
    EParamInst(EValPtr _param, int _loc):param_(move(_param)), loc_(_loc){type_ = EInstType::Param;}
    void Dump(ofstream& ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    void Resolving(BlockPtr block, BlockPtr backward_block);
    void SetReg(string _reg){reg_ = move(_reg);}
    string& GetReg() {return reg_;}
    void SetCall(EInstPtr _call) {call_inst_ = move(_call);}
    EInstPtr GetCall() {return call_inst_;}
    void SetFunc(EFuncDefPtr _func) {func_ = move(_func);}
    EValPtr GetParam() {return param_;}
    bool NeedReg() {return true;}
    void SetStackPos(int pos) {stack_pos_ = pos;}
    int GetStackPos() {return stack_pos_;}
    EValPair VisitOpr();
    
private:
    EFuncDefPtr func_;
    EValPtr param_;
    string reg_;
    EInstPtr call_inst_;
    int stack_pos_ = -1;
    int loc_;
};

class EReturnInst: public EInstBase {
public:
    EReturnInst(int _loc, EValPtr _val, EFuncDefPtr _func):
    loc_(_loc), val_(move(_val)), func_(move(_func)) {type_ = EInstType::Ret;}
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    bool NeedReg() {return true;}
    EValPair VisitOpr();
    void Resolving(BlockPtr block, BlockPtr backward_block);
    EFuncDefPtr GetFunc() {return func_;}
    EValPtr val_;
    
private:
    int loc_; 
    EFuncDefPtr func_;
};

class EBinaryInst: public EInstBase {
public:
    EBinaryInst(EValPtr _dest, EValPtr _lhs, Operator _op, EValPtr _rhs, int _loc):
            dest_(move(_dest)), lhs_(move(_lhs)), op_(_op), rhs_(move(_rhs)),loc_(_loc)  {type_ = EInstType::Binary;}
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    void Resolving(BlockPtr block, BlockPtr backward_block);
    EValPtr GetLhs() {return lhs_;}
    EValPtr GetRhs() {return rhs_;}
    EValPtr GetDest() {return dest_;}
    Operator GetOp() {return op_;}
    bool NeedReg() {return true;}
    EValPair VisitOpr();

private:
    Operator op_;
    EValPtr dest_;
    EValPtr lhs_;
    EValPtr rhs_;
    int loc_;
};

class EUnaryInst: public EInstBase {
public:
    EUnaryInst(EValPtr _dest, Operator _op, EValPtr _rhs, int _loc):
            dest_(move(_dest)), op_(move(_op)), rhs_(move(_rhs)),loc_(_loc) {type_ = EInstType::Unary;}
    void Dump(ofstream &ofs);
    void GenerateTigger(TiggerGenerator& tgen);
    void Resolving(BlockPtr block, BlockPtr backward_block);
    EValPtr GetDest() {return dest_;}
    EValPtr GetRhs() {return rhs_;}
    Operator GetOp() {return op_;}
    bool NeedReg() {return true;}
    EValPair VisitOpr();

private:
    Operator op_;
    EValPtr dest_;
    EValPtr rhs_;
    int loc_;
};



//单个变量的初值在定义时就可以直接赋予
class EVarVal: public EValueBase {
public:
    EVarVal(char _type, int _id):
            type_(_type), id_(_id){val_type_ = EValType::Var;}
    void Dump(ofstream& ofs);
    void TDump(ofstream& ofs);
    //void Resolving(BlockPtr block, BlockPtr backward_block);

    // getter
    char GetType() {return type_;}
    int GetID() {return id_;}

private:
    char type_;
    int id_;
};


class EArrayElemVal: public EValueBase{
public:
    EArrayElemVal(EValPtr _addr, EValPtr _index):
        addr_(move(_addr)),index_(move(_index)){val_type_ = EValType::ArrayElem;}
    void Dump(ofstream& ofs);
    void Resolving(bool is_use, int _pos, BlockPtr block, BlockPtr backward_block);
    EValPtr GetAddr() {return addr_;}
    EValPtr GetIndex() {return index_;}
    bool IsArrayElem() {return true;}

private:
    EValPtr addr_;
    EValPtr index_;
};


class EArrayVal: public EValueBase {
public:
    EArrayVal(char _type, int _id, int _dim):
            type_(_type), id_(_id), dim_(_dim){val_type_ = EValType::Array;}
    void Dump(ofstream& ofs);
    void TDump(ofstream& ofs);
    //void Resolving(BlockPtr block, BlockPtr backward_block);
    bool NeedAlloc() {return true;}

    // getter
    char GetType() {return type_;}
    int GetID() {return id_;}
    int GetDim() {return dim_;}
    int GetStackPos() {return stack_pos_;}

    //
    void SetStackPos(int _pos) {stack_pos_ = _pos;}
    bool IsArray() {return true;}

private:
    char type_;
    int id_;
    int dim_;
    int stack_pos_; //栈中起始位置
};


class ELabelVal : public EValueBase {
public:
    ELabelVal(int _id) : id_(_id) {val_type_ = EValType::Label;}
    void Dump(ofstream& ofs);
    void Resolving(BlockPtr block, BlockPtr backward_block){return;}

    // getter
    int GetID() {return id_;}
    bool NeedAlloc() {return false;}

private:
    int id_;
};

// integer
class EIntVal : public EValueBase {
public:
    EIntVal(int val) : val_(val) {val_type_ = EValType::Int;}
    void Dump(ofstream& ofs);
    void Resolving(BlockPtr block, BlockPtr backward_block){return;}

    // getter
    int GetConstVal() {return val_;}
    bool NeedAlloc() {return false;}
    void Minus() {val_ = -val_;}

private:
    int val_;
};

// 对应一个真实的寄存器，主要用于param, call, return指令的分解
class ERegVal : public EValueBase {
    public:
    ERegVal(string& _ident):reg_(_ident){
        //assert(false);
        val_type_ = EValType::Reg;
    }
    void Dump(ofstream& ofs);
    string& GetReg() {return reg_;}
    char GetType() {return 'a';}
    //bool NeedAlloc() {return false;}

    private:
    string reg_;
};
using ERegValPtr = shared_ptr<ERegVal>;
/**
 * 在不考虑函数调用的情况下
 * 尝试把一个函数划分为几个block，跳转只能发生在函数内部的block之间
 * 1. 一个初始block，没有label
 * 2. if语句后不论有没有label都自成一个block，直到下一个block
 * 3. 自带标签的block
 * 
 * 在考虑函数调用的情况下，我们先假设调用函数时所有寄存器的值都被压入栈中
 * 当函数返回时，从栈中取出值，寄存器状态恢复到调用前，除了a0保存返回值
 * 所以我们先不考虑各个函数间的block关系，只分析函数内部block
*/
class Block{
    public:
    Block(EFuncDefPtr _func, bool _labeled):
    func_(move(_func)), labeled_(_labeled){
        block_id_ = next_block_id_++;
    }  
    void Dump(ofstream& ofs);
    void PushInst(EInstPtr _inst) {inst_.push_back(move(_inst));}

    // getters
    EInstPtr GetLastInst(){
        if(!inst_.empty())
            return inst_.back();
        return nullptr;
    }
    bool IsInstEmpty() {return inst_.empty();}
    int GetBlockID() {return block_id_;}
    bool IsLabeled() {return labeled_;}
    EFuncDefPtr GetFunc() {return func_;}
    BlockPtr GetSuccessor() {return  successor_;}
    BlockPtrList& GetSucs() {return sucs_;}
    BlockPtrList& GetPred() {return pred_;}
    EInstPtrList& GetInsts() {return inst_;}
    BlockPtr GetPredecessor() {return predecessor_;}
    BlockPtr GetForward() {return forward_;}
    BlockPtr GetBackward() {return backward_;}
    bool IsVisited() {return is_visited_;}
    bool IsActive() {return is_active_;}
    bool IsLoopHeader() {return is_loop_header_;}
    bool IsLoopEnd() {return is_loop_end_;}
    const set<int>& GetLoopIndexes() {return loop_indexes_;}
    int GetLoopIndex() {return loop_index_;}
    int GetLoopDepth() {return loop_depth_;}
    int GetForwardBlockNum() {return forward_block_num_;}

    // setters
    void SetSuccessor(BlockPtr _scs) {successor_ = move(_scs);}
    void SetPredecessor(BlockPtr _pred) {predecessor_ = move(_pred);}
    void SetBackward(BlockPtr _back) {backward_ = move(_back);}
    void SetForward(BlockPtr _forward) {forward_ = move(_forward);}
    void AddSucs(BlockPtr _scus) {sucs_.push_back(move(_scus));}
    void AddPred(BlockPtr _pred) {pred_.push_back(move(_pred));}

    void SetVisit() {is_visited_ = true;}
    void UnsetVisit() {is_visited_ = false;}
    void SetActive() {is_active_ = true;}
    void UnsetActive() {is_active_ = false;}
    void SetLoopHeader() {is_loop_header_ = true;}
    void SetLoopEnd() {is_loop_end_ = true;}
    // 只有loop header和 loop end才设置这个
    void SetLoopIndex(int _index) { 
        assert(is_loop_end_ || is_loop_header_);
        loop_index_ = _index;
    }
    void AddLoopIndex(int _index) {loop_indexes_.insert(_index);}
    void SetLoopDepth() {loop_depth_ = loop_indexes_.size();}
    // 在非回溯时添加
    void IncForwardBlockNum() {forward_block_num_++;}
    void DecForwardBlockNum() {forward_block_num_--;}

    // reset block id
    static void ResetBlockID() {next_block_id_ = 0;}

    //private: 每次都创建接口太麻烦了
    static int next_block_id_;
    int block_id_;
    bool labeled_ = false;
    EInstPtrList inst_;

    BlockPtr successor_ = nullptr,
             predecessor_ = nullptr;
    BlockPtr backward_ = nullptr, 
             forward_ = nullptr;
    BlockPtrList sucs_;
    BlockPtrList pred_;

    bool is_visited_ = false, 
         is_active_ = false,
         is_loop_header_ = false, 
         is_loop_end_ = false;

    set<int> loop_indexes_;
    int loop_index_ = -1;
    int loop_depth_ = -1;
    int forward_block_num_ = 0;

    int from_ = -1;
    int to_ = __INT_MAX__;

    set<EValPtr> live_gen_;  // 在block之前被定义的操作变量
    set<EValPtr> live_kill_; // 在block之中被赋值的操作变量
    set<EValPtr> live_in_;   // 在block初始存活的操作变量
    set<EValPtr> live_out_;  // 在block末尾存活的操作变量  

    EFuncDefPtr func_ = nullptr;
};
using BlockPtr = shared_ptr<Block>;
using BlockPtrList = vector<BlockPtr>;
using LabelMap = unordered_map<int, BlockPtr>;
using SymbolTable = unordered_map<string, EValPtr>;

class EFuncDefVal: public EValueBase {
    public:
    //
    EFuncDefVal(string& _ident, int _param, int _loc_start):
    ident_(_ident), param_(_param), loc_start_(_loc_start){}
    void Dump(ofstream& ofs);
    void DumpBlock(ofstream& ofs);
    // 把回溯边和跳转边加上
    void BuildBlockGragh();
    //void LoopAnalyzeOn();
    //void SortBlocks();

    // setters
    void SetAnalyzer(CFGAnalyzerPtr _analyzer) {cfg_analyzer_ = move(_analyzer);}
    void SetEndLoc(int _loc) {loc_end_ = _loc;}
    void PushBlock(BlockPtr _block) {blocks_.push_back(move(_block));}
    void PopBlock() {blocks_.pop_back();}
    void AddLabeledBlock(int _label, BlockPtr _block);
    void AddTable(string& _ident, EValPtr _val) {table_.insert({_ident, _val});}
    //
    void AddStackSize(int _size) {
        stack_size_ += _size;
    }
    int GetStackSize() {return stack_size_;}

    // getters
    string& GetIdent() {return ident_;}
    int GetParamNum() {return param_;}
    BlockPtr GetLabeledBlock(int label);
    BlockPtrList& GetBlocks() {return blocks_;}
    EValPtr GetVal(string& _ident) {
        if(table_.find(_ident) != table_.end()){
            return table_[_ident];
        }
        return nullptr;       
    }

    CFGAnalyzerPtr cfg_analyzer_ = nullptr;
    BlockPtrList blocks_;
    RegisterPtrList reserved_regs;
    int reserved_reg_stack_;

    EValPtrList global_vars_;
    private:
    string ident_;
    int param_;
    int stack_size_ = 0;
    int loc_start_;
    int loc_end_;
     
    // label id到blockptr的映射
    LabelMap label2block_;
    // 函数的符号表
    SymbolTable table_;
    // reserved reg
};
using EFuncDefPtr = shared_ptr<EFuncDefVal>;

#endif
