#include "regalloc.hpp"
#include "cfg.hpp"
#include <list>
using namespace std;

void FuncSet::Dump(ofstream& ofs){
    for(auto func: funcs_){
        func->DumpBlock(ofs);
    }
}
void FuncSet::BuildBlockGragh(){
    for(auto func: funcs_){
        func->BuildBlockGragh();
    }
}
void FuncSet::LoopAnalyzeOn(){
    for(auto func: funcs_){
        func->cfg_analyzer_->LoopAnalyzeOn();
    }
}
void FuncSet::SortBlocks(){
    for(auto func: funcs_){
        func->cfg_analyzer_->ComputeBlockOrder(func->blocks_[0]);
    }
}

void FuncSet::RegisterAllocate(){
    for(auto func: funcs_){
        func->cfg_analyzer_->RegisterAllocate();
    }
}

// 开始循环分析
void CFGAnalyzer::LoopAnalyzeOn(){
    for(auto block: func_->GetBlocks()){
       MarkLoopIndex(block); 
    }
    BackwardLoopIndex();
    UpdateLoopDepth();
}

// 递归记录loop index
void CFGAnalyzer::MarkLoopIndex(BlockPtr cur_block){
    // 每一个block只遍历一次
    if(cur_block->IsVisited())
            return;

    cur_block->SetVisit();  // set visit
    cur_block->SetActive(); // set active
    auto inst = cur_block->GetLastInst();
    if(inst == nullptr)
        return;
    
    // 最后的指令是分支指令或者跳转指令
    if(inst->IsBranch()){
        auto branch_block = func_->GetLabeledBlock(inst->GetGotoLabelID());
        // 向上回溯
        if(branch_block->IsActive()){ // must be active ?
            // 回溯的block标记为loop header
            if(!branch_block->IsLoopHeader()){ // 首次被标记为loop header
                branch_block->SetLoopHeader();
                branch_block->SetLoopIndex(AttributeLoopIndex());
            }
            // 将当前block的loop index设置为header的loop index
            cur_block->SetLoopEnd();
            cur_block->SetLoopIndex(branch_block->GetLoopIndex());
            // 将loop end收集起来用作loop index集合的标记
            AddLoopEnd(cur_block);
        }
    }

    // 直接处理cur_block的后继
    for(auto block: cur_block->GetSucs()){
        MarkLoopIndex(block);
    }     
    cur_block->UnsetActive();
}

// 从每一个loop end向前回溯，添加loop index
// 向前回溯包括了任意可跳转到其的指令
void CFGAnalyzer::BackwardLoopIndex() {
    for(auto loop_end: loop_ends_){
        auto loop_header = loop_end->GetBackward();
        int loop_index = loop_header->GetLoopIndex();
        // 将loop index先加入loop header和loop end的index集合
        loop_header->AddLoopIndex(loop_index);
        loop_end->AddLoopIndex(loop_index);
        BackwardOnce(loop_end, loop_header, loop_index);
    }
}

// 标记index 集合，从loop end向上，直到loop header，将loop index加入途径的
// block loop index集合。相当于标记了该loop当中的每一个block
void CFGAnalyzer::BackwardOnce(BlockPtr _block, BlockPtr _loop_header, int _loop_index){
   //这里active和visit作用差不多
    _block->SetActive();
    for(auto pred: _block->GetPred()){
        if(pred->IsActive() || pred == _loop_header) continue;
        pred->AddLoopIndex(_loop_index);
        BackwardOnce(pred, _loop_header, _loop_index);
    }
    // 这样能避免循环，但某些处于多个循环中的block还是会被多访问
    // 后期会想办法优化一下
    _block->UnsetActive();
    return;
}

void CFGAnalyzer::UpdateLoopDepth(){
    for(auto block: func_->GetBlocks()){
        block->SetLoopDepth();
    }
}

// 计算控制块的顺序
void CFGAnalyzer::ComputeBlockOrder(BlockPtr first_block){
    list<BlockPtr> work_list;
    work_list.push_back(first_block);
    list<BlockPtr>::iterator it;

    while(!work_list.empty()){
        auto cur_block = work_list.back();
        work_list.pop_back();
        //cout << cur_block->GetBlockID() << "\n";
        ordered_blocks_.push_back(cur_block);

        for(auto sucs_block: cur_block->GetSucs()){
            // 如果时回溯边，不考虑
            if(cur_block->GetBackward() == sucs_block) continue;
            sucs_block->DecForwardBlockNum();
            // 待处理前驱--
            // 前驱都已经被处理，加入worklist等待处理
            if(sucs_block->GetForwardBlockNum() == 0){  
                for(it = work_list.begin(); it != work_list.end(); it++){
                    // 保持有序性，在第一个大于其的元素前加入
                    if((*it)->GetLoopDepth() > sucs_block->GetLoopDepth()){
                        work_list.insert(it, sucs_block);
                            break;
                    }
                }
                if(it == work_list.end()){
                    work_list.push_back(sucs_block);
                }
            }
        }
    }
    return;
}

void CFGAnalyzer::RegisterAllocate(){
    lsra_ = make_shared<LSRAMachine>(func_, ordered_blocks_);
    lsra_->Pass();
}