#pragma once

#include "pass.hpp"

namespace scbe::Target {
class RegisterInfo;
}

namespace scbe::Target::AArch64 {

class AArch64InstructionInfo;

class AArch64SaveCallRegisters : public MachineFunctionPass {
public:
    AArch64SaveCallRegisters(RegisterInfo* registerInfo, InstructionInfo* instructionInfo) : m_registerInfo(registerInfo), m_instructionInfo((AArch64InstructionInfo*)instructionInfo) {}

    bool run(MIR::Function* function) override;
    void saveCall(MIR::CallInstruction* instruction);

private:
    RegisterInfo* m_registerInfo;
    AArch64InstructionInfo* m_instructionInfo;
};

}