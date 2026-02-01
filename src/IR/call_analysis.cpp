#include "IR/call_analysis.hpp"
#include "IR/heuristics.hpp"
#include "IR/instruction.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"

namespace scbe::IR {

bool CallAnalysis::run(Function* function) {
    function->getHeuristics().m_callSites.clear();
    function->m_isRecursive = false;

    for(auto& block : function->getBlocks()) {
        for(auto& instruction : block->getInstructions()) {
            if(instruction->getOpcode() != Instruction::Opcode::Call) continue;
            CallInstruction* call = cast<CallInstruction>(instruction.get());

            if(call->getCallee()->isFunction()) {
                if(call->getCallee() == function) function->m_isRecursive = true;
            }
            
            CallSite callSite(call, call->getParentBlock(), call->getCallee());
            function->getHeuristics().addCallSite(callSite);
        }
    }
    
    return false;
}

}