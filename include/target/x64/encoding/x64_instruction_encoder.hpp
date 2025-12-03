#pragma once

#include "codegen/instruction_encoder.hpp"

namespace scbe::Target::x64 {

class x64InstructionEncoder : public Codegen::InstructionEncoder {
public:
    x64InstructionEncoder(InstructionInfo* info, TargetSpecification spec) : InstructionEncoder(info, spec) {}

    std::optional<Codegen::Fixup> encode(MIR::Instruction* instruction, UMap<std::string, size_t> symbols, std::vector<uint8_t>& bytes) override;

private:
    uint8_t encodeREX(uint8_t w, uint8_t r, uint8_t x, uint8_t b) const;
    uint8_t encodeModRM(uint8_t mod, uint8_t reg, uint8_t rm) const;
    uint8_t encodeSIB(uint8_t scale, uint8_t index, uint8_t base) const;
    uint8_t encodeRegister(uint32_t reg) const;

    bool isExtendedRegister(uint8_t reg) const;
};

}