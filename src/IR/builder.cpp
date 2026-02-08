#include "IR/builder.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/instruction.hpp"
#include "IR/value.hpp"
#include "type.hpp"

namespace scbe::IR {

AllocateInstruction* Builder::createAllocate(Type* type, const std::string& name) {
    assert(!type->isFuncType() && "Cannot allocate function");
    auto instr = std::make_unique<AllocateInstruction>(m_context->makePointerType(type), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createLoad(Value* ptr, const std::string& name) {
    auto instr = std::make_unique<LoadInstruction>(ptr);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createStore(Value* ptr, Value* value) {
    assert(ptr->getType()->isPtrType() && cast<PointerType>(ptr->getType())->getPointee() == value->getType());
    auto instr = std::make_unique<StoreInstruction>(ptr, value);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createAdd(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::Add, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::Add, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createSub(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::Sub, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::Sub, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createIMul(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::IMul, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::IMul, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createUMul(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::UMul, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::UMul, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createFMul(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::FMul, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::FMul, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createIDiv(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::IDiv, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::IDiv, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createUDiv(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::UDiv, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::UDiv, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createFDiv(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::FDiv, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::FDiv, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createIRem(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::IRem, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::IRem, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createURem(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::URem, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::URem, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

void Builder::createRet(Value* value) {
    auto instr = std::make_unique<ReturnInstruction>(value);
    auto ret = instr.get();
	insert(std::move(instr));
}



Value* Builder::createICmpEq(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::ICmpEq, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::ICmpEq, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createICmpNe(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::ICmpNe, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::ICmpNe, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createICmpGt(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::ICmpGt, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::ICmpGt, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createICmpGe(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::ICmpGe, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::ICmpGe, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createICmpLt(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::ICmpLt, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::ICmpLt, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createICmpLe(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::ICmpLe, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::ICmpLe, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createUCmpGt(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::UCmpGt, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::UCmpGt, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createUCmpGe(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::UCmpGe, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::UCmpGe, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createUCmpLt(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::UCmpLt, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::UCmpLt, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createUCmpLe(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::UCmpLe, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::UCmpLe, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createFCmpEq(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::FCmpEq, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::FCmpEq, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createFCmpNe(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::FCmpNe, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::FCmpNe, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createFCmpGt(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::FCmpGt, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::FCmpGt, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createFCmpGe(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::FCmpGe, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::FCmpGe, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createFCmpLt(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::FCmpLt, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::FCmpLt, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createFCmpLe(Value* lhs, Value* rhs, const std::string& name) {
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::FCmpLe, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::FCmpLe, lhs, rhs, m_context->getI1Type(), name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createLeftShift(Value* lhs, Value* rhs, const std::string& name) {
    assert(rhs->getType()->isIntType());
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::ShiftLeft, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::ShiftLeft, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createLRightShift(Value* lhs, Value* rhs, const std::string& name) {
    assert(rhs->getType()->isIntType());
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::LShiftRight, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::LShiftRight, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createARightShift(Value* lhs, Value* rhs, const std::string& name) {
    assert(rhs->getType()->isIntType());
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::AShiftRight, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::AShiftRight, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createAnd(Value* lhs, Value* rhs, const std::string& name) {
    assert(rhs->getType()->isIntType());
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::And, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::And, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createXor(Value* lhs, Value* rhs, const std::string& name) {
    assert(rhs->getType()->isIntType());
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::Xor, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::Xor, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createOr(Value* lhs, Value* rhs, const std::string& name) {
    assert(rhs->getType()->isIntType());
    if(auto v = m_folder.foldBinOp(Instruction::Opcode::Or, lhs, rhs)) return v;
    auto instr = std::make_unique<BinaryOperator>(Instruction::Opcode::Or, lhs, rhs, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createGEP(Value* ptr, const std::vector<Value*>& indices, const std::string& name) {
    assert((ptr->getType()->isPtrType() || ptr->getType()->isArrayType()) && "Expected pointer or array value");

    auto current = ptr->getType();
    for(auto index : indices) {
        assert(index->getType()->isIntType() && "Expected index integer type");
        assert(current->getContainedTypes().size() > 0 && "Expected > 0 contained types");

        switch (index->getKind()) {
            case Value::ValueKind::ConstantInt: {
                ConstantInt* constant = (ConstantInt*)index;
                current = current->getContainedTypes().at(current->isArrayType() || current->isPtrType() ? 0 : constant->getValue());
                break;
            }
            case Value::ValueKind::Register: {
                assert((current->isPtrType() || current->isArrayType()) && "Expected pointer or array type");
                current = current->getContainedTypes().at(0);
                break;
            }
            default:
                assert(false && "Unreachable");
                break;
        }
    }
    auto instr = std::make_unique<GEPInstruction>(m_context->makePointerType(current), ptr, indices, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createCall(Value* callee, const std::string& name) {
    FunctionType* fnType = cast<FunctionType>(cast<PointerType>(callee->getType())->getPointee());
    assert(fnType->isFuncType());
    auto instr = std::make_unique<CallInstruction>(fnType->getReturnType(), callee, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createCall(Value* callee, const std::vector<Value*>& args, const std::string& name) {
    FunctionType* fnType = cast<FunctionType>(cast<PointerType>(callee->getType())->getPointee());
    assert(fnType->isFuncType());
    auto instr = std::make_unique<CallInstruction>(fnType->getReturnType(), callee, args, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createZext(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Zext, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Zext, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createSext(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Sext, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Sext, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createTrunc(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Trunc, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Trunc, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createFptrunc(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Fptrunc, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Fptrunc, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createFpext(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Fpext, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Fpext, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createFptosi(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Fptosi, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Fptosi, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createFptoui(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Fptoui, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Fptoui, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createSitofp(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Sitofp, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Sitofp, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createUitofp(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Uitofp, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Uitofp, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createBitcast(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Bitcast, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Bitcast, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createPtrtoint(Value* value, Type* toType, const std::string& name) {
    if(auto v = m_folder.foldCast(Instruction::Opcode::Ptrtoint, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Ptrtoint, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}
Value* Builder::createInttoptr(Value* value, Type* toType, const std::string& name) {
    assert(toType->isPtrType() && "Expected pointer type");
    if(auto v = m_folder.foldCast(Instruction::Opcode::Inttoptr, value, toType)) return v;
    auto instr = std::make_unique<CastInstruction>(Instruction::Opcode::Inttoptr, value, toType, name);
    auto ret = instr.get();
	insert(std::move(instr));
    return ret;
}

Value* Builder::createExtractValue(Value* from, ConstantInt* index, const std::string& name) {
    auto instr = std::make_unique<ExtractValueInstruction>(from, index, name);
    auto ret = instr.get();
    insert(std::move(instr));
    return ret;
}

Value* Builder::createPhi(const std::vector<std::pair<Value*, Block*>>& values, Type* type, const std::string& name) {
    auto instr = std::make_unique<PhiInstruction>(type, name);
    for(auto& [value, block] : values) {
        instr->addOperand(value);
        instr->addOperand(block);
    }
    auto ret = instr.get();
    insert(std::move(instr));
    return ret;
}

void Builder::createCondJump(Block* first, Block* second, Value* cond) {
    if(cond->isConstantInt()) {
        ConstantInt* constant = (ConstantInt*)(cond);
        createJump(constant->getValue() ? first : second);   
        return;
    }

    auto instr = std::make_unique<JumpInstruction>(first, second, cond);
    auto ret = instr.get();
	insert(std::move(instr));
}

void Builder::createJump(Block* to) {
    auto instr = std::make_unique<JumpInstruction>(to);
    auto ret = instr.get();
	insert(std::move(instr));
}

void Builder::createSwitch(Value* value, Block* defaultBlock, const std::vector<std::pair<ConstantInt*, Block*>>& cases) {
    auto instr = std::make_unique<SwitchInstruction>(value, defaultBlock, cases);
    auto ret = instr.get();
	insert(std::move(instr));
}

void Builder::insert(std::unique_ptr<Instruction> instruction) {
    assert(m_currentBlock && "Current block not set");

    if(!m_insertPoint) {
        m_currentBlock->addInstruction(std::move(instruction));
        return;
    }

    if(m_insertBefore)
        m_currentBlock->addInstructionBefore(std::move(instruction), m_insertPoint);
    else
        m_currentBlock->addInstructionAfter(std::move(instruction), m_insertPoint);
    m_insertPoint = instruction.get();
}

}