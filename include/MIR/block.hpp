#pragma once

#include "MIR/operand.hpp"
#include "MIR/instruction.hpp"
#include "MIR/register_info.hpp"
#include "instruction.hpp"
#include "type_alias.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace scbe::Target {
class InstructionInfo;
}

namespace scbe::IR {
class Block;
}

namespace scbe::MIR {

class Function;

class Block : public Symbol {
public:
    Block(const std::string& name, IR::Block* irBlock);

    const std::vector<std::unique_ptr<Instruction>>& getInstructions() const { return m_instructions; }
    const std::vector<Block*>& getSuccessors() const { return m_successors; }
    const std::vector<Block*>& getPredecessors() const { return m_predecessors; }    
    Function* getParentFunction() const { return m_parentFunction; }

    void addInstruction(std::unique_ptr<Instruction> instruction);
    void addInstructionBeforeLast(std::unique_ptr<Instruction> instruction);
    void addInstructionAtFront(std::unique_ptr<Instruction> instruction);
    void addInstructionAt(std::unique_ptr<Instruction> instruction, size_t index);
    std::unique_ptr<Instruction> removeInstruction(Instruction* instruction);
    std::unique_ptr<Instruction> removeInstruction(size_t idx);

    size_t getInstructionIdx(Instruction* instruction);
    size_t last() { return m_instructions.size(); }
    MIR::Instruction* getTerminator(Target::InstructionInfo* info);

    void addSuccessor(Block* block) { m_successors.push_back(block); }
    void addPredecessor(Block* block) { m_predecessors.push_back(block); }
    void addPhiLowering(Register* dest, Operand* src) { m_phiLowering.emplace_back(dest, src); }

    void setEpilogueSize(size_t size) { m_epilogueSize = size; }

    bool hasReturn(Target::InstructionInfo* info) const;
    bool hasInstruction(Instruction* instruction) const { return std::find_if(m_instructions.begin(), m_instructions.end(), [instruction](auto const& ptr) { return ptr.get() == instruction; }) != m_instructions.end(); }

    IR::Block* getIRBlock() const { return m_irBlock; }
    const std::vector<std::pair<Register*, Operand*>>& getPhiLowering() const { return m_phiLowering; }
    size_t getEpilogueSize() const { return m_epilogueSize; }

private:
    std::vector<std::unique_ptr<Instruction>> m_instructions;
    std::vector<Block*> m_successors;
    std::vector<Block*> m_predecessors;
    UMap<MIR::Instruction*, size_t> m_idxCache;
    Function* m_parentFunction = nullptr;
    IR::Block* m_irBlock = nullptr;
    std::vector<std::pair<Register*, Operand*>> m_phiLowering;
    size_t m_epilogueSize = 0;

friend class Function;
};

}