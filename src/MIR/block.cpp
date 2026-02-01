#include "IR/block.hpp"
#include "MIR/block.hpp"
#include "MIR/function.hpp"
#include "MIR/instruction.hpp"
#include "MIR/register_info.hpp"
#include "target/instruction_info.hpp"

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
    if(auto it = m_idxCache.find(instruction); it != m_idxCache.end())
        return it->second;
    auto it = std::find_if(m_instructions.begin(), m_instructions.end(), [instruction](auto const& ptr) { return ptr.get() == instruction; });
    assert(it != m_instructions.end());
    size_t res = it - m_instructions.begin();
    m_idxCache.insert({instruction, res});
    return res;
}

MIR::Instruction* Block::getTerminator(Target::InstructionInfo* info) {
    for (auto it = m_instructions.rbegin(); it != m_instructions.rend(); ++it) {
        auto& ins = *it;
        if(ins->getOpcode() == RETURN_LOWER_OP) return ins.get();
        auto desc = info->getInstructionDescriptor(ins->getOpcode());
        if(!desc.isJump() && !desc.isReturn()) continue;
        return ins.get();
    }
    return nullptr;
}

void Block::addInstruction(std::unique_ptr<Instruction> instruction) {
    if(instruction->getOpcode() == PHI_LOWER_OP) m_phis.push_back(cast<PhiLowering>(instruction.get()));
    instruction->m_parentBlock = this;
    m_instructions.push_back(std::move(instruction));
    m_idxCache.clear();
    m_parentFunction->instructionsChanged();
}
void Block::addInstructionBeforeLast(std::unique_ptr<Instruction> instruction) { 
    if(m_instructions.empty()) return addInstruction(std::move(instruction));
    if(instruction->getOpcode() == PHI_LOWER_OP) m_phis.push_back(cast<PhiLowering>(instruction.get()));
    instruction->m_parentBlock = this;
    m_instructions.insert(m_instructions.end() - 1, std::move(instruction));
    m_idxCache.clear();
    m_parentFunction->instructionsChanged();
}
void Block::addInstructionAtFront(std::unique_ptr<Instruction> instruction) {
    if(instruction->getOpcode() == PHI_LOWER_OP) m_phis.push_back(cast<PhiLowering>(instruction.get()));
    instruction->m_parentBlock = this;
    m_instructions.insert(m_instructions.begin(), std::move(instruction));
    m_idxCache.clear();
    m_parentFunction->instructionsChanged();
}
void Block::addInstructionAt(std::unique_ptr<Instruction> instruction, size_t index) {
    if(instruction->getOpcode() == PHI_LOWER_OP) m_phis.push_back(cast<PhiLowering>(instruction.get()));
    instruction->m_parentBlock = this;
    m_instructions.insert(m_instructions.begin() + index, std::move(instruction));
    m_idxCache.clear();
    m_parentFunction->instructionsChanged();
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
    m_idxCache.clear();
    m_parentFunction->instructionsChanged();
    if(instruction->getOpcode() == PHI_LOWER_OP)
        m_phis.erase(std::remove(m_phis.begin(), m_phis.end(), cast<PhiLowering>(instruction)), m_phis.end());
    return std::move(ret);
}

std::unique_ptr<Instruction> Block::removeInstruction(size_t idx) {
    Instruction* instruction = m_instructions.at(idx).get();
    for(auto pair : m_parentFunction->getRegisterInfo().m_liveRanges) {
        for(LiveRange& range : pair.second) {
            if(range.m_instructionRange.first == instruction) range.m_instructionRange.first = range.m_instructionRange.second;
            if(range.m_instructionRange.second == instruction) range.m_instructionRange.second = range.m_instructionRange.first;
        }
    }
    std::unique_ptr<Instruction> ret = std::move(m_instructions[idx]);
    m_instructions.erase(m_instructions.begin() + idx);
    m_idxCache.clear();
    m_parentFunction->instructionsChanged();
    return std::move(ret);
}

}