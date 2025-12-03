#include "target/AArch64/AArch64_asm_printer.hpp"
#include "MIR/function.hpp"
#include "printer_util.hpp"
#include "target/AArch64/AArch64_instruction_info.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "unit.hpp"

#include <iomanip>
#include <limits>

namespace scbe::Target::AArch64 {

void AArch64AsmPrinter::print(IR::Constant* constant) {
    if(constant->isConstantString()) {
        IR::ConstantString* str = cast<IR::ConstantString>(constant);
        m_output << ".asciz \"" << escapeString(str->getValue()) << "\"";
    }
    else if(constant->isConstantFloat()) {
        IR::ConstantFloat* floatVal = cast<IR::ConstantFloat>(constant);
        if(cast<FloatType>(floatVal->getType())->getBits() == 32) {
            m_output << ".float " << std::fixed << std::setprecision(std::numeric_limits<float>::max_digits10)
                << floatVal->getValue();
        }
        else {
            m_output << ".double " << std::fixed << std::setprecision(std::numeric_limits<double>::max_digits10)
                << floatVal->getValue();
        }
    }
    else if(constant->isConstantInt()) {
        IR::ConstantInt* intVal = cast<IR::ConstantInt>(constant);
        IntegerType* type = (IntegerType*)(intVal->getType());
        if(type->getBits() == 64) {
            m_output << ".quad " << intVal->getValue();
        }
        else if(type->getBits() == 32) {
            m_output << ".long " << intVal->getValue();
        }
        else if(type->getBits() == 16) {
            m_output << ".short " << intVal->getValue();
        }
        else if(type->getBits() == 8) {
            m_output << ".byte " << intVal->getValue();
        }
    }
    else if(constant->isConstantStruct()) {
        for(auto& value : cast<IR::ConstantStruct>(constant)->getValues()) {
            print(value);
            m_output << "\n";
        }
    }
    else if(constant->isConstantArray()) {
        for(auto& value : cast<IR::ConstantArray>(constant)->getValues()) {
            print(value);
            m_output << "\n";
        }
    }
    else if(constant->isFunction()) {
        m_output << ".quad " << cast<IR::Function>(constant)->getMachineFunction()->getName();
    }
    else if(constant->isBlock()) {
        m_output << ".quad " << cast<IR::Block>(constant)->getMIRBlock()->getName();
    }
    else if(constant->isNullValue()) {
        m_output << ".quad 0";
    }
    else if(constant->isConstantGEP()) {
        IR::ConstantGEP* gep = cast<IR::ConstantGEP>(constant);
        m_output << ".quad " << gep->getBase()->getName();
        size_t off = gep->calculateOffset(m_dataLayout);
        if(off != 0) m_output << " + " << off;
    }
    else if(constant->isGlobalVariable()) {
        m_output << ".quad " << constant->getName();
    }
}

void AArch64AsmPrinter::init(Unit& unit) {
    
    for(auto& global : unit.getGlobals()) {
        switch(global->getKind()) {
            case IR::GlobalValue::ValueKind::GlobalVariable: {
                IR::GlobalVariable* var = cast<IR::GlobalVariable>(global.get());
                if(var->getValue()) {
                    m_output << var->getName() << ":\n";
                    print(var->getValue());
                    break;
                }
                m_output << ".extern " << var->getName();
                break;
            }
            default:
                continue;
        }
        m_output << "\n";
    }

    m_output << ".text\n\n";
    for(auto& func : unit.getFunctions()) {
        if(!func->hasBody())
            m_output << ".extern " << func->getMachineFunction()->getName() << "\n";
        else {
            m_output << ".align 4\n";
            m_output << ".global " << func->getMachineFunction()->getName() << "\n";
        }
    }
    m_output << "\n";
}

void AArch64AsmPrinter::end(Unit& unit) {}

void AArch64AsmPrinter::print(MIR::Function* function) {
    m_output << function->getName() << ":\n";
    for(auto& block : function->getBlocks()) {
        print(block.get());
    }
}

void AArch64AsmPrinter::print(MIR::Block* block) {
    if(block != block->getParentFunction()->getEntryBlock())
        m_output << block->getName() << ":\n";

    for(auto& instruction : block->getInstructions()) {
        print(instruction.get());
        m_output << "\n";
    }
    m_output << "\n";
}

void AArch64AsmPrinter::print(MIR::Instruction* instruction) {
    m_output << "  ";
    if(instruction->getOpcode() == CALL_LOWER_OP) {
        m_output << "?";
        return;
    }
    m_output << m_instructionInfo->getMnemonic(instruction->getOpcode()).getName() << " ";
    InstructionDescriptor desc = m_instructionInfo->getInstructionDescriptor(instruction->getOpcode());

    if(desc.mayLoad() || desc.mayStore()) {
        size_t nonMemoryOperands = desc.getNumOperands() - 4;
        for(size_t i = 0; i < nonMemoryOperands; i++) {
            auto rest = desc.getRestriction(i);
            print(instruction->getOperands().at(i), &rest);
            m_output << ", ";
        }
        printMemory(
            cast<MIR::Register>(instruction->getOperands().at(nonMemoryOperands)),
            instruction->getOperands().at(nonMemoryOperands + 1),
            cast<MIR::ImmediateInt>(instruction->getOperands().at(nonMemoryOperands + 2)),
            cast<MIR::Symbol>(instruction->getOperands().at(nonMemoryOperands + 3))
        );
        return;
    }

    for(size_t i = 0; i < instruction->getOperands().size(); i++) {
        auto rest = desc.getRestriction(i);
        print(instruction->getOperands().at(i), &rest);
        if(i < instruction->getOperands().size() - 1)
            m_output << ", ";
    }
}

void AArch64AsmPrinter::print(MIR::Operand* operand, Restriction* restriction) {
    if(!operand) {
        m_output << "_";
        return;
    }

    if(operand->hasFlag(ExprModLow12)) m_output << ":lo12:";
    else if(operand->hasFlag(ExprModHigh12)) m_output << ":hi12:";

    switch(operand->getKind()) {
        case MIR::Operand::Kind::Register: {
            auto reg = cast<MIR::Register>(operand);

            // should not happen in codegen, keep for debugging purposes
            if(!m_registerInfo->isPhysicalRegister(reg->getId())) {
                m_output << "%" << reg->getId();
                return;
            }
            m_output << m_registerInfo->getRegisterDesc(reg->getId()).getName();
            return;
        }
        case MIR::Operand::Kind::ImmediateInt: {
            if(operand->hasFlag(AArch64OperandFlags::ShiftLeft)) {
                m_output << "LSL ";
            }
            else if(operand->hasFlag(AArch64OperandFlags::Conditional)) {
                AArch64Conditions cond = (AArch64Conditions)cast<MIR::ImmediateInt>(operand)->getValue();
                switch(cond) {
                    case AArch64Conditions::Eq: m_output << "eq"; break;
                    case AArch64Conditions::Ne: m_output << "ne"; break;
                    case AArch64Conditions::Cs: m_output << "cs"; break;
                    case AArch64Conditions::Cc: m_output << "cc"; break;
                    case AArch64Conditions::Mi: m_output << "mi"; break;
                    case AArch64Conditions::Pl: m_output << "pl"; break;
                    case AArch64Conditions::Vs: m_output << "vs"; break;
                    case AArch64Conditions::Vc: m_output << "vc"; break;
                    case AArch64Conditions::Hi: m_output << "hi"; break;
                    case AArch64Conditions::Ls: m_output << "ls"; break;
                    case AArch64Conditions::Ge: m_output << "ge"; break;
                    case AArch64Conditions::Lt: m_output << "lt"; break;
                    case AArch64Conditions::Gt: m_output << "gt"; break;
                    case AArch64Conditions::Le: m_output << "le"; break;
                    case AArch64Conditions::Al: m_output << "al"; break;
                    default: m_output << "?"; break;
                }
                return;
            }
            m_output << cast<MIR::ImmediateInt>(operand)->getValue();
            return;
        }
        case MIR::Operand::Kind::Block: {
            auto bb = cast<MIR::Block>(operand);
            m_output << bb->getName();
            return;
        }
        case MIR::Operand::Kind::GlobalAddress: {
            auto global = cast<MIR::GlobalAddress>(operand);
            m_output << global->getName();
            return;
        }
        case MIR::Operand::Kind::ExternalSymbol: {
            auto symbol = cast<MIR::ExternalSymbol>(operand);
            m_output << symbol->getName();
            return;
        }
        case MIR::Operand::Kind::ConstantIndex: {
            auto ci = cast<MIR::ConstantIndex>(operand);
            m_output << ci->getName();
            return;
        }
        default: break;
    }

    m_output << "?";
}

void AArch64AsmPrinter::printMemory(MIR::Register* base, MIR::Operand* offset, MIR::ImmediateInt* indexing, MIR::Symbol* symbol) {
    Indexing idx = (Indexing)cast<MIR::ImmediateInt>(indexing)->getValue();

    if(idx == Indexing::PostIndexed) {
        m_output << "[";
        print(base);
        m_output << "]";
        if(!offset->isImmediateInt() || cast<MIR::ImmediateInt>(offset)->getValue() != 0) {
            m_output << ", ";
            print(offset);
        }
        else if(symbol) {
            m_output << ", ";
            print(symbol);
        }
        return;
    }
    m_output << "[";
    print(base);
    if(!offset->isImmediateInt() || cast<MIR::ImmediateInt>(offset)->getValue() != 0) {
        m_output << ", ";
        print(offset);
    }
    else if(symbol) {
        m_output << ", ";
        print(symbol);
    }
    m_output << "]";
    if(idx == Indexing::PreIndexed)
        m_output << "!";
}


}