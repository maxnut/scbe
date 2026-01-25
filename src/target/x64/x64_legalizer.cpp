#include "target/x64/x64_legalizer.hpp"
#include "IR/builder.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "calling_convention.hpp"
#include "target/call_info.hpp"
#include "unit.hpp"

namespace scbe::Target::x64 {

bool x64Legalizer::run(IR::Instruction* instruction) {
    if(instruction->getOpcode() == IR::Instruction::Opcode::Uitofp) {
        IntegerType* fromType = (IntegerType*)(instruction->getOperand(0)->getType());
        if(fromType->getBits() != 64) return false;

        FloatType* toType = (FloatType*)(instruction->getType());
        IR::Builder builder(m_context);
        builder.setCurrentBlock(instruction->getParentBlock());
        builder.setInsertPoint(instruction);
        IR::Value* left = instruction->getOperand(0);
        IR::Value* one = IR::ConstantInt::get(64, 1, m_context);
        IR::Value* zero = IR::ConstantInt::get(64, 0, m_context);

        IR::Value* result = builder.createAllocate(toType);
        auto mergeU = instruction->getParentBlock()->split(cast<IR::AllocateInstruction>(result));
        IR::Block* fast = instruction->getParentBlock()->getParentFunction()->insertBlockAfter(instruction->getParentBlock(), "fast");
        IR::Block* slow = instruction->getParentBlock()->getParentFunction()->insertBlockAfter(instruction->getParentBlock(), "slow");
        IR::Block* merge = mergeU.get();
        instruction->getParentBlock()->getParentFunction()->insertBlockAfter(slow, std::move(mergeU));

        IR::Value* isLarge = builder.createICmpLt(left, zero);
        builder.createCondJump(slow, fast, isLarge);

        builder.setCurrentBlock(fast);
        builder.createStore(result, builder.createSitofp(left, toType));
        builder.createJump(merge);

        builder.setCurrentBlock(slow);
        IR::Value* shifted = builder.createLeftShift(left, one);
        IR::Value* lsb = builder.createAnd(left, one);
        IR::Value* shOr = builder.createOr(shifted, lsb);
        IR::Value* asSigned = builder.createSitofp(shOr, toType);
        builder.createStore(result, builder.createAdd(asSigned, asSigned));
        builder.createJump(merge);

        builder.setCurrentBlock(merge);
        IR::Value* res = builder.createLoad(result);
        instruction->getParentBlock()->getParentFunction()->replace(instruction, res);
        instruction->getParentBlock()->removeInstruction(instruction);
        restart();
        return true;
    }
    if(instruction->getOpcode() == IR::Instruction::Opcode::Fptoui) {
        FloatType* fromType = (FloatType*)(instruction->getOperand(0)->getType());
        IntegerType* toType = (IntegerType*)(instruction->getType());
        if(toType->getBits() != 64) return false;

        IR::Builder builder(m_context);
        builder.setCurrentBlock(instruction->getParentBlock());
        builder.setInsertPoint(instruction);
        IR::Value* left = instruction->getOperand(0);
        IR::Value* big = IR::ConstantInt::get(64, 0x8000000000000000, m_context);
        IR::Value* limit = IR::ConstantFloat::get(fromType->getBits(), fromType->getBits() == 32 ? 9.223372e18 : 9223372036854775808.0, m_context);

        IR::Value* result = builder.createAllocate(toType);
        auto mergeU = instruction->getParentBlock()->split(cast<IR::AllocateInstruction>(result));
        IR::Block* fast = instruction->getParentBlock()->getParentFunction()->insertBlockAfter(instruction->getParentBlock(), "fast");
        IR::Block* slow = instruction->getParentBlock()->getParentFunction()->insertBlockAfter(instruction->getParentBlock(), "slow");
        IR::Block* merge = mergeU.get();
        instruction->getParentBlock()->getParentFunction()->insertBlockAfter(slow, std::move(mergeU));

        IR::Value* inRange = builder.createFCmpLt(left, limit);
        builder.createCondJump(fast, slow, inRange);

        builder.setCurrentBlock(fast);
        builder.createStore(result, builder.createFptosi(left, toType));
        builder.createJump(merge);

        builder.setCurrentBlock(slow);
        IR::Value* sub = builder.createSub(left, limit);
        IR::Value* tmp = builder.createFptosi(sub, toType);
        builder.createStore(result, builder.createOr(tmp, big));
        builder.createJump(merge);

        builder.setCurrentBlock(merge);
        IR::Value* res = builder.createLoad(result);
        instruction->getParentBlock()->getParentFunction()->replace(instruction, res);
        instruction->getParentBlock()->removeInstruction(instruction);
        restart();
        return true;
    }
    return false;
}

void x64Legalizer::init(Unit& unit) {
    for(auto& func : unit.getFunctions()) {
        if(!func->hasBody() || !func->getFunctionType()->isVarArg()) continue;
        CallingConvention cc = func->getCallingConvention();
        if(cc == CallingConvention::Count) cc = getDefaultCallingConvention(m_spec);
        if(cc != CallingConvention::x64SysV) continue;
        IR::Block* realEntry = func->getEntryBlock();
        IR::Block* vaHeader = func->insertBlockBefore(realEntry, "vaheader");
        IR::Builder bb(m_context);
        bb.setCurrentBlock(vaHeader);
        bb.createJump(realEntry);
    }
}

}