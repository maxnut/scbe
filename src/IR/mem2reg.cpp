#include "IR/mem2reg.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/dominator_tree.hpp"
#include "IR/instruction.hpp"
#include "IR/value.hpp"
#include "type.hpp"

#include <queue>

namespace scbe::IR {

bool Mem2Reg::run(IR::Function* function) {
    std::vector<IR::AllocateInstruction*> promoted;
    for(auto alloca : function->getAllocations()) {
        if(!isAllocaPromotable(alloca))
            continue;
        promoted.push_back(alloca);

        auto defining = allocaDefiningBlocks(alloca);
        
        USet<IR::Block*> idf;
        std::queue<IR::Block*> queue;
        for(auto block : defining)
            queue.push(block);

        while(!queue.empty()) {
            auto block = queue.front();
            queue.pop();
            if(!function->getDominatorTree()->hasDominanceFrontiers(block)) continue;
            for(auto bb : function->getDominatorTree()->getDominanceFrontiers(block)) {
                if(idf.contains(bb)) continue;
                idf.insert(bb);
                queue.push(bb);
            }
        }

        for(IR::Block* need : idf) {
            PointerType* pointerType = (PointerType*)alloca->getType();
            std::unique_ptr<IR::PhiInstruction> phi = std::make_unique<IR::PhiInstruction>(pointerType->getPointee(), alloca);
            need->setPhiForValue(alloca, phi.get());
            
            IR::Instruction* lastPhi = nullptr;
            for(auto it = need->getInstructions().rbegin(); it != need->getInstructions().rend(); ++it) {
                auto& instruction = *it;
                if(instruction->getOpcode() == IR::Instruction::Opcode::Phi) {
                    lastPhi = instruction.get();
                    break;
                }
            }
            
            if(lastPhi) {
                need->addInstructionAfter(std::move(phi), lastPhi);
                continue;
            }
            need->addInstructionAtFront(std::move(phi));
        }
    }

    UMap<IR::Value*, std::vector<IR::Value*>> stack;
    rename(function->getDominatorTree(), function->getEntryBlock(), stack, promoted);

    for(IR::AllocateInstruction* alloca : promoted)
        alloca->getParentBlock()->removeInstruction(alloca);
    
    return promoted.size() > 0;
}

void Mem2Reg::rename(IR::DominatorTree* tree, IR::Block* current, UMap<IR::Value*, std::vector<IR::Value*>>& stack, const std::vector<IR::AllocateInstruction*>& promoted) {
    UMap<IR::AllocateInstruction*, size_t> stackSize;
    for(IR::AllocateInstruction* alloca : promoted) {
        stackSize[alloca] = stack[alloca].size();
    }

    std::vector<IR::Instruction*> toRemove;
    for(auto& instruction : current->getInstructions()) {
        if(instruction->getOpcode() == IR::Instruction::Opcode::Store && wasAllocaUsed(promoted, instruction.get())) {
            stack[instruction->getOperand(0)].push_back(instruction->getOperand(1));
            toRemove.push_back(instruction.get());
        }
        else if(instruction->getOpcode() == IR::Instruction::Opcode::Load && stack.contains(instruction->getOperand(0)) && wasAllocaUsed(promoted, instruction.get())) {
            if(stack.at(instruction->getOperand(0)).empty()) {
                stack.at(instruction->getOperand(0)).push_back(IR::UndefValue::get(instruction->getType(), m_context));
            }
            current->getParentFunction()->replace(instruction.get(), stack.at(instruction->getOperand(0)).back());
            toRemove.push_back(instruction.get());
        }
        else if(instruction->getOpcode() == IR::Instruction::Opcode::Phi) {
            auto phi = cast<IR::PhiInstruction>(instruction.get());
            if(phi->getAlloca())
                stack[phi->getAlloca()].push_back(instruction.get());
        }
    }

    for(IR::Instruction* instruction : toRemove) {
        current->removeInstruction(instruction);
    }

    if(tree->hasChildren(current)) {
        for(IR::Block* child : tree->getChildren(current)) {
            rename(tree, child, stack, promoted);
        }
    }

    for(auto& pair : current->getSuccessors()) {
        IR::Block* successor = pair.first;
        for(IR::AllocateInstruction* promotedAlloca : promoted) {
            if(!successor->getPhiForValues().contains(promotedAlloca)) continue;

            IR::PhiInstruction* phi = successor->getPhiForValues().at(promotedAlloca);
            IR::Value* op = !stack.at(promotedAlloca).empty() ? stack.at(promotedAlloca).back() :
                IR::UndefValue::get(cast<PointerType>(promotedAlloca->getType())->getPointee(), m_context);
            phi->addOperand(op);
            phi->addOperand(current);
        }
    }

    for(IR::AllocateInstruction* alloca : promoted) {
        while(stack[alloca].size() > stackSize[alloca]) {
            stack[alloca].pop_back();
        }
    }
}

bool Mem2Reg::isAllocaPromotable(IR::AllocateInstruction* instruction) {
    if(instruction->getType()->getKind() == Type::TypeKind::Array || instruction->getType()->getKind() == Type::TypeKind::Struct)
        return false;

    bool hasLoad = false;
    bool hasStore = false;
    for(auto use : instruction->getUses()) {
        if(use->getOpcode() != IR::Instruction::Opcode::Load && use->getOpcode() != IR::Instruction::Opcode::Store)
            return false;
        hasLoad |= use->getOpcode() == IR::Instruction::Opcode::Load;
        hasStore |= use->getOpcode() == IR::Instruction::Opcode::Store;
    }

    return hasLoad && hasStore;
}

USet<IR::Block*> Mem2Reg::allocaDefiningBlocks(IR::AllocateInstruction* instruction) {
    USet<IR::Block*> ret;
    for(auto use : instruction->getUses()) {
        if(use->getOpcode() == IR::Instruction::Opcode::Store)
            ret.insert(use->getParentBlock());
    }
    return ret;
}

USet<IR::Block*> Mem2Reg::allocaUsingBlocks(IR::AllocateInstruction* instruction) {
    USet<IR::Block*> ret;
    for(auto use : instruction->getUses()) {
        if(use->getOpcode() == IR::Instruction::Opcode::Load)
            ret.insert(use->getParentBlock());
    }
    return ret;
}

bool Mem2Reg::wasAllocaUsed(const std::vector<IR::AllocateInstruction*>& promoted, IR::Instruction* instruction) {
    for(auto alloca : promoted) {
        for(auto use : alloca->getUses()) {
            if(use == instruction)
                return true;
        }
    }
    return false;
}

}