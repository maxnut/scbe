#include "IR/block.hpp"
#include "IR/instruction.hpp"
#include "IR/function.hpp"
#include "cast.hpp"

#include <algorithm>

namespace scbe::IR {

Block::~Block() = default;
Block::Block(Ref<Context> ctx, const std::string& name) : GlobalValue(ctx->makePointerType(ctx->getVoidType()), Linkage::Internal, ValueKind::Block, name), m_context(ctx) {}

bool nameBlacklist(Instruction::Opcode opcode) {
    return opcode == Instruction::Opcode::Store || opcode == Instruction::Opcode::Ret || opcode == Instruction::Opcode::Jump;
}

void Block::addInstruction(std::unique_ptr<Instruction> instruction) {
    if(instruction->getName().empty() && !nameBlacklist(instruction->getOpcode()))
        instruction->setName(std::to_string(m_parentFunction->m_valueNameCounter++));
    instruction->m_parentBlock = this;

    m_instructions.push_back(std::move(instruction));
    m_instructions.back()->onAdd();
}

void Block::addInstructionAtFront(std::unique_ptr<Instruction> instruction) {
    if(instruction->getName().empty() && !nameBlacklist(instruction->getOpcode()))
        instruction->setName(std::to_string(m_parentFunction->m_valueNameCounter++));
    instruction->m_parentBlock = this;
    m_instructions.insert(m_instructions.begin(), std::move(instruction));
    m_instructions.front()->onAdd();
}

void Block::addInstructionAfter(std::unique_ptr<Instruction> instruction, Instruction* after) {
    if(instruction->getName().empty() && !nameBlacklist(instruction->getOpcode()))
        instruction->setName(std::to_string(m_parentFunction->m_valueNameCounter++));
    instruction->m_parentBlock = this;

    auto it = getInstructionIdx(after);

    m_instructions.insert(m_instructions.begin() + it + 1, std::move(instruction));
    m_instructions[it + 1]->onAdd();
}

void Block::addInstructionBefore(std::unique_ptr<Instruction> instruction, Instruction* before) {
    if(instruction->getName().empty() && !nameBlacklist(instruction->getOpcode()))
        instruction->setName(std::to_string(m_parentFunction->m_valueNameCounter++));
    instruction->m_parentBlock = this;

    auto it = getInstructionIdx(before);

    m_instructions.insert(m_instructions.begin() + it, std::move(instruction));
    m_instructions[it]->onAdd();
}

std::unique_ptr<Instruction> Block::removeInstruction(Instruction* instruction) {
    if(!hasInstruction(instruction)) return nullptr;
    auto idx = getInstructionIdx(instruction);
    
    instruction->beforeRemove(this);
    
    auto instr = std::move(m_instructions[idx]);
    m_instructions.erase(m_instructions.begin() + idx);
    return instr;
}

Block* Block::getImmediateDominator() {
    Block* idom = nullptr;
    for(auto d : m_dominators) {
        if(d == this) continue;
        bool isImmediate = true;
        for(auto other : m_dominators) {
            if(other == this || other == d) continue;
            if(std::find(other->m_dominators.begin(), other->m_dominators.end(), d) != other->m_dominators.end()) {
                isImmediate = false;
                break;
            }
        }
        if(isImmediate) {
            idom = d;
            break;
        }
    }
    return idom;
}

size_t Block::getInstructionIdx(Instruction* instruction) {
    auto it = std::find_if(m_instructions.begin(), m_instructions.end(), [instruction](auto const& ptr) { return ptr.get() == instruction; });
    assert(it != m_instructions.end());
    return it - m_instructions.begin();
}

void Block::setPhiForValue(Value* value, PhiInstruction* phi) {
    m_phiForValues[value] = phi;
}

std::unique_ptr<Block> Block::split(Instruction* at) {
    if(m_instructions.size() < 2) return nullptr;
    std::unique_ptr<Block> block = std::unique_ptr<Block>(new Block(at->getParentBlock()->m_context));
    auto point = getInstructionIdx(at);

    std::vector<std::pair<Value*, Value*>> replacements;
    for(size_t i = point + 1; i < m_instructions.size(); i++) {
        auto& instruction = m_instructions[i];
        auto clone = instruction->clone();
        replacements.push_back({instruction.get(), clone.get()});
        block->m_parentFunction = m_parentFunction;
        block->addInstruction(std::move(clone));
        block->m_parentFunction = nullptr;
    }
    for(auto& replacement : replacements) {
        block->replace(replacement.first, replacement.second);
        getParentFunction()->replace(replacement.first, replacement.second);
    }
    
    while(point + 1< m_instructions.size()) {
        removeInstruction(m_instructions.back().get());
    }
    return std::move(block);
}

void Block::addSuccessor(Block* block) {
    if(!m_successors.contains(block)) {
        m_successors.insert({block, 1});
        return;
    }
    m_successors.at(block)++;
}

void Block::addPredecessor(Block* block) {
    if(!m_predecessors.contains(block)) {
        m_predecessors.insert({block, 1});
        return;
    }
    m_predecessors.at(block)++;
}

void Block::removeSuccessor(Block* block) {
    uint32_t& count = m_successors.at(block);
    count--;
    if(count == 0) m_successors.erase(block);
}

void Block::removePredecessor(Block* block) {
    uint32_t& count = m_predecessors.at(block);
    count--;
    if(count == 0) m_predecessors.erase(block);
}

bool Block::isTerminator() const {
    for(auto& ins : m_instructions) {
        if(ins->isTerminator()) return true;
    }
    return false;
}

void Block::replace(Value* replace, Value* with) {
    for(auto& instr : getInstructions()) {
        for(auto& op : instr->m_operands) {
            if(op != replace) continue;
            replace->removeFromUses(instr.get());
            op = with;
            with->addUse(instr.get());
            if(with->isBlock() && instr->isJump()) {
                cast<Block>(replace)->removePredecessor(this);
                removeSuccessor(cast<Block>(replace));
                cast<Block>(with)->addPredecessor(this);
                addSuccessor(cast<Block>(with));
                m_parentFunction->setDominatorTreeDirty();
            }
        }
    }
}

void Block::clearInstructions() {
    for(auto& instr : getInstructions())
        instr->beforeRemove(this);
    m_instructions.clear();
}

}