#ifndef GLOBAL_HPP_
#define GLOBAL_HPP_

#include "eeyore.hpp"
using namespace std;


using SymbolTable = unordered_map<string, EValPtr>;
using SymbolTableList = vector<SymbolTable>;
using FuncTable = unordered_map<string, EFuncDefPtr>;
using SymbolTablePtr = shared_ptr<SymbolTable>;

//管理全局指令和符号表
class GlobalManager{
    public:
    GlobalManager(){
        AddLibFuns();
        string name = "a0";
        auto a0 = make_shared<ERegVal>(name);
        table_.insert({name, a0});
    }
    void AddTable(string& _ident, EValPtr _val){
        table_.insert({_ident, _val});
    }
    void AddFunc(string& _ident, EFuncDefPtr _func){
        func_table.insert({_ident, _func});
    }
    EValPtr GetVal(const string& _ident){
        if(table_.find(_ident) != table_.end()){
            return table_[_ident];
        }
        return nullptr;
    }
    EFuncDefPtr GetFunc(const string& _ident){
        if(func_table.find(_ident) != func_table.end())
            return func_table[_ident];
        return nullptr;
    }
    void PushInst(EInstPtr _inst){inst_.push_back(move(_inst));}
    void AddLibFuns();
    EInstPtrList& GetGlobalInst() {return inst_;}


    private:
    SymbolTable table_;    //全局符号表
    FuncTable func_table;
    EInstPtrList inst_;     //全局指令
};

#endif