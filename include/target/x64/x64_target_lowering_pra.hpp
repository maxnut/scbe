#pragma once

#include "target/target_lowering_pra.hpp"

namespace scbe::Target::x64 {
    
class x64TargetLoweringPRA : public TargetLoweringPRA {
public:
    x64TargetLoweringPRA(RegisterInfo* registerInfo, InstructionInfo* instructionInfo, DataLayout* dataLayout, Ref<Context> ctx) :
        TargetLoweringPRA(registerInfo, instructionInfo, dataLayout, ctx) {}

    bool run(MIR::Function* function) override;
    void lowerIntrinsic(MIR::Block* block, MIR::IntrinsicLowering* instruction) override;

    void saveCall(MIR::CallInstruction* instruction);
};

}