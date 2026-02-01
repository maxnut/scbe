#include "target/instruction_utils.hpp"
#include "MIR/instruction.hpp"
#include "ISel/instruction.hpp"
#include <limits>

namespace scbe::Target {

std::unique_ptr<MIR::Instruction> instr(uint32_t op) {
    return std::make_unique<MIR::Instruction>(op);
}

uint32_t selectOpcode(size_t size, bool flt, const std::array<uint32_t, 4>& opcodes, const std::array<uint32_t, 4>& opcodesFlt) {
    if(flt) {
        switch(size) {
            case 4: return opcodesFlt[0];
            case 8: return opcodesFlt[1];
        }
        throw std::runtime_error("Not legal");
    }
    switch(size) {
        case 1: return  opcodes[0];
        case 2: return opcodes[1];
        case 4: return opcodes[2];
        case 8: return opcodes[3];
    }
    throw std::runtime_error("Not legal");
}

uint32_t selectOpcode(DataLayout* layout, Type* type, const std::array<uint32_t, 4>& opcodes, const std::array<uint32_t, 4>& opcodesFlt) {
    return selectOpcode(layout->getSize(type), type->isFltType(), opcodes, opcodesFlt);
}

uint32_t selectRegister(size_t size, const std::array<uint32_t, 4>& regs) {
    switch(size) {
        case 1: return regs[0];
        case 2: return regs[1];
        case 4: return regs[2];
        case 8: return regs[3];
    }
    throw std::runtime_error("Not legal");
}

ISel::Node* extractOperand(ISel::Node* node, bool extractCast) {
    switch (node->getKind()) {
        case ISel::Node::NodeKind::ConstantInt:
        case ISel::Node::NodeKind::MultiValue:
        case ISel::Node::NodeKind::FunctionArgument:
        case ISel::Node::NodeKind::ConstantFloat:
        case ISel::Node::NodeKind::GlobalValue:
        case ISel::Node::NodeKind::Register:
        case ISel::Node::NodeKind::FrameIndex:
        case ISel::Node::NodeKind::Root:
        case ISel::Node::NodeKind::Count:
            break;
        case ISel::Node::NodeKind::GenericCast: {
            ISel::Instruction* i = (ISel::Instruction*)node;
            if(!extractCast) return i->getResult();
            return extractOperand(i->getOperands().at(0));
        }
        default: {
            ISel::Instruction* i = (ISel::Instruction*)node;
            return i->getResult();
        }
    }
    return node;
}

bool isRegister(ISel::Node* node) {
    return node->getKind() == ISel::Node::NodeKind::Register || node->getKind() == ISel::Node::NodeKind::FunctionArgument;
}

MIR::ImmediateInt::Size immSizeFromValue(int64_t value) {
    if (value <= std::numeric_limits<int8_t>::max() && value >= std::numeric_limits<int8_t>::min())
        return MIR::ImmediateInt::Size::imm8;
    if (value <= std::numeric_limits<int16_t>::max() && value >= std::numeric_limits<int16_t>::min())
        return MIR::ImmediateInt::Size::imm16;
    if (value <= std::numeric_limits<int32_t>::max() && value >= std::numeric_limits<int32_t>::min())
        return MIR::ImmediateInt::Size::imm32;
    return MIR::ImmediateInt::Size::imm64;
}

}