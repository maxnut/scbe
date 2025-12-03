#pragma once

#include "target/asm_printer.hpp"
#include "target/target_specification.hpp"

namespace scbe::MIR {
class Function;
class Block;
class Instruction;
class Operand;
class Register;
class Symbol;
}

namespace scbe::Target {
class InstructionInfo;
class RegisterInfo;
}

namespace scbe::Target::x64 {

class x64AsmPrinter : public AsmPrinter {
public:
    x64AsmPrinter(std::ofstream& output, InstructionInfo* instructionInfo, RegisterInfo* registerInfo, DataLayout* dataLayout, TargetSpecification spec)
        : AsmPrinter(output, instructionInfo, registerInfo, dataLayout, spec) {}

    void print(MIR::Function* function) override;
    void print(MIR::Block* block) override;
    void print(MIR::Instruction* instruction) override;
    void print(MIR::Operand* operand, Restriction* restriction = nullptr) override;

    void print(IR::Constant* constant) override;

    void printMemory(size_t size, MIR::Register* base, int64_t offset, MIR::Register* index, int64_t scale, MIR::Symbol* symbol);

    void init(Unit& unit) override;
    void end(Unit& unit) override;
};

}