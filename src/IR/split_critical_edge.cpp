#include "IR/split_critical_edge.hpp"
#include "IR/builder.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/instruction.hpp"

namespace scbe::IR {

bool SplitCriticalEdge::run(Function* function) {
    for(size_t i = 0; i < function->getBlocks().size(); i++) {
        if(function->getBlocks().at(i)->getPredecessors().size() <= 1) continue;

        UMap<Block*, uint32_t> copy = function->getBlocks().at(i)->getPredecessors();

        for(auto& [pred, cnt] : copy) {
            if(pred->getSuccessors().size() <= 1) continue;

            std::vector<PhiInstruction*> phis;

            for(auto& ins : function->getBlocks().at(i)->getInstructions()) {
                if(ins->getOpcode() != Instruction::Opcode::Phi) continue;
                phis.push_back(cast<PhiInstruction>(ins.get()));
            }

            if(phis.empty()) continue;

            Block* redirect = function->insertBlockAfter(function->getBlocks().at(i).get());
            Builder builder(m_ctx);
            builder.setCurrentBlock(redirect);
            builder.createJump(function->getBlocks().at(i).get());

            pred->replace(function->getBlocks().at(i).get(), redirect); // TODO i dont think there are other places with blocks as operands as jump, but i could be wrong 

            // TODO maybe replace this with a new replace call with lambda filter?
            for(PhiInstruction* phi : phis) {
                for(auto& op : phi->m_operands) {
                    if(op != pred) continue;
                    pred->removeFromUses(phi);
                    op = redirect;
                    redirect->addUse(phi);
                }
            }
        }
    }
    return false;
}

}