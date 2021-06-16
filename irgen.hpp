#ifndef IRGEN_H_
#define IRGEN_H_

#include <ostream>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <cstddef>
#include "ast.hpp"
#include "ir.hpp"
using namespace std;

using FuncTabel = unordered_map<string, FuncDefPtr>;
using VarTabel = unordered_map<string, ValPtr>;
using VarTabelStack = vector<VarTabel>;

class IRGenerator {
  public:
  IRGenerator(){
    // create a virtual global function, and add it to the func tabel
    cur_func_ptr_ = make_shared<FuncDef>(Type::Void, "", 0);
    cur_func_ptr_->SetGlobal();
    cur_func_ptr_->ResetGlobalSlot();
    func_tabel_ = {};
    func_tabel_.insert({"", cur_func_ptr_});
    func_vector_.push_back(cur_func_ptr_);
    func_stack_.push_back(cur_func_ptr_);
    // var tabel
    var_tabel_stack_ = {{}}; //create global var tabel
    cur_var_tabel_it_ = var_tabel_stack_.end() - 1;
    var_tabel_num = 1;
    // lib functions
    lib_func_tabel_.insert({"getint",make_shared<FuncDef>(Type::Int, "getint", 0)});
    lib_func_tabel_.insert({"getch",make_shared<FuncDef>(Type::Int, "getch", 0)});
    lib_func_tabel_.insert({"getarray",make_shared<FuncDef>(Type::Int, "getarray", 1)});
    lib_func_tabel_.insert({"putint",make_shared<FuncDef>(Type::Void, "putint", 1)});
    lib_func_tabel_.insert({"putch",make_shared<FuncDef>(Type::Void, "putch", 1)});
    lib_func_tabel_.insert({"putarray",make_shared<FuncDef>(Type::Void, "putarray", 2)});
    lib_func_tabel_.insert({"starttime",make_shared<FuncDef>(Type::Void, "starttime", 0)});
    lib_func_tabel_.insert({"stoptime",make_shared<FuncDef>(Type::Void, "stoptime", 0)});
  }

  void Dump(stringstream& ofs);

  bool IsGlobal(){ return var_tabel_num == 1;}

  ValPtr GenerateOn(FuncDefAST& ast);
  ValPtr GenerateOn(FuncParaAST& ast);
  ValPtr GenerateOn(BlockAST& ast);
  ValPtr GenerateOn(VarDeclAST& ast);
  ValPtr GenerateOn(VarDefAST& ast);

  ValPtr GenerateOn(IfElseAST& ast);
  ValPtr GenerateOn(WhileAST& ast);
  ValPtr GenerateOn(ControlAST& ast);

  ValPtr GenerateOn(BinaryExpAST& ast);
  ValPtr GenerateOn(UnaryExpAST& ast);
  ValPtr GenerateOn(LValAST& ast);
  ValPtr GenerateOn(IntAST& ast);
  ValPtr GenerateOn(FuncCallAST& ast);
  
  // handle the expAST. Only used in decl, so do not need to generate inst
  // return IntVal
  // 理论上在全局声明变量时不会出现 a[fun(...)]，动态数组了
  //ValPtr CalculateVal(LValAST& ast);
  //ValPtr CalculateVal(UnaryExpAST& ast);
  //ValPtr CalculateVal(BinaryExpAST& ast);
  //ValPtr CalculateVal(IntAST& ast);

  // look up the table
  ValPtr GetVarVal(string& _ident);
  // look up the table
  FuncDefPtr GetFuncPtr(string& _ident);
  // 
  FuncDefPtr CurFuncPtr() {return cur_func_ptr_;}

  private:
  FuncTabel func_tabel_; //only global function
  vector<FuncDefPtr> func_vector_;
  vector<FuncDefPtr> func_stack_;
  FuncTabel lib_func_tabel_;

  VarTabelStack var_tabel_stack_;
  int var_tabel_num;

  VarTabelStack::iterator cur_var_tabel_it_;
  FuncDefPtr cur_func_ptr_;

  //cond check point
  vector<BranchPtr> branch_stack_;

  InstPtrList global_decls_;
};

#endif
