#include "target/x64/x64_asm_printer.hpp"
#include "IR/function.hpp"
#include "IR/global_value.hpp"
#include "IR/value.hpp"
#include "IR/block.hpp"
#include "MIR/function.hpp"
#include "MIR/instruction.hpp"
#include "MIR/operand.hpp"
#include "cast.hpp"
#include "printer_util.hpp"
#include "target/instruction_info.hpp"
#include "target/target_specification.hpp"
#include "type.hpp"
#include "unit.hpp"
#include <iomanip>
#include <limits>

namespace scbe::Target::x64 {


void x64AsmPrinter::print(MIR::Function* function) {
    m_output << function->getName() << ":\n";
    for(auto& block : function->getBlocks()) {
        print(block.get());
    }
}

void x64AsmPrinter::print(MIR::Block* block) {
    if(block != block->getParentFunction()->getEntryBlock())
        m_output << block->getName() << ":\n";

    for(auto& instruction : block->getInstructions()) {
        print(instruction.get());
        m_output << "\n";
    }
    m_output << "\n";
}

void x64AsmPrinter::print(MIR::Instruction* instruction) {
    m_output << "  ";
    if(instruction->getOpcode() == CALL_LOWER_OP) {
        m_output << "?";
        return;
    }
    m_output << m_instructionInfo->getMnemonic(instruction->getOpcode()).getName() << " ";

    InstructionDescriptor desc = m_instructionInfo->getInstructionDescriptor(instruction->getOpcode());
    
    if(desc.isLoad()) {
        print(instruction->getOperands().at(0));
        m_output << ", ";
        printMemory(desc.getSize(),
            cast<MIR::Register>(instruction->getOperands().at(1)),
            ((MIR::ImmediateInt*)instruction->getOperands().at(4))->getValue(),
            cast<MIR::Register>(instruction->getOperands().at(3)),
            ((MIR::ImmediateInt*)instruction->getOperands().at(2))->getValue(),
            cast<MIR::Symbol>(instruction->getOperands().at(5))
        );
        return;
    }

    if(desc.isStore()) {
        printMemory(desc.getSize(),
            cast<MIR::Register>(instruction->getOperands().at(0)),
            ((MIR::ImmediateInt*)instruction->getOperands().at(3))->getValue(),
            cast<MIR::Register>(instruction->getOperands().at(2)),
            ((MIR::ImmediateInt*)instruction->getOperands().at(1))->getValue(),
            cast<MIR::Symbol>(instruction->getOperands().at(4))
        );
        m_output << ", ";
        print(instruction->getOperands().at(5));
        return;
    }

    for(size_t i = 0; i < instruction->getOperands().size(); i++) {
        print(instruction->getOperands().at(i));
        if(i < instruction->getOperands().size() - 1)
            m_output << ", ";
    }
}

void x64AsmPrinter::print(MIR::Operand* operand, Restriction* restriction) {
    if(!operand) {
        m_output << "_";
        return;
    }
        
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
            m_output << cast<MIR::ImmediateInt>(operand)->getValue();
            return;
        }
        case MIR::Operand::Kind::Block: {
            auto bb = cast<MIR::Block>(operand);
            m_output << bb->getName();
            return;
        }
        case MIR::Operand::Kind::ExternalSymbol:
        case MIR::Operand::Kind::GlobalAddress: {
            auto symbol = cast<MIR::Symbol>(operand);
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



void x64AsmPrinter::printMemory(size_t size, MIR::Register* base, int64_t offset, MIR::Register* index, int64_t scale, MIR::Symbol* symbol) {
    switch(size) {
        default: break;
        case 1: m_output << "BYTE PTR "; break;
        case 2: m_output << "WORD PTR "; break;
        case 4: m_output << "DWORD PTR "; break;
        case 8: m_output << "QWORD PTR "; break;
        case 16: m_output << "OWORD PTR "; break;
        case 32: m_output << "XMMWORD PTR "; break;
        case 64: m_output << "YMMWORD PTR "; break;
    }

    m_output << "[";
    print(base);
    if(index) {
        m_output << " + ";
        print(index);
        if(scale != 1) {
            m_output << " * ";
            m_output << scale;
        }
    }
    if(offset != 0) {
        if(offset >= 0)
            m_output << " + " << offset;
        else
            m_output << " - " << -offset;
    }

    if(symbol) {
        m_output << " + " << symbol->getName();
    }

    m_output << "]";
}

void x64AsmPrinter::print(IR::Constant* constant) {
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

void x64AsmPrinter::init(Unit& unit) {
#ifdef __linux__
    m_output << ".section .note.GNU-stack,\"\",@progbits\n";
#endif
    m_output << ".intel_syntax noprefix\n\n";

    m_output << ".section .data\n";

    for(auto& global : unit.getGlobals()) {
        switch(global->getKind()) {
            case IR::GlobalValue::ValueKind::GlobalVariable: {
                IR::GlobalVariable* var = cast<IR::GlobalVariable>(global.get());
                if(var->getValue()) {
                    m_output << var->getName() << ":\n";
                    print(var->getValue());
                    m_output << "\n";
                    break;
                }
                // TODO: non-pic can have extern variables
                // m_output << ".extern " << var->getName();
                break;
            }
            default:
                continue;
        }
    }
    
    m_output << "\n.section .text\n";
    for(auto& func : unit.getFunctions()) {
        if(!func->hasBody())
            m_output << ".extern " << func->getMachineFunction()->getName() << "\n";
        else
            m_output << ".global " << func->getMachineFunction()->getName() << "\n";
    }
    m_output << "\n";
}

void x64AsmPrinter::end(Unit& unit) {

}

}