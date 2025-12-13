#pragma once

#include "pass.hpp"
#include "codegen/spiller.hpp"

namespace scbe::MIR {
class Function;
class Register;
}

namespace scbe::Target {
class InstructionInfo;
class RegisterInfo;
}

#define SPILL std::numeric_limits<uint32_t>().max()

namespace scbe::Codegen {

class RegallocBase : public MachineFunctionPass {
public:
    RegallocBase(DataLayout* dataLayout, Target::InstructionInfo* instrInfo, Target::RegisterInfo* registerInfo) : m_registerInfo(registerInfo), m_spiller(dataLayout, instrInfo, registerInfo), m_instructionInfo(instrInfo) {}

    virtual uint32_t pickAvailableRegister(MIR::Function* function, uint32_t vregId) = 0;
    virtual void analyze(MIR::Function* function) = 0;
    virtual void end(MIR::Function* function) = 0;

    bool run(MIR::Function* function) override;
    void processSpills(MIR::Function* function);

protected:
    Spiller m_spiller;
    Target::RegisterInfo* m_registerInfo = nullptr;
    Target::InstructionInfo* m_instructionInfo = nullptr;
};

}