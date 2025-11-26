#include "IR/constant_folder.hpp"
#include "IR/builder.hpp"
#include "IR/instruction.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/value.hpp"

namespace scbe::IR {

bool ConstantFolder::run(IR::Instruction* instruction) {
    IR::Value* result = nullptr;
    if(auto cst = dyn_cast<IR::CastInstruction>(instruction)) {
        result = m_folder.foldCast(cst->getOpcode(), cst->getValue(), cst->getType());
    }
    else if(auto binOp = dyn_cast<IR::BinaryOperator>(instruction)) {
        result = m_folder.foldBinOp(binOp->getOpcode(), binOp->getLHS(), binOp->getRHS());
    }
    else if(auto jmp = dyn_cast<IR::JumpInstruction>(instruction)) {
        if(jmp->getNumOperands() < 3) return false;

        if(jmp->getOperands().at(0) == jmp->getOperands().at(1)) {
            IR::Builder builder(m_context);
            builder.setCurrentBlock(instruction->getParentBlock());
            builder.setInsertPoint(instruction);
            builder.createJump(cast<IR::Block>(jmp->getOperand(0)));

            instruction->getParentBlock()->getParentFunction()->removeInstruction(instruction);
            restart();
            return true;
        }

        if(!jmp->getOperands().at(2)->isConstantInt()) return false;

        IR::ConstantInt* cond = cast<IR::ConstantInt>(jmp->getOperands().at(2));

        IR::Builder builder(m_context);
        builder.setCurrentBlock(instruction->getParentBlock());
        builder.setInsertPoint(instruction);
        builder.createJump(cond->getValue() ? cast<IR::Block>(jmp->getOperand(0)) : cast<IR::Block>(jmp->getOperand(1)));

        instruction->getParentBlock()->getParentFunction()->removeInstruction(instruction);
        restart();
        return true;
    }

    if(!result) return false;

    instruction->getParentBlock()->getParentFunction()->replace(instruction, result);
    instruction->getParentBlock()->getParentFunction()->removeInstruction(instruction);

    restart();

    return true;
}

}