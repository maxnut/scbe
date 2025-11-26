#pragma once

#include "MIR/operand.hpp"
#include "MIR/instruction.hpp"
#include "MIR/register_info.hpp"
#include "instruction.hpp"
#include "type_alias.hpp"

#include <algorithm>
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

    void addInstruction(std::unique_ptr<Instruction> instruction) {
        instruction->m_parentBlock = this;
        m_instructions.push_back(std::move(instruction));
    }
    void addInstructionBeforeTerminator(std::unique_ptr<Instruction> instruction) { 
        if(m_instructions.empty()) return addInstruction(std::move(instruction));
        instruction->m_parentBlock = this;
        m_instructions.insert(m_instructions.end() - 1, std::move(instruction));
    }
    void addInstructionAtFront(std::unique_ptr<Instruction> instruction) {
        instruction->m_parentBlock = this;
        m_instructions.insert(m_instructions.begin(), std::move(instruction));
    }
    void addInstructionAt(std::unique_ptr<Instruction> instruction, size_t index) {
        instruction->m_parentBlock = this;
        m_instructions.insert(m_instructions.begin() + index, std::move(instruction));
    }
    std::unique_ptr<Instruction> removeInstruction(Instruction* instruction);

    size_t getInstructionIdx(Instruction* instruction);
    size_t last() { return m_instructions.size(); }

    void addSuccessor(Block* block) { m_successors.push_back(block); }
    void addPredecessor(Block* block) { m_predecessors.push_back(block); }

    bool hasReturn(Target::InstructionInfo* info) const;
    bool hasInstruction(Instruction* instruction) const { return std::find_if(m_instructions.begin(), m_instructions.end(), [instruction](auto const& ptr) { return ptr.get() == instruction; }) != m_instructions.end(); }

    IR::Block* getIRBlock() const { return m_irBlock; }

private:
    std::vector<std::unique_ptr<Instruction>> m_instructions;
    std::vector<Block*> m_successors;
    std::vector<Block*> m_predecessors;
    Function* m_parentFunction = nullptr;
    IR::Block* m_irBlock = nullptr;

friend class Function;
};

}