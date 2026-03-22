#pragma once

#include "MIR/function.hpp"
#include "MIR/instruction.hpp"
#include "codegen/spiller.hpp"
#include "opt_level.hpp"
#include "pass.hpp"
#include "target/target_specification.hpp"

namespace scbe {
class DataLayout;
}

namespace scbe::Target {

class InstructionInfo;
class RegisterInfo;

class TargetLowering : public MachineFunctionPass {

protected:
    TargetLowering(RegisterInfo* registerInfo, InstructionInfo* instructionInfo, DataLayout* dataLayout, TargetSpecification spec, OptimizationLevel optLevel, Ref<Context> ctx)
        : MachineFunctionPass(), m_registerInfo(registerInfo), m_instructionInfo(instructionInfo), m_dataLayout(dataLayout), m_spiller(dataLayout, instructionInfo, registerInfo), m_targetSpec(spec), m_optLevel(optLevel), m_ctx(ctx) {}

    virtual bool run(MIR::Function* function);

    virtual MIR::CallInstruction* lowerCall(MIR::Block* block, MIR::CallLowering* instruction) = 0;
    virtual void lowerFunction(MIR::Function* function) = 0;
    virtual void lowerSwitch(MIR::Block* block, MIR::SwitchLowering* instruction) = 0;
    virtual void lowerReturn(MIR::Block* block, MIR::ReturnLowering* instruction) = 0;
    virtual bool lowerIntrinsic(MIR::Block* block, MIR::IntrinsicLowering* instruction) = 0;

    void lowerPhis(MIR::Function* function);
    void parallelCopy(std::vector<std::pair<MIR::Register*, MIR::Operand*>>& copies, MIR::Block* block);

protected:
    RegisterInfo* m_registerInfo = nullptr;
    InstructionInfo* m_instructionInfo = nullptr;
    DataLayout* m_dataLayout = nullptr;
    Codegen::Spiller m_spiller;
    TargetSpecification m_targetSpec;
    OptimizationLevel m_optLevel;
    Ref<Context> m_ctx;
};

}