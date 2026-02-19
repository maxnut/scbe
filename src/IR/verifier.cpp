#include "IR/verifier.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/instruction.hpp"
#include "type.hpp"
#include <algorithm>

namespace scbe::IR {

bool Verifier::run(Function* function) {
    for(auto& arg : function->getArguments()) {
        if(arg->getType()->isIntType() || arg->getType()->isFltType() || arg->getType()->isPtrType()) continue;
        m_diagnosticEmitter->error("Function " + function->getName() + " has unsupported parameter type", function->getSourceLocation());
    }
    for(auto& block : function->getBlocks()) {
        verify(block.get());
    }
    return false;
}

void Verifier::verify(Block* block) {
    if(block->getInstructions().empty())
        m_diagnosticEmitter->error("Block " + block->getName() + " has no instructions", block->getSourceLocation());
    else if(!block->isTerminator())
        m_diagnosticEmitter->error("Block " + block->getName() + " has no terminator", block->getSourceLocation());

    for(auto& instruction : block->getInstructions()) {
        verify(instruction.get());
    }
}

void Verifier::verify(Instruction* instruction) {
    for(size_t i = 0; i < instruction->getNumOperands(); i++) {
        Value* op = instruction->getOperand(i);
        if(Instruction* ins = dyn_cast<Instruction>(op)) {
            if(ins->getParentBlock()->getParentFunction() == instruction->getParentBlock()->getParentFunction()) continue;
            std::string err = "Instruction ";
            if(!instruction->getName().empty()) err += instruction->getName() + " ";
            err += "has operand n." + std::to_string(i) + " located in a different function (" + ins->getParentBlock()->getParentFunction()->getName() + ")";
            m_diagnosticEmitter->error(err, instruction->getSourceLocation());
        }

        if(std::find(op->getUses().begin(), op->getUses().end(), instruction) == op->getUses().end()) {
            std::string err = "Instruction ";
            if(!instruction->getName().empty()) err += instruction->getName() + " ";
            err += "uses operand n." + std::to_string(i) + " but was not found in the operands uses";
            m_diagnosticEmitter->error(err, instruction->getSourceLocation());
        }
    }

    std::string opcodeString = Instruction::opcodeString(instruction->getOpcode());

    switch(instruction->getOpcode()) {
        case Instruction::Opcode::Ret: {
            if(instruction->getOperands().empty()) break;
            if(instruction->getNumOperands() > 1) {
                m_diagnosticEmitter->error("return instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            Type* retType = instruction->getParentBlock()->getParentFunction()->getFunctionType()->getReturnType();
            if(instruction->getOperand(0)->getType() != retType)
                m_diagnosticEmitter->error("return instruction has mismatched type", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::Load: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error("load instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getOperand(0)->getType()->isPtrType())
                m_diagnosticEmitter->error("load instruction does not reference a pointer", instruction->getSourceLocation());
            
            break;
        }
        case Instruction::Opcode::Store: {
            if(instruction->getNumOperands() != 2) {
                m_diagnosticEmitter->error("store instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getOperand(0)->getType()->isPtrType())
                m_diagnosticEmitter->error("store instruction does not reference a pointer", instruction->getSourceLocation());
            else if(cast<PointerType>(instruction->getOperand(0)->getType())->getPointee() != instruction->getOperand(1)->getType())
                m_diagnosticEmitter->error("store instruction has mismatched types", instruction->getSourceLocation());
            
            break;
        }
        case Instruction::Opcode::Add: {
            if(instruction->getNumOperands() != 2) {
                m_diagnosticEmitter->error("add instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(instruction->getOperand(0)->getType() != instruction->getOperand(1)->getType())
                m_diagnosticEmitter->error("add instruction has mismatched types", instruction->getSourceLocation());

            if((!instruction->getOperand(0)->getType()->isFltType() && !instruction->getOperand(0)->getType()->isIntType()) ||
                    (!instruction->getOperand(1)->getType()->isFltType() && !instruction->getOperand(1)->getType()->isIntType()))
                m_diagnosticEmitter->error("add instruction has unsupported types", instruction->getSourceLocation());
            
            break;
        }
        case Instruction::Opcode::Sub: {
            if(instruction->getNumOperands() != 2) {
                m_diagnosticEmitter->error("sub instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(instruction->getOperand(0)->getType() != instruction->getOperand(1)->getType())
                m_diagnosticEmitter->error("sub instruction has mismatched types", instruction->getSourceLocation());
            if((!instruction->getOperand(0)->getType()->isFltType() && !instruction->getOperand(0)->getType()->isIntType()) ||
                    (!instruction->getOperand(1)->getType()->isFltType() && !instruction->getOperand(1)->getType()->isIntType()))
                m_diagnosticEmitter->error("sub instruction has unsupported types", instruction->getSourceLocation());
            
            break;
        }
        case Instruction::Opcode::ICmpEq:
        case Instruction::Opcode::ICmpNe:
        case Instruction::Opcode::ICmpGt:
        case Instruction::Opcode::ICmpGe:
        case Instruction::Opcode::ICmpLt:
        case Instruction::Opcode::ICmpLe:
        case Instruction::Opcode::UCmpGt:
        case Instruction::Opcode::UCmpGe:
        case Instruction::Opcode::UCmpLt:
        case Instruction::Opcode::UCmpLe: {
            if(instruction->getNumOperands() != 2) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(instruction->getOperand(0)->getType() != instruction->getOperand(1)->getType())
                m_diagnosticEmitter->error(opcodeString + " instruction has mismatched types", instruction->getSourceLocation());
            if((!instruction->getOperand(0)->getType()->isIntType() && !instruction->getOperand(0)->getType()->isPtrType())
                    || (!instruction->getOperand(1)->getType()->isIntType() && !instruction->getOperand(1)->getType()->isPtrType()))
                m_diagnosticEmitter->error(opcodeString + " instruction has unsupported types", instruction->getSourceLocation());
            
            break;
        }
        case Instruction::Opcode::FCmpEq:
        case Instruction::Opcode::FCmpNe:
        case Instruction::Opcode::FCmpGt:
        case Instruction::Opcode::FCmpGe:
        case Instruction::Opcode::FCmpLt:
        case Instruction::Opcode::FCmpLe: {
            if(instruction->getNumOperands() != 2) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(instruction->getOperand(0)->getType() != instruction->getOperand(1)->getType())
                m_diagnosticEmitter->error(opcodeString + " instruction has mismatched types", instruction->getSourceLocation());
            if(!instruction->getOperand(0)->getType()->isFltType() || !instruction->getOperand(1)->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " instruction has unsupported types", instruction->getSourceLocation());
            
            break;
        }
        case Instruction::Opcode::Jump: {
            if(instruction->getNumOperands() == 1) {
                Function* ff = cast<Block>(instruction->getOperand(0))->getParentFunction();
                if(instruction->getParentBlock()->getParentFunction() != ff)
                    m_diagnosticEmitter->error("jump destination is in a different function (" + ff->getName() + ")", instruction->getSourceLocation());
                break;
            }
            else if(instruction->getNumOperands() == 3) {
                Function* ff1 = cast<Block>(instruction->getOperand(0))->getParentFunction();
                Function* ff2 = cast<Block>(instruction->getOperand(1))->getParentFunction();
                if(instruction->getParentBlock()->getParentFunction() != ff1)
                    m_diagnosticEmitter->error("jump destination 1 is in a different function (" + ff1->getName() + ")", instruction->getSourceLocation());
                if(instruction->getParentBlock()->getParentFunction() != ff2)
                    m_diagnosticEmitter->error("jump destination 2 is in a different function (" + ff2->getName() + ")", instruction->getSourceLocation());

                if(!instruction->getOperand(2)->getType()->isIntType() && !instruction->getOperand(2)->getType()->isPtrType())
                    m_diagnosticEmitter->error("jump condition has unsupported type", instruction->getSourceLocation());

                break;
            }

            m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
            break;
        }
        case Instruction::Opcode::Phi: {
            if(instruction->getOperands().empty() || instruction->getNumOperands() % 2 != 0) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            for(size_t i = 0; i < instruction->getNumOperands(); i += 2) {
                Value* val = instruction->getOperand(i);
                Block* block = cast<Block>(instruction->getOperand(i+1));

                if(instruction->getParentBlock()->getParentFunction() != block->getParentFunction())
                    m_diagnosticEmitter->error("phi's incoming block n." + std::to_string(i/2) + " is in a different function (" + block->getParentFunction()->getName() + ")", instruction->getSourceLocation());

                if(val->getType() != instruction->getType())
                    m_diagnosticEmitter->error("phi instruction has mismatched types", instruction->getSourceLocation());
            }
            break;
        }
        case Instruction::Opcode::GetElementPtr: {
            if(instruction->getNumOperands() < 2) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getOperand(0)->getType()->isPtrType() && !instruction->getOperand(0)->getType()->isArrayType())
                m_diagnosticEmitter->error("getelementpointer has unsupported base type", instruction->getSourceLocation());

            for(size_t i = 1; i < instruction->getNumOperands(); i++) {
                if(instruction->getOperand(i)->getType()->isIntType()) continue;
                m_diagnosticEmitter->error("getelementpointer has unsupported index type for n." + std::to_string(i), instruction->getSourceLocation());
                break;
            }
            break;
        }
        case Instruction::Opcode::Call: {
            if(instruction->getOperands().empty()) {
                m_diagnosticEmitter->error("call instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getOperand(0)->getType()->isPtrType() || !cast<PointerType>(instruction->getOperand(0)->getType())->getPointee()->isFuncType()) {
                m_diagnosticEmitter->error("call instruction has unexpected callee type", instruction->getSourceLocation());
                break;
            }

            FunctionType* ty = cast<FunctionType>(cast<PointerType>(instruction->getOperand(0)->getType())->getPointee());
            CallInstruction* call = cast<CallInstruction>(instruction);
            for(size_t i = 0; i < call->getArguments().size(); i++) {
                if(ty->isVarArg() && i >= ty->getArguments().size()) break;
                if(ty->getArguments()[i] == call->getArguments()[i]->getType()) continue;
                m_diagnosticEmitter->error("call instruction has unexpected argument type for n." + std::to_string(i), instruction->getSourceLocation());
                break;
            }
            break;
        }
        case Instruction::Opcode::Zext:
        case Instruction::Opcode::Sext:
        case Instruction::Opcode::Trunc: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            bool err = false;
            if(!instruction->getType()->isIntType()) {
                m_diagnosticEmitter->error(opcodeString + " has unsupported cast type", instruction->getSourceLocation());
                err = true;
            }
            if(!instruction->getOperand(0)->getType()->isIntType()) {
                m_diagnosticEmitter->error(opcodeString + " has unsupported operand type", instruction->getSourceLocation());
                err = true;
            }

            if(err) break;

            if(instruction->getOpcode() == Instruction::Opcode::Trunc) {
                if(cast<IntegerType>(instruction->getType())->getBits() >= cast<IntegerType>(instruction->getOperand(0)->getType())->getBits())
                    m_diagnosticEmitter->error("trunc instruction does not cast to a smaller type", instruction->getSourceLocation());
            }
            else if(cast<IntegerType>(instruction->getType())->getBits() <= cast<IntegerType>(instruction->getOperand(0)->getType())->getBits()) {
                m_diagnosticEmitter->error(opcodeString + " instruction does not cast to a bigger type", instruction->getSourceLocation());
            }

            break;
        }
        case Instruction::Opcode::Fptrunc: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported cast type", instruction->getSourceLocation());
            if(!instruction->getOperand(0)->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported operand type", instruction->getSourceLocation());

            if(cast<FloatType>(instruction->getType())->getBits() >= cast<FloatType>(instruction->getOperand(0)->getType())->getBits())
                m_diagnosticEmitter->error("fptrunc instruction does not cast to a smaller type", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::Fpext: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported cast type", instruction->getSourceLocation());
            if(!instruction->getOperand(0)->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported operand type", instruction->getSourceLocation());

            if(cast<FloatType>(instruction->getType())->getBits() <= cast<FloatType>(instruction->getOperand(0)->getType())->getBits())
                m_diagnosticEmitter->error("fpext instruction does not cast to a bigger type", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::Fptosi:
        case Instruction::Opcode::Fptoui: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getType()->isIntType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported cast type", instruction->getSourceLocation());
            if(!instruction->getOperand(0)->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported operand type", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::Sitofp:
        case Instruction::Opcode::Uitofp: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported cast type", instruction->getSourceLocation());
            if(!instruction->getOperand(0)->getType()->isIntType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported operand type", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::Bitcast: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error("bitcast instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(m_layout->getSize(instruction->getType()) != m_layout->getSize(instruction->getOperand(0)->getType()))
                m_diagnosticEmitter->error("bitcast has types of different size ", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::Ptrtoint: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error("ptrtoint instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getType()->isIntType())
                m_diagnosticEmitter->error("ptrtoint has unsupported cast type", instruction->getSourceLocation());
            if(!instruction->getOperand(0)->getType()->isPtrType())
                m_diagnosticEmitter->error("ptrtoint has unsupported operand type", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::Inttoptr: {
            if(instruction->getNumOperands() != 1) {
                m_diagnosticEmitter->error("inttoptr instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getType()->isPtrType())
                m_diagnosticEmitter->error("inttoptr has unsupported cast type", instruction->getSourceLocation());
            if(!instruction->getOperand(0)->getType()->isIntType())
                m_diagnosticEmitter->error("Inttoptr has unsupported operand type", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::ShiftLeft:
        case Instruction::Opcode::LShiftRight:
        case Instruction::Opcode::AShiftRight: {
            if(instruction->getNumOperands() != 2) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getOperand(0)->getType()->isIntType())
                m_diagnosticEmitter->error(opcodeString  + " has unsupported left op type", instruction->getSourceLocation());
            if(!instruction->getOperand(1)->getType()->isIntType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported right op type", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::And:
        case Instruction::Opcode::Or:
        case Instruction::Opcode::Xor:
        case Instruction::Opcode::IRem:
        case Instruction::Opcode::URem:
        case Instruction::Opcode::IMul:
        case Instruction::Opcode::UMul:
        case Instruction::Opcode::IDiv:
        case Instruction::Opcode::UDiv: {
            if(instruction->getNumOperands() != 2) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getOperand(0)->getType()->isIntType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported left op type", instruction->getSourceLocation());
            if(!instruction->getOperand(1)->getType()->isIntType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported right op type", instruction->getSourceLocation());

            if(instruction->getOperand(0)->getType() != instruction->getOperand(1)->getType())
                m_diagnosticEmitter->error(opcodeString + " has mismatched types", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::FDiv:
        case Instruction::Opcode::FMul: {
            if(instruction->getNumOperands() != 2) {
                m_diagnosticEmitter->error(opcodeString + " instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            if(!instruction->getOperand(0)->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported left op type", instruction->getSourceLocation());
            if(!instruction->getOperand(1)->getType()->isFltType())
                m_diagnosticEmitter->error(opcodeString + " has unsupported right op type", instruction->getSourceLocation());

            if(instruction->getOperand(0)->getType() != instruction->getOperand(1)->getType())
                m_diagnosticEmitter->error(opcodeString + " has mismatched types", instruction->getSourceLocation());

            break;
        }
        case Instruction::Opcode::Switch: {
            if(instruction->getNumOperands() < 2) {
                m_diagnosticEmitter->error("switch instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }
            SwitchInstruction* swi = cast<SwitchInstruction>(instruction);

            if(!swi->getCondition()->getType()->isIntType())
                m_diagnosticEmitter->error("switch instruction has unsupported condition type", instruction->getSourceLocation());

            if(instruction->getParentBlock()->getParentFunction() != swi->getDefaultCase()->getParentFunction())
                m_diagnosticEmitter->error("switch instruction default destination is in a different function (" + swi->getDefaultCase()->getParentFunction()->getName() + ")", instruction->getSourceLocation());

            for(auto& casePair : swi->getCases()) {
                if(!casePair.first->getType()->isIntType())
                    m_diagnosticEmitter->error("switch case has unsupported condition type", instruction->getSourceLocation());

                if(instruction->getParentBlock()->getParentFunction() != casePair.second->getParentFunction())
                    m_diagnosticEmitter->error("switch instruction default destination is in a different function (" + casePair.second->getParentFunction()->getName() + ")", instruction->getSourceLocation());
            }
            break;
        }
        case Instruction::Opcode::ExtractValue: {
            if(instruction->getNumOperands() < 2) {
                m_diagnosticEmitter->error("extractvalue instruction has unexpected operand count", instruction->getSourceLocation());
                break;
            }

            ExtractValueInstruction* ex = cast<ExtractValueInstruction>(instruction);

            if(!ex->getAggregate()->getType()->isStructType())
                m_diagnosticEmitter->error("extractvalue case has unsupported aggregate type", instruction->getSourceLocation());
            if(!ex->getIndex()->getType()->isIntType())
                m_diagnosticEmitter->error("extractvalue case has unsupported index type", instruction->getSourceLocation());
            break;
        }
        case Instruction::Opcode::Allocate:
        case Instruction::Opcode::Count:
            break;
    }
}

}