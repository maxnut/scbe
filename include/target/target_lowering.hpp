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
    TargetLowering(RegisterInfo* registerInfo, InstructionInfo* instructionInfo, DataLayout* dataLayout, OS os, OptimizationLevel optLevel)
        : MachineFunctionPass(), m_registerInfo(registerInfo), m_instructionInfo(instructionInfo), m_dataLayout(dataLayout), m_spiller(dataLayout, instructionInfo, registerInfo), m_os(os), m_optLevel(optLevel) {}

    virtual bool run(MIR::Function* function);

    virtual MIR::CallInstruction* lowerCall(MIR::Block* block, MIR::CallLowering* instruction) = 0;
    virtual void lowerFunction(MIR::Function* function) = 0;
    virtual void lowerSwitch(MIR::Block* block, MIR::SwitchLowering* instruction) = 0;
    virtual void lowerReturn(MIR::Block* block, MIR::ReturnLowering* instruction) = 0;
    virtual void parallelCopy(MIR::Block* block) = 0;

    void lowerPhis(MIR::Function* function);

protected:
    RegisterInfo* m_registerInfo;
    InstructionInfo* m_instructionInfo;
    DataLayout* m_dataLayout = nullptr;
    Codegen::Spiller m_spiller;
    OS m_os = OS::Unknwon;
    OptimizationLevel m_optLevel;
};

}