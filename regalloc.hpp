#ifndef REGALLOC_HPP_
#define REGALLOC_HPP_
#include <queue>
#include <list>
#include "eeyore.hpp"
using namespace std;

/**
 * 定义寄存器
*/

class Interval;
using IntervalPtr = shared_ptr<Interval>;
//用链表存储interval list
using IntervalPtrList = list<IntervalPtr>;


enum class RegType{
    x, // x0 存储 0
    t, // 调用者保存寄存器
    s, // 被调用者保存寄存器
    a  // 传递函数参数的寄存器
};

class Register{
    public:
    Register(){}
    Register(const string& _ident, int _id):
    ident_(_ident), id_(_id){}
    
    bool free_ = true;
    string ident_;
    int id_; //标号
};
using RegisterPtr = shared_ptr<Register>;
using RegisterPtrList = vector<RegisterPtr>;

enum class UseType{
    Use,
    Def,    //赋值
};

class UsePos{
    public:
    UsePos(int _pos, UseType _type):
    pos_(_pos), type_(_type){}

    bool spill_flag_ = false;
    int pos_;
    UseType type_;
};
using UsePosPtr = shared_ptr<UsePos>;
using UsePosPtrList = vector<UsePosPtr>;

class Interval{
    public:
    Interval(){
        id_ = next_id_++;
    }
    void Dump(ofstream& ofs){
        ofs << "(";
        ofs << reg_->ident_;
        ofs << ")";
    }
    void AddUsePos(int _pos, UseType _use_type);
    bool Spilled(int _pos){return _pos > spill_store_pos_;}
    //找 <= pos的第一次赋值
    void SetSpillStoreUse(int _pos, bool _is_use);
    
    static int next_id_;
    int id_;
    int from_ = -1;
    int to_ = 2e8;
    // store pos和use pos都对应一个inst的开始pos
    int spill_store_pos_ = 2e8;
    int spill_use_pos_ = 2e8;   //use stack from this

    int spill_stack_ = -1;
    
    EValPtr var_ = nullptr;        // 对应的变量
    RegisterPtr reg_ = nullptr;   //分配的寄存器
    
    //从后往前记录使用的inst pos
    UsePosPtrList use_pos_;
};

class LSRAMachine{
    public:
    LSRAMachine(EFuncDefPtr _func, BlockPtrList _blocks):
    func_(move(_func)), blocks(move(_blocks)){}
    void Pass();

    void SetStack();
    void BuildReg();
    //void SetFreeReg();
    
    void LocalLiveSet();
    void GlobalLiveSet();
    void BuildInterval();
    
    void AllocOn();
    void ExpireOldIntervals(int pos);
    void SpillAtInterval(IntervalPtr cur);
    void AddActiveList(IntervalPtr cur);
    void AddUnhandledList(IntervalPtr in);

    int AllocStack();
    void Resolving();
    void Resolving2();

    void HandleCall();


    EFuncDefPtr func_;
    BlockPtrList blocks;

    int stack_size;
    IntervalPtrList active_list_;
    IntervalPtrList unhandled_list_;

    unordered_map<string, RegisterPtr> regs_map_;
    RegisterPtrList reg_list_;
    RegisterPtrList free_reg_;

    bool s_regs_[9];

    set<int> call_ret_pos_;
};
using LSRAMachinePtr = shared_ptr<LSRAMachine>;

#endif