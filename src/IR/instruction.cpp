#include "IR/instruction.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"

namespace scbe::IR {

void Instruction::beforeRemove(Block* from) {
    for(auto& op : getOperands())
        op->removeFromUses(this);

    for(auto& instr : getUses())
        instr->removeOperand(this);
}

void Instruction::removeOperand(Value* operand) {
    m_operands.erase(std::find(m_operands.begin(), m_operands.end(), operand)); operand->removeFromUses(this); 
}

void Instruction::cloneInternal() {
    m_name = "";
    m_parentBlock = nullptr;
    m_uses.clear();
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

std::string Instruction::opcodeString(Opcode op) {
  switch (op) {
    case Instruction::Opcode::Load:
        return "load";
        break;
    case Instruction::Opcode::Store:
        return "store";
        break;
    case Instruction::Opcode::Add:
        return "add";
        break;
    case Instruction::Opcode::Sub:
        return "sub";
        break;
    case Instruction::Opcode::IMul:
        return "imul";
        break;
    case Instruction::Opcode::UMul:
        return "umul";
        break;
    case Instruction::Opcode::FMul:
        return "fmul";
        break;
    case Instruction::Opcode::Allocate:
        return "allocate";
        break;
    case Instruction::Opcode::Ret:
        return "ret";
        break;
    case Instruction::Opcode::ICmpEq:
        return "icmp eq";
        break;
    case Instruction::Opcode::ICmpNe:
        return "icmp ne";
        break;
    case Instruction::Opcode::ICmpGt:
        return "icmp gt";
        break;
    case Instruction::Opcode::ICmpGe:
        return "icmp ge";
        break;
    case Instruction::Opcode::ICmpLt:
        return "icmp lt";
        break;
    case Instruction::Opcode::ICmpLe:
        return "icmp le";
        break;
    case Instruction::Opcode::UCmpGt:
        return "ucmp gt";
        break;
    case Instruction::Opcode::UCmpGe:
        return "ucmp ge";
        break;
    case Instruction::Opcode::UCmpLt:
        return "ucmp lt";
        break;
    case Instruction::Opcode::UCmpLe:
        return "ucmp le";
        break;
    case Instruction::Opcode::FCmpEq:
        return "fcmp eq";
        break;
    case Instruction::Opcode::FCmpNe:
        return "fcmp ne";
        break;
    case Instruction::Opcode::FCmpGt:
        return "fcmp gt";
        break;
    case Instruction::Opcode::FCmpGe:
        return "fcmp ge";
        break;
    case Instruction::Opcode::FCmpLt:
        return "fcmp lt";
        break;
    case Instruction::Opcode::FCmpLe:
        return "fcmp le";
        break;
    case Instruction::Opcode::Jump:
        return "jump";
        break;
    case Instruction::Opcode::Phi:
        return "phi";
        break;
    case Instruction::Opcode::GetElementPtr:
        return "getelementptr";
        break;
    case Instruction::Opcode::Call:
        return "call";
        break;
    case Instruction::Opcode::Zext:
        return "zext";
        break;
    case Instruction::Opcode::Sext:
        return "sext";
        break;
    case Instruction::Opcode::Trunc:
        return "trunc";
        break;
    case Instruction::Opcode::Fptrunc:
        return "fptrunc";
        break;
    case Instruction::Opcode::Fpext:
        return "fpext";
        break;
    case Instruction::Opcode::Fptosi:
        return "fptosi";
        break;
    case Instruction::Opcode::Uitofp:
        return "uitofp";
        break;
    case Instruction::Opcode::Bitcast:
        return "bitcast";
        break;
    case Instruction::Opcode::Ptrtoint:
        return "ptrtoint";
        break;
    case Instruction::Opcode::Inttoptr:
        return "inttoptr";
        break;
    case Instruction::Opcode::Fptoui:
        return "fptoui";
        break;
    case Instruction::Opcode::Sitofp:
        return "sitofp";
        break;
    case Instruction::Opcode::ShiftLeft:
        return "shl";
        break;
    case Instruction::Opcode::LShiftRight:
        return "lshr";
        break;
    case Instruction::Opcode::AShiftRight:
        return "ashr";
        break;
    case Instruction::Opcode::And:
        return "and";
        break;
    case Instruction::Opcode::Or:
        return "or";
        break;
    case Instruction::Opcode::Xor:
        return "xor";
        break;
    case Instruction::Opcode::IDiv:
        return "idiv";
        break;
    case Instruction::Opcode::UDiv:
        return "udiv";
        break;
    case Instruction::Opcode::FDiv:
        return "fdiv";
        break;
    case Instruction::Opcode::IRem:
        return "irem";
        break;
    case Instruction::Opcode::URem:
        return "urem";
        break;
    case Instruction::Opcode::Switch:
        return "switch";
        break;
    case Instruction::Opcode::ExtractValue:
        return "extractvalue";
        break;
    case Instruction::Opcode::Count:
        break;
    }
    return "";
}

}