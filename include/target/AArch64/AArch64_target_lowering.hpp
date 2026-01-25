#pragma once

#include "MIR/instruction.hpp"
#include "opt_level.hpp"
#include "target/register_info.hpp"
#include "target/target_lowering.hpp"

namespace scbe::Target::AArch64 {

class AArch64TargetLowering : public TargetLowering {
public:
    AArch64TargetLowering(RegisterInfo* registerInfo, InstructionInfo* instructionInfo, DataLayout* dataLayout, TargetSpecification spec, OptimizationLevel level, Ref<Context> ctx)
        : TargetLowering(registerInfo, instructionInfo, dataLayout, spec, level, ctx) {}

    MIR::CallInstruction* lowerCall(MIR::Block* block, MIR::CallLowering* instruction) override;
    void lowerFunction(MIR::Function* function) override;
    void lowerSwitch(MIR::Block* block, MIR::SwitchLowering* instruction) override;
    void lowerReturn(MIR::Block* block, MIR::ReturnLowering* instruction) override;
    void lowerVaStart(MIR::Block* block, MIR::VaStartLowering* instruction) override;
    void lowerVaEnd(MIR::Block* block, MIR::VaEndLowering* instruction) override;
    void parallelCopy(MIR::Block* block) override;

private:
    std::vector<MIR::Instruction*> m_returnInstructions;
    size_t m_usedGp = 0, m_usedFp = 0;
};

}