#pragma once

#include "target/asm_printer.hpp"

namespace scbe::Target::AArch64 {

class AArch64AsmPrinter : public AsmPrinter {
public:
    AArch64AsmPrinter(std::ofstream& output, InstructionInfo* instructionInfo, RegisterInfo* registerInfo, DataLayout* dataLayout, TargetSpecification spec)
        : AsmPrinter(output, instructionInfo, registerInfo, dataLayout, spec) {}

    void print(MIR::Function* function) override;
    void print(MIR::Block* block) override;
    void print(MIR::Instruction* instruction) override;
    void print(MIR::Operand* operand, Restriction* restriction = nullptr) override;

    void print(IR::Constant* constant) override;

    void printMemory(MIR::Register* base, MIR::Operand* offset, MIR::ImmediateInt* indexing, MIR::Symbol* symbol);

    void init(Unit& unit) override;
    void end(Unit& unit) override;
};

}