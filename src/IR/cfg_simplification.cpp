#include "IR/cfg_semplification.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/instruction.hpp"
#include <utility>

namespace scbe::IR {

bool CFGSemplification::run(IR::Function* function) {
    bool ret = mergeBlocks(function);
    ret |= removeNoPredecessors(function);
    ret |= replaceEmpty(function);
    return ret;
}

bool CFGSemplification::mergeBlocks(IR::Function* function) {
    bool change = false;
    bool reset = false;
    do {
        reset = false;
        for(auto& block : function->getBlocks()) {
            if(!block->getInstructions().back()->isJump()) continue;
            JumpInstruction* jump = cast<JumpInstruction>(block->getInstructions().back().get());
            if(jump->getNumOperands() > 1) continue;
            Block* to = cast<Block>(jump->getOperand(0));
            if(to->getPredecessors().size() > 1 || function->getHeuristics().isLoop(to)) continue; // > 1 means this block has other predecessors
            block.get()->removeInstruction(jump);
            function->replace(to, block.get());
            function->mergeBlocks(block.get(), to);
            reset = true;
            change = true;
            break;
        }
    }
    while(reset);

    return change;
}

bool CFGSemplification::removeNoPredecessors(IR::Function* function) {
    std::vector<Block*> toRemove;

    for(auto& block : function->getBlocks()) {
        if(block.get() == function->getEntryBlock() || block->getPredecessors().size() > 0 || block->isTerminator()) continue;
        toRemove.push_back(block.get());
    }

    for(Block* block : toRemove)
        function->removeBlock(block);
    
    return toRemove.size() > 0;
}

struct EmptyEntry {
    Block* m_entry = nullptr;
    Block* m_target = nullptr;
    std::vector<IR::PhiInstruction*> m_phis;
};

bool CFGSemplification::replaceEmpty(IR::Function* function) {
    std::vector<EmptyEntry> toReplace;
    for(auto& block : function->getBlocks()) {
        if(block->getInstructions().size() != 1 || !block->getInstructions().back()->isJump()) continue;
        JumpInstruction* jump = cast<JumpInstruction>(block->getInstructions().back().get());
        if(jump->getNumOperands() > 1) continue;
        if(block.get() == function->getEntryBlock()) continue;

        if(block.get() == cast<Block>(jump->getOperand(0))) continue;

        EmptyEntry entry{block.get(), cast<Block>(jump->getOperand(0))};

        for(auto& instruction : entry.m_target->getInstructions()) {
            if(instruction->getOpcode() != IR::Instruction::Opcode::Phi) continue;
            auto phi = cast<PhiInstruction>(instruction.get());
            entry.m_phis.push_back(phi);
        }
        toReplace.push_back(std::move(entry));
    }

    bool anyChange = false;

    for(const EmptyEntry& entry : toReplace) {
        bool skip = false;
        for(IR::PhiInstruction* phi : entry.m_phis) {
            for(size_t i = 0; i < phi->getNumOperands(); i += 2) {
                IR::Block* block = cast<IR::Block>(phi->getOperands().at(i+1));
                if(!entry.m_entry->getPredecessors().contains(block)) continue;
                skip = true;
                break;
            }
            if(skip) break;
        }
        if(skip) continue;

        for(IR::PhiInstruction* phi : entry.m_phis) {
            IR::Value* clone = nullptr;
            for(size_t i = 0; i < phi->getNumOperands(); i += 2) {
                IR::Block* block = cast<IR::Block>(phi->getOperands().at(i+1));
                if(block != entry.m_entry) continue;
                clone = phi->getOperands().at(i);
                phi->removeOperand(entry.m_entry);
                break;
            }

            if(!clone) continue;

            for(auto& predecessor : entry.m_entry->getPredecessors()) {
                phi->addOperand(clone);
                phi->addOperand(predecessor.first);
            }
        }

        function->replace(entry.m_entry, entry.m_target);
        function->removeBlock(entry.m_entry);
        anyChange = true;
    }

    return anyChange;
}

}