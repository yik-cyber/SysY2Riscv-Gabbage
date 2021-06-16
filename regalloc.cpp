#include "regalloc.hpp"
#include <cstring>
using namespace std;

int Interval::next_id_ = 0;
string real_reg_names[] = {
        "a0", "a1", "a2", "a3", "a4", "a5", "a6",
        "a7",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", //15

        "s0", "s1", "s2", "s3", "s4", "s5", "s6",
        "s7", "s8",
};

string reserve_reg_names[] = {
        "s9", "s10", "s11",
        "x0",
};
void Interval::AddUsePos(int _pos, UseType _use_type){
    auto use_pos = make_shared<UsePos>(_pos, _use_type);
    use_pos_.push_back(move(use_pos));
}

void Interval::SetSpillStoreUse(int _pos, bool _is_use){
    // inst pos
    if(_is_use){
        //pre store只会出现在这种情况
        spill_store_pos_ = _pos - 1; 
    }
    else{
        spill_store_pos_ = _pos + 2;
    }
    //直接设置成_pos，调用的时候interval所属的变量
    //一定在当前指令中
    //use直接设置成吓一跳指令的pos，一旦>=use_pos就要加载
    spill_use_pos_ = _pos + 5;
}

void LSRAMachine::SetStack(){
    stack_size = func_->GetStackSize();
}

int LSRAMachine::AllocStack(){
    int s = func_->GetStackSize();
    func_->AddStackSize(1);
    return s;
}

void LSRAMachine::BuildReg(){
    int i = 0; //参数不参与分配
    for(; i < func_->GetParamNum(); ++i){
        auto reg = make_shared<Register>(real_reg_names[i], i);
        reg->free_ = false;
        regs_map_[real_reg_names[i]] = reg;
        reg_list_.push_back(reg);
    }
    for(; i < 24; ++i){
        auto reg = make_shared<Register>(real_reg_names[i], i);
        regs_map_[real_reg_names[i]] = reg;
        reg->free_ = true;
        reg_list_.push_back(reg);
        free_reg_.push_back(reg);
    }
    for(i = 0; i < 4; ++i){
        auto reg = make_shared<Register>(real_reg_names[23+i], 23+i);
        regs_map_[real_reg_names[23+i]] = reg;
        reg->free_ = true;
        reg_list_.push_back(reg);
    }
    memset(s_regs_, 0, sizeof(s_regs_));
}

void LSRAMachine::LocalLiveSet(){
    int pos_ = 0;
    for(auto block: blocks){
        for(auto inst: block->inst_){
            inst->pos_ = pos_;
            pos_ += 5;
            if(!inst->NeedReg()) continue;  //check
            auto pair = inst->VisitOpr();
            auto defs = pair.first;
            auto uses = pair.second;
            for(auto use: uses){
                // 在某条指令中被使用但未在该block中赋值，一定是从前面的block存活到当前block的
                if(use->NeedAlloc()){ //check
                    if(block->live_kill_.find(use) == block->live_kill_.end()){
                        block->live_gen_.insert(use);
                    }
                }
            }

            for(auto def: defs){
                // 在当前block被赋值，直接加入live kill
                if(def->NeedAlloc()){
                    block->live_kill_.insert(def);
                }
            }
        }
        // live in 初始化为 live gen，只包含了在block中出现的变量
        block->live_in_ = block->live_gen_;
        block->live_out_.clear();
    }
} 

void LSRAMachine::GlobalLiveSet(){
    // 考虑到回溯block也属于其后继，所以需要pass多遍来确保live out计算的完备性
    bool changed = true;
    while(changed){
        changed = false;
        for(int i = blocks.size() - 1; i >= 0; i--){
            auto block = blocks[i];
            set<EValPtr> new_out;
            for(auto sucs: block->GetSucs()){
                // 将后继block的live_in加入当前block的live_out
                if(sucs){
                    new_out.insert(sucs->live_in_.begin(), sucs->live_in_.end());
                } 
            }

            if(new_out != block->live_out_){
                changed = true;
                block->live_out_ = new_out;
                // 更新live in，将live out中不属于live kill中的元素加入 live in
                for(auto opr: block->live_out_){
                    if(block->live_kill_.find(opr) == block->live_kill_.end()){
                        block->live_in_.insert(opr);
                    }
                }
            }
        }        
    } 
}


void LSRAMachine::BuildInterval(){
    set<IntervalPtr> collect;
    for(auto it = blocks.rbegin(); it != blocks.rend(); ++it){
        auto block = *it;
        if(block->inst_.empty())
            continue;
        auto block_from = block->inst_.front()->pos_;
        block->from_ = block_from;

        auto block_to = block->inst_.back()->pos_ + 4; //last
        block->to_ = block_to;

        for(auto val: block->live_out_){
            if(val->NeedAlloc()){
                if(val->interval_ == nullptr){
                    val->interval_ = make_shared<Interval>();
                    val->interval_->var_ = val;
                    if(val->GetType() == 'p'){
                        val->interval_->reg_ = reg_list_[val->GetID()];
                    }
                    collect.insert(val->interval_);
                }
                val->interval_->from_ = block_from;
                if(val->interval_->to_ == 2e8)    //first
                    val->interval_->to_ = block_to;
            }
        }
        for(auto bit = block->inst_.rbegin(); bit != block->inst_.rend(); ++bit){
            auto inst = *bit;
            if(!inst->NeedReg()) continue;
            auto pair = inst->VisitOpr();
            auto defs = pair.first;
            auto uses = pair.second;
            
            //因为指令是从后向前，所以先def后use
            for(auto def: defs){
                if(!def->NeedAlloc()) continue;
                auto in = def->interval_;
                if(in == nullptr){
                    // first def
                    in = make_shared<Interval>(); 
                    in->var_ = def;
                    if(def->GetType() == 'p'){
                        in->reg_ = reg_list_[def->GetID()];
                    }
                    collect.insert(in);
                    def->interval_ = in;
                }
                if(in->from_ == -1){ //first
                    in->from_ = inst->pos_ + 1;
                }
                else{
                    in->from_ = min(in->from_, inst->pos_ + 1);
                }
                if(in->to_ == 2e8){
                    in->to_ = inst->pos_ + 1;
                }
                else{
                    in->to_ = max(in->to_, inst->pos_ + 1);
                }  
                in->AddUsePos(inst->pos_ + 1, UseType::Def);
            }

            for(auto use: uses){
                if(!use->NeedAlloc()) continue;
                auto in = use->interval_;
                if(in == nullptr){
                    // last use
                    in = make_shared<Interval>();
                    in->var_ = use;
                    if(use->GetType() == 'p'){
                        int id = use->GetID();
                        in->reg_ = reg_list_[id];
                    } 
                    collect.insert(in);
                    use->interval_ = in;
                }
                if(in->from_ == -1){
                    in->from_ = inst->pos_;
                }
                else{
                    in->from_ = min(in->from_, inst->pos_);
                }
                if(in->to_ == 2e8){
                    in->to_ = inst->pos_;
                }
                else{
                    in->to_ = max(in->to_, inst->pos_);
                } 
                in->AddUsePos(inst->pos_, UseType::Use);
            }
        }
    }
    for(auto in: collect){
        if(!in->reg_) //起初没有分配，非参数变量
            AddUnhandledList(in);
        else
            AddActiveList(in);
    }   
}


// increase with end point
void LSRAMachine::ExpireOldIntervals(int pos){
    for(auto it = active_list_.begin(); it != active_list_.end(); ){
        auto in = *it;
        if(in->to_ >= pos)
            return;
        else{
            // to < pos, expire
            in->reg_->free_ = true;
            free_reg_.push_back(in->reg_);
            it = active_list_.erase(it);
        }
    }
}

// spill pos记录的是使用溢出值的开始位置
void LSRAMachine::SpillAtInterval(IntervalPtr cur){
    auto spill = active_list_.back();
    if(spill->to_ > cur->to_){
        //在cur开始后分裂，分裂出来后就完全放栈上
        active_list_.pop_back();
        cur->reg_ = spill->reg_; //状态没有改变
        // spill use
        bool store_flag = false, use_flag = false;
        for(auto use: spill->use_pos_){
            //从后向前。最靠近cur from的store pos
            if(use->pos_ < cur->from_ && use->type_ == UseType::Def){
                //对spill赋值后存储
                spill->spill_store_pos_ = use->pos_ + 1;
                store_flag = true;
                break;
            }
        }
        for(auto it = spill->use_pos_.rbegin(); it != spill->use_pos_.rend(); ++it){
            //从前向后，寻找>cur from的第一个use pos
            auto use = *it;
            if(use->pos_ > cur->from_ && use->type_ == UseType::Use){
                spill->spill_use_pos_ = use->pos_;
                use_flag = true;
                break;
            }
        }
        //不管怎样只要不是数组就留一个栈
        if(!spill->var_->IsArray()){
            spill->spill_stack_ = AllocStack();
        }      
        AddActiveList(cur);
    }
    else{
        //spilt itself
        cur->spill_stack_ = AllocStack();
        bool store_flag = false, use_flag = false;
        for(auto it = spill->use_pos_.rbegin(); it != spill->use_pos_.rend(); ++it){
            //从前向后，寻找第一个use和def点
            auto use = *it;
            if(!store_flag && use->type_ == UseType::Def){
                assert(use->pos_ % 5 == 1);
                cur->spill_store_pos_ = use->pos_ + 1;
                store_flag = true;
            }
            if(!use_flag && use->type_ == UseType::Use){
                assert(use->pos_ % 5 == 0);
                cur->spill_use_pos_ = use->pos_;
                use_flag = true;
            }
            if(use_flag && store_flag)
                break;
        }    
    }
}

void LSRAMachine::AddActiveList(IntervalPtr in){
    for(auto it = active_list_.begin(); it != active_list_.end(); ++it){
        if((*it)->to_ > in->to_){
            active_list_.insert(it, in);
            return;
        }
    }
    active_list_.push_back(in);
}

void LSRAMachine::AddUnhandledList(IntervalPtr in){
    for(auto it = unhandled_list_.begin(); it != unhandled_list_.end(); ++it){
        if((*it)->from_ > in->from_){
            unhandled_list_.insert(it, in);
            return;
        }
    }
    unhandled_list_.push_back(in);    
}

void LSRAMachine::AllocOn(){
    while(!unhandled_list_.empty()){
        auto in = unhandled_list_.front();
        unhandled_list_.pop_front();
        if(in == nullptr)
            continue; //cc

        ExpireOldIntervals(in->from_);
        
        bool allocated = false;
        if(free_reg_.size() > 0){
            while(free_reg_.size() > 0){
                if(allocated) 
                    break;
                auto reg = free_reg_.back();               
                free_reg_.pop_back();
                if(!reg->free_) continue;
                reg->free_ = false;
                in->reg_ = reg;
                if(reg->ident_[0] == 's'){
                    s_regs_[reg->ident_[1] - '0'] = true;
                }
                allocated = true;
                AddActiveList(in);
                break;
            }          
        }            
        if(!allocated)
            SpillAtInterval(in);
    }
}

void LSRAMachine::Resolving(){
    for(auto block: blocks){
        auto forward_block = block->GetForward();
        for(auto inst: block->inst_){
            inst->Resolving(block, forward_block);
        }  
    }
    //进入函数时要保存的寄存器
    if(func_->GetIdent() == "main")
        return; //但是main函数不用坐这一步
    int cnt = 0;
    for(int i = 0; i < 9; ++i){
        if(s_regs_[i]){
            func_->reserved_regs.push_back(reg_list_[15+i]);
            cnt++;
        }    
    }
    func_->reserved_reg_stack_ = func_->GetStackSize();
    func_->AddStackSize(cnt);
}

void LSRAMachine::Resolving2(){
    for(auto block: blocks){
        auto backward = block->backward_;
        if(backward != nullptr){
            for(auto item: block->live_in_){
                if(backward->live_out_.find(item) != backward->live_out_.end()){
                    auto in = item->interval_;
                    if(in == nullptr) continue; //cc
                    if(in->spill_store_pos_ == 2e8 || in->spill_use_pos_ == 2e8)
                        continue;
                    if(in->spill_store_pos_ >= backward->from_ - 1){
                        for(auto use: in->use_pos_){
                            if(use->pos_ < backward->from_ && use->type_ == UseType::Def){
                                in->spill_store_pos_ = use->pos_ + 1;
                                break;
                            }       
                        }
                    }
                    if(in->spill_use_pos_ > backward->from_)
                        in->spill_use_pos_ = backward->from_;
                }
            }
        }
    }
}

// a0 - t7是调用者保存的寄存器
void LSRAMachine::HandleCall(){
    set<EValPtr> active_var_;
    set<RegisterPtr> active_regs_;
    for(auto it_1 = blocks.rbegin(); it_1 != blocks.rend(); ++it_1){
        auto block = *it_1;
        for(auto item: block->live_out_){
            active_var_.insert(item);
        }
        for(auto it_2 = block->inst_.rbegin(); it_2 != block->inst_.rend(); ++it_2){
            auto inst = *it_2;
            if(!inst->NeedReg()) continue;
                auto pair = inst->VisitOpr();
                auto defs = pair.first;
                auto uses = pair.second;
            
            for(auto def: defs){
                if(!def->NeedAlloc()) continue;
                auto it = active_var_.find(def);
                if(it != active_var_.end()){
                    active_var_.erase(it);
                }
            }

            if(inst->type_ == EInstType::Call){
                for(auto item: active_var_){
                    if(item->interval_->reg_->id_ > 14) 
                        continue;
                    if(item->interval_->spill_store_pos_ > inst->pos_){
                        inst->SetReservePos(func_->GetStackSize());
                        inst->AddReserveReg(item->interval_->reg_);
                    }
                }
            }

            for(auto use: uses){
                if(!use->NeedAlloc()) continue;
                active_var_.insert(use);
            }   
        }
        active_var_.clear();
    } 
}


void LSRAMachine::Pass(){
    SetStack();
    BuildReg();

    LocalLiveSet();
    GlobalLiveSet();
    BuildInterval();

    AllocOn();

    Resolving();
    Resolving2();
    //遍历inst，顺便记录active set，如果到call仍然active并且之后还要用到，就要记录，放到栈上
    //如果没有栈位置还要分配一个栈位置
    //对于参数，如果分配的寄存器不是ai还要进行一次赋值
    //结束时恢复寄存器的值
    HandleCall();
    func_->AddStackSize(16);
    //对于return，如果a0分配的不是a0，还要进行一次赋值再跑路
    //return前还要恢复callee保存的寄存器
    //HandleReturn();
    //除main函数要统计用了哪些callee保存的寄存器，在进入时要压栈，return时要恢复
    //HandleComin();
}