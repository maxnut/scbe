#pragma once

#include "data_layout.hpp"
#include "pass.hpp"
#include "target/instruction_info.hpp"
#include "target/register_info.hpp"
#include "target/target_specification.hpp"

#include <fstream>

namespace scbe::MIR {
class Function;
class Block;
class Instruction;
}

namespace scbe::Target {

class AsmPrinter : public MachineFunctionPass {
public:
    AsmPrinter(std::ofstream& output, InstructionInfo* instructionInfo, RegisterInfo* registerInfo, DataLayout* dataLayout, TargetSpecification spec)
        : m_output(output), m_instructionInfo(instructionInfo), m_registerInfo(registerInfo), m_dataLayout(dataLayout), m_spec(spec) {}

    bool run(MIR::Function* function) override;

    virtual void print(MIR::Function* function) = 0;
    virtual void print(MIR::Block* block) = 0;
    virtual void print(MIR::Instruction* instruction) = 0;
    virtual void print(MIR::Operand* operand, Restriction* restriction = nullptr) = 0;
    
    virtual void print(IR::Constant* constant) = 0;

protected:
    std::ofstream& m_output;
    InstructionInfo* m_instructionInfo;
    RegisterInfo* m_registerInfo;
    MIR::Function* m_current = nullptr;
    DataLayout* m_dataLayout = nullptr;
    TargetSpecification m_spec;
};

}