#include "IR/fix_phis.hpp"
#include "IR/instruction.hpp"
#include "IR/block.hpp"

namespace scbe::IR {

bool FixPhis::run(Instruction* instruction) {
    PhiInstruction* phi = dyn_cast<PhiInstruction>(instruction);
    if(!phi) return false;

    bool change = false;

    for(size_t i = 0; i < phi->getNumOperands();) {
        if(phi->getParentBlock()->getPredecessors().contains(cast<Block>(phi->getOperand(i+1)))) {
            i += 2;
            continue;
        }
        phi->removeOperand(phi->getOperand(i+1)); // phi removeOperand also removes value associated with block
        change = true;
    }

    return change;
}

}