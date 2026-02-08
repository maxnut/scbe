#include "IR/function_inlining.hpp"
#include "IR/builder.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/heuristics.hpp"
#include "IR/instruction.hpp"
#include "unit.hpp"

namespace scbe::IR {

bool FunctionInlining::run(Function* function) {
    bool changes = false;
    do {
        changes = false;
        for(auto& callSite : function->getHeuristics().getCallSites()) {
            if(!callSite.getCallee()->isFunction()) continue;
            Function* callee = cast<Function>(callSite.getCallee());

            if(!callee->hasBody() || callee->isRecursive()) continue;

            size_t callerSize = function->getInstructionSize();
            size_t calleeSize = callee->getInstructionSize();

            const double callOverheadBenefit = 8.0;
            const double loopDepthWeight = 6.0;
            const double constArgBonus = 3.0;
            const double globalArgBonus = 1.5;
            const double tinyFuncThreshold = 6;
            const double tinyFuncBonus = 10.0;

            double baseCost = static_cast<double>(calleeSize);
            double baseBenefit = callOverheadBenefit;

            LoopInfo* innermost = function->getHeuristics().getInnermostLoop(callSite.getLocation());
            uint32_t loopDepth = innermost ? innermost->getDepth() : 0;
            double loopBonus = loopDepth * loopDepthWeight;
            baseBenefit += loopBonus;

            for (Value* arg : callSite.getCall()->getArguments()) {
                if (arg->isConstant())
                    baseBenefit += constArgBonus;
                else if (arg->isGlobalVariable())
                    baseBenefit += globalArgBonus;
            }

            if (calleeSize <= tinyFuncThreshold)
                baseBenefit += tinyFuncBonus;

            double score = baseBenefit - baseCost;

            int budget = std::max(
                100,
                static_cast<int>(function->getUnit()->getIRInstructionSize() * 0.2)
            );

            bool withinBudget = (m_totalInstructionsAdded + calleeSize) <= budget;
            bool profitable = (score >= 0.0);

            if (!profitable || !withinBudget)
                continue;

            changes = true;

            std::vector<PhiInstruction*> phis;

            for(auto& pair : callSite.getLocation()->getSuccessors()) {
                for(auto& ins : pair.first->getInstructions()) {
                    if(ins->getOpcode() != Instruction::Opcode::Phi) continue;
                    phis.push_back(cast<PhiInstruction>(ins.get()));
                }
            }

            IR::Builder builder(function->getUnit()->getContext());
            builder.setCurrentBlock(callSite.getLocation());
            auto mergeBlock = callSite.getLocation()->split(callSite.getCall());
            Block* mergeBlockPtr = mergeBlock.get();

            auto returnValuePtr = builder.createAllocate(callee->getFunctionType()->getReturnType());
            UMap<Value*, Value*> vmap;
            for(size_t i = 0; i < callee->getArguments().size(); i++) {
                vmap[callee->getArguments().at(i).get()] = callSite.getCall()->getArguments()[i];
            }

            std::vector<Block*> clonedBlocks;
            UMap<Block*, std::vector<std::unique_ptr<Instruction>>> instructionsPerBlocks;
            for(auto& block : callee->getBlocks()) {
                builder.setCurrentBlock(function->insertBlockAfter(builder.getCurrentBlock()));
                clonedBlocks.push_back(builder.getCurrentBlock());
                vmap[block.get()] = builder.getCurrentBlock();
                for(auto& instruction : block->getInstructions()) {
                    auto clone = instruction->clone();
                    vmap[instruction.get()] = clone.get();
                    // we do not add the instruction here yet because for example:
                    // add jump instruction, jump instruction still references old blocks from the callee,
                    // jump->onAdd() gets called, it adds successors and predecessors
                    // but we haven't updated operands yet
                    instructionsPerBlocks[builder.getCurrentBlock()].push_back(std::move(clone));
                }
            }

            for(auto& pair : instructionsPerBlocks) {
                for(auto& instruction : pair.second) {
                    for(auto& op : instruction->m_operands) {
                        if(!vmap.contains(op)) continue;
                        Value* with = vmap.at(op);
                        op->removeFromUses(instruction.get());
                        op = with;
                        op->addUse(instruction.get());
                    }
                }
            }

            std::vector<Instruction*> toRemove;
            for(auto& pair : instructionsPerBlocks) {
                builder.setCurrentBlock(pair.first);
                for(auto& ins : pair.second) {
                    Instruction* instruction = ins.get();
                    builder.getCurrentBlock()->addInstruction(std::move(ins));
                    if(instruction->getOpcode() == Instruction::Opcode::Ret) {
                        toRemove.push_back(instruction);
                        builder.setCurrentBlock(pair.first);
                        builder.setInsertPoint(instruction);
                        if(instruction->getOperands().size() > 0)
                            builder.createStore(returnValuePtr, instruction->getOperand(0));
                        builder.createJump(mergeBlockPtr);
                    }
                }
            }

            for(Instruction* remove : toRemove) {
                remove->getParentBlock()->removeInstruction(remove);
            }

            builder.setCurrentBlock(mergeBlockPtr);
            builder.setInsertPoint(mergeBlock->getInstructions().at(0).get());
            builder.setInsertBefore(true);
            function->insertBlockAfter(clonedBlocks.back(), std::move(mergeBlock));
            function->replace(callSite.getCall(), builder.createLoad(returnValuePtr));
            builder.setInsertBefore(false);

            function->removeInstruction(callSite.getCall());

            builder.setCurrentBlock(callSite.getLocation());
            builder.createJump(clonedBlocks.at(0));

            for(auto& phi : phis) {
                for(size_t i = 1; i < phi->getNumOperands(); i += 2) {
                    auto& op = phi->m_operands.at(i);
                    if(op != callSite.getLocation()) continue;
                    op->removeFromUses(phi);
                    op = mergeBlockPtr;
                    mergeBlockPtr->addUse(phi);
                }
            }

            m_totalInstructionsAdded += function->getInstructionSize() - callerSize;
            break;
        }
    }
    while(changes);
    return changes;
}

}