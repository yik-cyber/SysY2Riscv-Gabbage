#ifndef SYSY_AST_H_
#define SYSY_AST_H_
#include <optional>
#include <memory>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <cassert>
#include <algorithm>
#include <map>
#include "token.hpp"
#include "ir.hpp"
using namespace std;

class IRGenerator;

using namespace std;
class BaseAST {
 friend IRGenerator;
 public:
  virtual ~BaseAST() = default;
  virtual void print(ofstream& ofs){
      ofs << "error" << endl;
  };
  // generate IR
  virtual ValPtr GenerateIR(IRGenerator& _irgen) {assert(false);};
  virtual int GenerateInt(IRGenerator& _irgen) {assert(false);}
  virtual int HandleInitVal(IRGenerator& _irgen, shared_ptr<ArrayVal> _val_ptr, int _cur_dim, int _cur_index) {assert(false);}
  virtual int HandleInitInt(IRGenerator& _irgen, shared_ptr<ArrayVal> _val_ptr, int _cur_dim, int _cur_index) {assert(false);}
  
  virtual bool IsLiteral(){return false;}
  virtual Type GetType() {assert(false);}

};

using ASTPtr = unique_ptr<BaseAST>;
using ASTPtrList = vector<ASTPtr>;

//The list for all compunits
class ProgAST: public BaseAST{
    public:
    void AppendCompUnit(ASTPtr _unit){
        CompUnits.push_back(move(_unit));
    }

    private:
    ASTPtrList CompUnits;
};

// variable declaration
class VarDeclAST: public BaseAST{
    public:
    VarDeclAST(ASTPtr _type, ASTPtrList _defs):type(move(_type)), defs(move(_defs)){}

    // visitor
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    ASTPtrList& GetDefs(){ return defs; }
    Type GetType() {return type->GetType();}

    private:
    ASTPtr type;
    ASTPtrList defs;
};

class ConstIntTypeAST:public BaseAST{
    public:
    ConstIntTypeAST(Type _type):type_(_type){}

    // visitor
    void print(ofstream& ofs){ ofs << "const int"; }
    bool IsConst(){ return true; }
    bool IsInt(){ return true; }
    bool IsVoid(){ return false; }

    // getter
    Type GetType(){return type_; }

    private:
    Type type_;
};

class IntTypeAST:public BaseAST{
    public:
    IntTypeAST(Type _type):type_(_type){}

    //visitor
    void print(ofstream& ofs){ ofs << "int"; }
    bool IsConst(){ return false; }
    bool IsInt(){ return true; }
    bool IsVoid(){ return false; }

    //getter
    Type GetType(){return type_; }

    private:
    Type type_;
};

class VoidTypeAST:public BaseAST{
    public:
    VoidTypeAST(Type _type):type_(_type){}

    //visitor
    void print(ofstream& ofs){ ofs << "void";}
    bool IsConst(){ return false; }
    bool IsInt(){ return false; }
    bool IsVoid(){ return true; }

    //getter
    Type GetType(){return type_; }

    private:
    Type type_;
};

class VarDefAST: public BaseAST{
    public:
    VarDefAST(string& _ident, ASTPtrList _dims, ASTPtr _init_val):
    ident_(_ident), dims_(move(_dims)), init_val_(move(_init_val)){}

    // visitor
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _ir_gen);
    bool IsArray(){ return !dims_.empty(); }
    bool IsInitialized(){ return (init_val_ != nullptr); }

    // getter
    string& GetIdent(){ return ident_; }
    ASTPtrList& GetDims() {return dims_; }
    ASTPtr& GetInitVal() {return init_val_; }
    vector<int>& GetDimNum() { return dim_num_;}
    vector<int>& GetAccNum() { return acc_num_; }

    //calculate dims, tot_dim, accmulate_dim
    int GetTotDimNum(IRGenerator& _irgen); 
    
    private:
    string ident_;
    ASTPtrList dims_;
    // e.g. a[2][3][4]
    vector<int> dim_num_; // 2 3 4 1
    vector<int> acc_num_; // 24 12 4 1
    int tot_dim_num_; // 24

    ASTPtr init_val_;
};


//for array
class InitValAST: public BaseAST{
    public:
    InitValAST(ASTPtrList _init_vals):init_vals_(move(_init_vals)){}
    
    // visitor
    void print(ofstream& ofs);
    bool IsLiteral() {return false;}
    //ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    ASTPtrList& GetInitVals() {return init_vals_; }
    int HandleInitVal(IRGenerator& _irgen, shared_ptr<ArrayVal> _val_ptr, int _cur_dim, int _cur_index);
    int HandleInitInt(IRGenerator& _irgen, shared_ptr<ArrayVal> _val_ptr, int _cur_dim, int _cur_index);

    private:
    ASTPtrList init_vals_;
};

/**
 * The AST of function
 * 
*/

class FuncDeclAST: public BaseAST{
    public:
    FuncDeclAST(string& _ident, ASTPtrList _paras, ASTPtr _type):
    ident_(_ident), paras_(move(_paras)), type_(move(_type)){}
    
    // visitor
    void print(ofstream& ofs);
    // do nothing
    ValPtr GenerateIR(IRGenerator& _irgen){
        return nullptr;
    }

    // getter
    string& GetIdent(){ return ident_; }
    ASTPtrList& GetParas(){return paras_; }
    Type GetType() {return type_->GetType();}

    private:
    string ident_;
    ASTPtrList paras_;
    ASTPtr type_;
};


class FuncDefAST: public BaseAST{
    public:
    FuncDefAST(string& _ident, ASTPtrList _paras, ASTPtr _block, ASTPtr _type):
    ident_(_ident), paras_(move(_paras)), block_(move(_block)), type_(move(_type)){}

    // visitor 
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    string& GetIdent() {return ident_;}
    ASTPtrList& GetParas() { return paras_; }
    ASTPtr& GetBlock() {return block_;}
    int GetParaNum() {return paras_.size(); }
    Type GetType() {return type_->GetType();}


    private:
    string ident_;
    ASTPtrList paras_;
    ASTPtr block_; //declaration, nullptr
    ASTPtr type_;
};

class BlockAST: public BaseAST{
    public:
    BlockAST(ASTPtrList _block_items):block_items_(move(_block_items)){}
    
    // visitor
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _irgen);

    //getter
    const ASTPtrList& GetStmts(){return block_items_;}

    private:
    ASTPtrList block_items_;
};

class FuncParaAST: public BaseAST{
    public:
    FuncParaAST(ASTPtr _type, string& _ident, ASTPtrList _dims):
    type_(move(_type)), ident_(_ident), dims_(move(_dims)){}
    
    // visitor
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    string GetIdent(){ return ident_; }
    ASTPtrList& GetDims(){ return dims_; }

    private:
    ASTPtr type_;
    string ident_;
    ASTPtrList dims_;
};


/**
 *  The AST of stmt
*/
//4.19 check
class IfElseAST: public BaseAST{
    public:
    //no else then, set it as nullptr
    IfElseAST(ASTPtr _cond, ASTPtr _then, ASTPtr _else_then):
             cond_(move(_cond)), then_(move(_then)), else_then_(move(_else_then)){}

    // visitor
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    ASTPtr& GetCond(){ return cond_; }
    ASTPtr& GetThen() {return then_; }
    ASTPtr& GetElseThen() {return else_then_; }
    bool IsElseThen() {return else_then_ != nullptr;}

    private:
    ASTPtr cond_;
    ASTPtr then_;
    ASTPtr else_then_;

};

class WhileAST: public BaseAST{
    public:
    WhileAST(ASTPtr _cond, ASTPtr _stmt):
    cond_(move(_cond)), stmt_(move(_stmt)){}

    //visitor
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    ASTPtr& GetCond(){return cond_;}
    ASTPtr& GetStmt() {return stmt_;}

    private:
    ASTPtr cond_;
    ASTPtr stmt_;
};

class ControlAST: public BaseAST{
    public:
    ControlAST(Keyword _type, ASTPtr _expr):
    type_(_type), expr_(move(_expr)){}

    //visitor
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _irgen);

    //getter
    Keyword GetControlType(){return type_;}
    ASTPtr& GetExpr() {return expr_;}

    private:
    Keyword type_;
    ASTPtr expr_;
};

class BlankAST: public BaseAST{
    public:
    //do nothing
};

class FuncCallAST: public BaseAST{
    public:
    FuncCallAST(string& _ident,int _line, ASTPtrList _paras):
    ident_(_ident), line_(_line), paras_(move(_paras)){}
    
    //visitor
    void print(ofstream& ofs);
    ValPtr GenerateIR(IRGenerator& _irgen);

    //getter
    string& GetIdent(){return ident_;}
    ASTPtrList& GetParas() {return paras_;}
    int GetLine() {return line_;}
    //bool IsLiteral() {return true;}

    void SetLine(int _line){line_ = _line;}
    
    private:
    string ident_;
    ASTPtrList paras_;
    int line_;
};

class AssignAST: public BaseAST{
    public:
    AssignAST(ASTPtr _lval, ASTPtr _val):
    lval_(move(_lval)), val_(move(_val)){}

    private:
    ASTPtr lval_;
    ASTPtr val_;
};

/*
    expr
*/
class LValAST: public BaseAST{
    public:
    LValAST(string& _ident, ASTPtrList _index):
    ident_(_ident), index_(move(_index)){ index_num_ = -1;}
    
    // visitor
    void print(ofstream& ofs);
    int GenerateInt(IRGenerator& _irgen);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    string& GetIdent(){return ident_;}
    ASTPtrList& GetIndex() {return index_;}
    bool IsLiteral(){return true;}
    //int GetIndexNum(IRGenerator& _irgen);

    private:
    string ident_;
    ASTPtrList index_;
    int index_num_; //record the calculated index, initialized with -1
    int val_;
};

class ExpAST: public BaseAST{
    public:
    
};

class UnaryExpAST: public ExpAST{
    public:
    UnaryExpAST(Operator _unary_op, ASTPtr _expr):
    unary_op_(_unary_op), expr_(move(_expr)){}
    
    // visitor
    void print(ofstream& ofs);
    int GenerateInt(IRGenerator& _irgen);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    Operator& GetOp(){return unary_op_;}
    ASTPtr& GetExpr() {return expr_;}
    bool IsLiteral() {return true;}

    private:
    Operator unary_op_;
    ASTPtr expr_;
    int val_;
};

class BinaryExpAST: public ExpAST{
    public:
    BinaryExpAST(ASTPtr _lhs, ASTPtr _rhs, Operator& _op):
                op_(_op), lhs_(move(_lhs)), rhs_(move(_rhs)){}

    // visitor
    void print(ofstream& ofs);
    int GenerateInt(IRGenerator& _irgen);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    Operator& GetOp(){return op_;}
    ASTPtr& GetLhs() {return lhs_;}
    ASTPtr& GetRhs() {return rhs_;}
    bool IsLiteral() {return true;}

    private:
    Operator op_;
    ASTPtr lhs_;
    ASTPtr rhs_;
    int val_;
};

class IntAST: public ExpAST{
    public:
    IntAST(int _val):val_(_val){}
    
    // visitor
    void print(ofstream& ofs){ ofs << val_; }
    int GenerateInt(IRGenerator& _irgen);
    ValPtr GenerateIR(IRGenerator& _irgen);

    // getter
    int GetVal(){ return val_;}
    bool IsLiteral() {return true;}

    private:
    int val_;
};

class CharAST: public ExpAST{
    public:
    CharAST(char _val):val(_val){}

    void print(ofstream& ofs){ ofs << val; }

    int Val(){ return val;}

    private:
    char val;
};

class StringAST: public BaseAST{
    public:
    StringAST(string& _str):str(_str){}
    void print(ofstream& ofs){ ofs << str; }

    private:
    string str;
};

#endif
