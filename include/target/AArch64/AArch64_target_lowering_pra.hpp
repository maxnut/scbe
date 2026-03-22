#pragma once

#include "target/target_lowering_pra.hpp"

namespace scbe::Target::AArch64 {
    
class AArch64TargetLoweringPRA : public TargetLoweringPRA {
public:
    AArch64TargetLoweringPRA(RegisterInfo* registerInfo, InstructionInfo* instructionInfo, DataLayout* dataLayout, Ref<Context> ctx) :
        TargetLoweringPRA(registerInfo, instructionInfo, dataLayout, ctx) {}

    bool run(MIR::Function* function) override;
    void lowerIntrinsic(MIR::Block* block, MIR::IntrinsicLowering* instruction) override {}

    void saveCall(MIR::CallInstruction* instruction);
};

}