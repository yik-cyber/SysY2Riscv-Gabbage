#ifndef CFG_HPP
#define CFG_HPP
#include "eeyore.hpp"
#include "regalloc.hpp"
#include <set>
using namespace std;

// 托管所有函数，提供外部接口
class FuncSet{
    public:
    void AddFunc(EFuncDefPtr _func) {funcs_.push_back(move(_func));}
    void Dump(ofstream& ofs);  
    void BuildBlockGragh();
    void LoopAnalyzeOn();
    void SortBlocks();

    void RegisterAllocate();


    //private:
    vector<EFuncDefPtr> funcs_;
};

// 操作变量的定义：局部定义的临时变量和原生非数组变量
// 数组变量必须存储在栈上

// 控制流程图分析
class CFGAnalyzer{
public:
    CFGAnalyzer(EFuncDefPtr _func):func_(move(_func)){
        next_loop_index_ = 1;
    }
    // 循环分析
    void LoopAnalyzeOn();
    void AddLoopEnd(BlockPtr _loop_end){loop_ends_.push_back(move(_loop_end));} 
    void MarkLoopIndex(BlockPtr cur_block);
    void BackwardOnce(BlockPtr _block, BlockPtr _loop_header, int _loop_index);
    void BackwardLoopIndex();
    void UpdateLoopDepth();
    int AttributeLoopIndex() {return next_loop_index_++;}

    // block排序
    void ComputeBlockOrder(BlockPtr first_block);

    //寄存器分配
    void RegisterAllocate();

private:
    EFuncDefPtr func_;
    LSRAMachinePtr lsra_;
    int next_loop_index_;

    BlockPtrList loop_ends_;
    BlockPtrList ordered_blocks_;
};
using CFGAnalyzerPtr = shared_ptr<CFGAnalyzer>;
using CFGAnalyzerPtrList = vector<CFGAnalyzerPtr>;



#endif //CFG_HPP
