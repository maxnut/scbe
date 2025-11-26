#include "IR/block.hpp"
#include "MIR/block.hpp"
#include "MIR/function.hpp"
#include "MIR/instruction.hpp"
#include "MIR/register_info.hpp"
#include "target/instruction_info.hpp"

#include <cassert>
#include <memory>

namespace scbe::MIR {

Block::Block(const std::string& name, IR::Block* irBlock) : Symbol(name, Operand::Kind::Block), m_irBlock(irBlock) {
    irBlock->m_mirBlock = this;
}

bool Block::hasReturn(Target::InstructionInfo* info) const {
    if(m_instructions.empty())
        return false;
    return info->getInstructionDescriptor(m_instructions.back()->getOpcode()).isReturn();
}

size_t Block::getInstructionIdx(Instruction* instruction) {
    auto it = std::find_if(m_instructions.begin(), m_instructions.end(), [instruction](auto const& ptr) { return ptr.get() == instruction; });
    assert(it != m_instructions.end());
    return it - m_instructions.begin();
}

std::unique_ptr<Instruction> Block::removeInstruction(Instruction* instruction) {
    for(auto pair : m_parentFunction->getRegisterInfo().m_liveRanges) {
        for(LiveRange& range : pair.second) {
            if(range.m_instructionRange.first == instruction) range.m_instructionRange.first = range.m_instructionRange.second;
            if(range.m_instructionRange.second == instruction) range.m_instructionRange.second = range.m_instructionRange.first;
        }
    }
    auto idx = getInstructionIdx(instruction);
    std::unique_ptr<Instruction> ret = std::move(m_instructions[idx]);
    m_instructions.erase(m_instructions.begin() + idx);
    return std::move(ret);
}

}