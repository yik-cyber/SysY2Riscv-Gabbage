#include "global.hpp"
using namespace std;

string lib_func_ident[] = {
        "getint", "getch", "_sysy_starttime", "_sysy_stoptime",
        "getarray",
        "putint", "putch", "putarray",
};

void GlobalManager::AddLibFuns(){
    for(int i = 0; i < 2; i++){
        auto func = make_shared<EFuncDefVal>(lib_func_ident[i], 0, -1);
        func_table.insert({lib_func_ident[i], func});
    }
    for(int i = 2; i < 7; i++){
        auto func = make_shared<EFuncDefVal>(lib_func_ident[i], 1, -1);
        func_table.insert({lib_func_ident[i], func});
    }
    auto func = make_shared<EFuncDefVal>(lib_func_ident[7], 2, -1);
    func_table.insert({lib_func_ident[7], func});
}