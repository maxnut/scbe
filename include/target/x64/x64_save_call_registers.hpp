#pragma once

#include "pass.hpp"


namespace scbe::Target {
class RegisterInfo;
}
namespace scbe::Target::x64 {
    

class x64InstructionInfo;
class x64SaveCallRegisters : public MachineFunctionPass {
public:
    x64SaveCallRegisters(RegisterInfo* registerInfo, InstructionInfo* instructionInfo) : m_registerInfo(registerInfo), m_instructionInfo((x64InstructionInfo*)instructionInfo) {}

    bool run(MIR::Function* function) override;
    size_t saveCall(MIR::Block* block, MIR::CallInstruction* instruction);

private:
    RegisterInfo* m_registerInfo;
    x64InstructionInfo* m_instructionInfo;
};

}