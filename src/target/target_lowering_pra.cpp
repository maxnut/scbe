#include "target/target_lowering_pra.hpp"
#include "MIR/function.hpp"

namespace scbe::Target {

bool TargetLoweringPRA::run(MIR::Function* function) {
    for(auto& bb : function->getBlocks()) {
        while(true) {
            bool lower = false;
            for(auto& instr : bb->getInstructions()) {
                if(instr->getOpcode() == INTRINSIC_LOWER_OP) {
                    lowerIntrinsic(bb.get(), cast<MIR::IntrinsicLowering>(instr.get()));
                    lower = true;
                    break;
                }
            }
            if(!lower)
                break;
        }
    }
    return false;
}

}