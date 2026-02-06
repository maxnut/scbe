#pragma once

#include "type_alias.hpp"
#include "value.hpp"
#include "IR/global_value.hpp"
#include "MIR/block.hpp"
#include "IR/instruction.hpp"

namespace scbe::IR {

class Function;
class Instruction;
class PhiInstruction;
class ComputeDominators;

class Block : public GlobalValue {
public:
    ~Block();

    void addInstruction(std::unique_ptr<Instruction> instruction);
    void addInstructionAtFront(std::unique_ptr<Instruction> instruction);
    void addInstructionAfter(std::unique_ptr<Instruction> instruction, Instruction* after);
    void addInstructionBefore(std::unique_ptr<Instruction> instruction, Instruction* before);
    void replace(Value* replace, Value* with);

    // Remove instruction from block.
    // Make sure to replace any uses of the instruction, since the instruction will be deleted and removed from any uses.
    // Otherwise you will end up with undefined behavior.
    std::unique_ptr<Instruction> removeInstruction(Instruction* instruction);
    void clearInstructions();
    void setPhiForValue(Value* value, PhiInstruction* phi);
    void addSuccessor(Block* block);
    void addPredecessor(Block* block);
    void removeSuccessor(Block* block);
    void removePredecessor(Block* block);
    const std::vector<std::unique_ptr<Instruction>>& getInstructions() const { return m_instructions; }

    const UMap<Block*, uint32_t>& getSuccessors() const { return m_successors; }
    const UMap<Block*, uint32_t>& getPredecessors() const { return m_predecessors; }
    const UMap<Value*, PhiInstruction*>& getPhiForValues() const { return m_phiForValues; }
    const UMap<PhiInstruction*, Value*>& getValueForPhis() const { return m_valueForPhis; }
    std::unique_ptr<Block> split(Instruction* at);

    Function* getParentFunction() const { return m_parentFunction; }
    MIR::Block* getMIRBlock() const { return m_mirBlock; }

    size_t getInstructionIdx(Instruction* instruction);
    bool hasInstruction(Instruction* instruction) const { return std::find_if(m_instructions.begin(), m_instructions.end(), [instruction](auto const& ptr) { return ptr.get() == instruction; }) != m_instructions.end(); }
    bool isTerminator() const;
    bool hasReturn() const;

protected:
    Block(Ref<Context> ctx, const std::string& name = "");

private:
    std::vector<std::unique_ptr<Instruction>> m_instructions;
    Function* m_parentFunction = nullptr;

    UMap<Block*, uint32_t> m_successors;
    UMap<Block*, uint32_t> m_predecessors;
    MIR::Block* m_mirBlock = nullptr;
    Ref<Context> m_context = nullptr;

    UMap<Value*, PhiInstruction*> m_phiForValues;
    UMap<PhiInstruction*, Value*> m_valueForPhis;

friend class Function;
friend class Builder;
friend class IR::ComputeDominators;
friend class MIR::Block;
};

}