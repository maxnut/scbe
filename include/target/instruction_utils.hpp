#pragma once

#include "MIR/operand.hpp"

#include <array>
#include <cstdint>

namespace scbe::MIR {
class Instruction;
}

namespace scbe {
class DataLayout;
}

namespace scbe::Target {

std::unique_ptr<MIR::Instruction> instr(uint32_t op);

template<typename... Args>
inline std::unique_ptr<MIR::Instruction> instr(uint32_t op, Args&&... ops) {
    return std::make_unique<MIR::Instruction>(op, std::forward<Args>(ops)...);
}

uint32_t selectOpcode(size_t size, bool flt, const std::array<uint32_t, 4>& opcodes, const std::array<uint32_t, 4>& opcodesFlt);

uint32_t selectOpcode(DataLayout* layout, Type* type, const std::array<uint32_t, 4>& opcodes, const std::array<uint32_t, 4>& opcodesFlt);

uint32_t selectRegister(size_t size, const std::array<uint32_t, 4>& regs);

ISel::Node* extractOperand(ISel::Node* node, bool extractCast = true);

bool isRegister(ISel::Node* node);

MIR::ImmediateInt::Size immSizeFromValue(int64_t value);

}