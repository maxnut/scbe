#pragma once

#include "pass.hpp"

namespace scbe::Target {

class InstructionInfo;
class RegisterInfo;

class TargetLoweringPRA : public MachineFunctionPass {
public:
    TargetLoweringPRA(RegisterInfo* registerInfo, InstructionInfo* instructionInfo, DataLayout* dataLayout, Ref<Context> ctx)
        : MachineFunctionPass(), m_registerInfo(registerInfo), m_instructionInfo(instructionInfo), m_dataLayout(dataLayout), m_ctx(ctx) {}

    virtual bool run(MIR::Function* function);

    virtual void lowerIntrinsic(MIR::Block* block, MIR::IntrinsicLowering* instruction) = 0;

protected:
    RegisterInfo* m_registerInfo = nullptr;
    InstructionInfo* m_instructionInfo = nullptr;
    DataLayout* m_dataLayout = nullptr;
    Ref<Context> m_ctx;
};
    
}