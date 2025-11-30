#include "IR/instruction.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include <span>

namespace scbe::IR {

void Instruction::beforeRemove(Block* from) {
    for(auto& op : getOperands())
        op->removeFromUses(this);

    for(auto& instr : getUses())
        instr->removeOperand(this);
}

void Instruction::cloneInternal() {
    for(auto& op : getOperands())
        op->addUse(this);
}

JumpInstruction::JumpInstruction(Block* first, Block* second, Value* value) : Instruction(Opcode::Jump, nullptr, "") { addOperand(first); addOperand(second); addOperand(value); }
JumpInstruction::JumpInstruction(Block* first) : Instruction(Opcode::Jump, nullptr, "") { addOperand(first); }

void JumpInstruction::onAdd() {
    Block* to = cast<Block>(getOperand(0));
    m_parentBlock->addSuccessor(to);
    to->addPredecessor(m_parentBlock);
    if(m_operands.size() > 1) {
        to = cast<Block>(getOperand(1));
        m_parentBlock->addSuccessor(to);
        to->addPredecessor(m_parentBlock);
    }
    m_parentBlock->getParentFunction()->setCFGDirty();
    Instruction::onAdd();
}

void JumpInstruction::beforeRemove(Block* from) {
    Block* to = cast<Block>(getOperand(0));
    from->removeSuccessor(to);
    to->removePredecessor(from);
    if(m_operands.size() > 1) {
        to = cast<Block>(getOperand(1));
        from->removeSuccessor(to);
        to->removePredecessor(from);
    }
    from->getParentFunction()->setCFGDirty();
    Instruction::beforeRemove(from);
}

CallInstruction::CallInstruction(Type* type, Value* callee, std::string name) : Instruction(Opcode::Call, type, name) { addOperand(callee); }
CallInstruction::CallInstruction(Type* type, Value* callee, const std::vector<Value*>& args, std::string name) : Instruction(Opcode::Call, type, name) {
    addOperand(callee);
    for(auto arg : args) addOperand(arg);
}

bool Instruction::hasSideEffect() const {
    return m_opcode == Opcode::Ret ||
    m_opcode == Opcode::Switch ||
    m_opcode == Opcode::Store ||
    m_opcode == Opcode::Jump ||
    m_opcode == Opcode::Call;
}

SwitchInstruction::SwitchInstruction(Value* value, Block* defaultCase, std::vector<std::pair<ConstantInt*, Block*>> cases) : Instruction(Instruction::Opcode::Switch, nullptr, "") {
    addOperand(value);
    addOperand(defaultCase);
    for(auto casePair : cases) {
        addOperand(casePair.first);
        addOperand(casePair.second);
    }
}

Block* SwitchInstruction::getDefaultCase() const {
    return cast<Block>(getOperand(1));
}

std::vector<std::pair<ConstantInt*, Block*>> SwitchInstruction::getCases() const {
    std::vector<std::pair<ConstantInt*, Block*>> cases;
    for(int i = 2; i < m_operands.size(); i += 2) {
        cases.emplace_back(cast<ConstantInt>(m_operands[i]), cast<Block>(m_operands[i + 1]));
    }
    return cases;
}

void SwitchInstruction::onAdd() {
    m_parentBlock->addSuccessor(getDefaultCase());
    getDefaultCase()->addPredecessor(m_parentBlock);
    for(auto casePair : getCases()) {
        m_parentBlock->addSuccessor(casePair.second);
        casePair.second->addPredecessor(m_parentBlock);
    }
    m_parentBlock->getParentFunction()->setCFGDirty();
    Instruction::onAdd();
}

void SwitchInstruction::beforeRemove(Block* from) {
    from->removeSuccessor(getDefaultCase());
    getDefaultCase()->removePredecessor(from);
    for(auto casePair : getCases()) {
        from->removeSuccessor(casePair.second);
        casePair.second->removePredecessor(from);
    }
    from->getParentFunction()->setCFGDirty();
    Instruction::beforeRemove(from);
}

ConstantInt* ExtractValueInstruction::getIndex() const { return cast<ConstantInt>(getOperand(1)); }

void PhiInstruction::removeOperand(Value* op) {
    if(op->isBlock()) {
        for(size_t i = 1; i < m_operands.size(); i+=2) {
            if(m_operands.at(i) != op) continue;
            m_operands.erase(m_operands.begin() + i - 1); // remove value associated with block
            break;
        }
    }
    Instruction::removeOperand(op);
}

void AllocateInstruction::beforeRemove(Block* from) {
    auto& allocations = from->getParentFunction()->m_allocations;
    allocations.erase(std::find(allocations.begin(), allocations.end(), this));
    Instruction::beforeRemove(from);
}

void AllocateInstruction::onAdd() {
    m_parentBlock->getParentFunction()->m_allocations.push_back(cast<AllocateInstruction>(this));
    Instruction::onAdd();
}

}