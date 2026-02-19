#include "IR/printer.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/value.hpp"
#include "printer_util.hpp"
#include "type.hpp"
#include "unit.hpp"
#include <ostream>

namespace scbe::IR {

void Printer::printIndentation() {
    for(int i = 0; i < m_indent; i++) m_output << "\t";
}

void HumanPrinter::print(Unit& unit) {
    m_output << "Unit " << unit.getName() << "\n\n";

    for(auto& global : unit.getGlobals()) {
        m_output << "global ";
        if(global->getName().empty()) m_output << "(anonymous) ";
        else m_output << global->getName() << " ";

        if(global->isConstant()) m_output << "constant ";

        print(global->getType());

        if(global->getValue()) {
            m_output << " = ";
            print(global->getValue());
        }
        m_output << "\n";
    }

    if(!unit.getGlobals().empty()) m_output << "\n";

    for(auto& function : unit.getFunctions())
        print(function.get());

    for(const StructType* deferred : m_deferPrintStruct) {
        m_output << deferred->getName() << " { ";
        for(size_t i = 0; i < deferred->getContainedTypes().size(); i++) {
            print(deferred->getContainedTypes().at(i));
            if(i < deferred->getContainedTypes().size() - 1)
                m_output << ", ";
        }
        m_output << " };\n";
    }
    m_output << "\n";
}

void HumanPrinter::print(const Function* function) {
    if(!function->hasBody()) {
        m_output << "extern ";
    }
    m_output << "fn " << function->getName();

    m_output << "(";
    for(int i = 0; i < function->getArguments().size(); i++) {
        print(function->getArguments().at(i).get());

        if(i < function->getFunctionType()->getArguments().size() - 1)
            m_output << ", ";
        else if(function->getFunctionType()->isVarArg())
            m_output << ", ...";
    }
    m_output << ")";

    m_output << " -> ";
    print(function->getFunctionType()->getReturnType());

    if(!function->hasBody()) {
        m_output << ";\n\n";
        return;
    }

    m_output << " {\n";

    for(auto& block : function->getBlocks()) {
        print(block.get());
    }
    
    m_output << "}\n\n";
}

void HumanPrinter::print(const Block* block) {
    printIndentation();
    m_output << block->getName() << ":\n";
    m_indent++;
    for(auto& instruction : block->getInstructions()) {
        print(instruction.get());
        m_output << "\n";
    }
    m_indent--;
}

void HumanPrinter::print(const Instruction* instruction) {
    printIndentation();

    switch(instruction->getOpcode()) {
        case Instruction::Opcode::Allocate: {
            auto allocateInstruction = static_cast<const AllocateInstruction*>(instruction);
            m_output << "%" << instruction->getName() << " = allocate ";
            auto typeToPrint = allocateInstruction->getType()->isArrayType() ? allocateInstruction->getType() : ((PointerType*)allocateInstruction->getType())->getPointee();
            print(typeToPrint);
            if(instruction->getNumOperands() == 1) {
                m_output << ", ";
                print(instruction->getOperand(0));
            }
            return;
        }
        case Instruction::Opcode::Call: {
            if(instruction->getUses().size() > 0)
                m_output << "%" << instruction->getName() << " = ";
            m_output << "call ";
            auto callInstruction = static_cast<const CallInstruction*>(instruction);
            print(callInstruction->getCallee());
            m_output << "(";
            for(int i = 0; i < callInstruction->getArguments().size(); i++) {
                print(callInstruction->getArguments()[i]);
                if(i < callInstruction->getArguments().size() - 1)
                    m_output << ", ";
            }
            m_output << ")";
            return;
        }
        case Instruction::Opcode::Switch: {
            auto switchInstruction = static_cast<const SwitchInstruction*>(instruction);
            m_output << "switch ";
            print(switchInstruction->getCondition());
            m_output << " ";
            print((const Value*)switchInstruction->getDefaultCase());
            m_output << " {\n";
            for(auto& casePair : switchInstruction->getCases()) {
                printIndentation();
                printIndentation();
                print(casePair.first);
                m_output << " -> ";
                print((const Value*)casePair.second);
                m_output << "\n";
            }
            printIndentation();
            m_output << "}";
            return;
        }
        default:
            break;
    }

    if(!instruction->getName().empty())
        m_output << "%" << instruction->getName() << " = ";
    
    if(instruction->isCast()) {
        const CastInstruction* castInstruction = static_cast<const CastInstruction*>(instruction);
        print(castInstruction->getType());
        m_output << " ";
    }
    m_output << Instruction::opcodeString(instruction->getOpcode());
    m_output << " ";
    for(int i = 0; i < instruction->getNumOperands(); i++) {
        print(instruction->getOperand(i));
        if(i < instruction->getNumOperands() - 1)
            m_output << ", ";
    }
}

void HumanPrinter::print(const Value* value) {
  switch (value->getKind()) {
    case Value::ValueKind::ConstantInt: {
        print(value->getType());
        m_output << " ";
        auto constantInt = (const ConstantInt*)value;
        m_output << constantInt->getValue();
        break;
    }
    case Value::ValueKind::ConstantFloat: {
        print(value->getType());
        m_output << " ";
        auto constantFloat = (const ConstantFloat*)value;
        m_output << constantFloat->getValue();
        break;
    }
    case Value::ValueKind::ConstantString: {
        print(value->getType());
        m_output << " ";
        auto constantString = (const ConstantString*)value;
        m_output << "\"" << escapeString(constantString->getValue()) << "\"";
        break;
    }
    case Value::ValueKind::ConstantStruct: {
        print(value->getType());
        auto constantStruct = (const ConstantStruct*)value;
        m_output << " { ";
        for (int i = 0; i < constantStruct->getValues().size(); i++) {
            print(constantStruct->getValues().at(i));
            if (i != constantStruct->getValues().size() - 1) {
                m_output << ", ";
            }
        }
        m_output << " }";
        break;
    }
    case Value::ValueKind::ConstantArray: {
        print(value->getType());
        auto constantArray = (const ConstantArray*)value;
        m_output << " { ";
        for (int i = 0; i < constantArray->getValues().size(); i++) {
            print(constantArray->getValues().at(i));
            if (i != constantArray->getValues().size() - 1) {
                m_output << ", ";
            }
        }
        m_output << " }";
        break;
    }
    case Value::ValueKind::Block:
        m_output << value->getName();
        break;
    case Value::ValueKind::Function: {
        Function* function = (Function*)value;
        print(function->getFunctionType());
        m_output << " ";
        m_output << function->getName();
        break;
    }
    case Value::ValueKind::Register:
    case Value::ValueKind::FunctionArgument:
        print(value->getType());
        m_output << " %" << value->getName();
        break;
    case Value::ValueKind::GlobalVariable:
        print(value->getType());
        m_output << " @" << value->getName();
        break;
    case Value::ValueKind::UndefValue:
        print(value->getType());
        m_output << " undef";
        break;
    case Value::ValueKind::NullValue:
        print(value->getType());
        m_output << " null";
        break;
    case Value::ValueKind::ConstantGEP: {
        const IR::ConstantGEP* gep = cast<const IR::ConstantGEP>(value);
        print(value->getType());
        m_output <<  " const getelementptr ";
        print(gep->getBase());
        m_output << " { ";
        for(size_t i = 0; i < gep->getIndices().size(); i++) {
            print(gep->getIndices().at(i));
            if (i != gep->getIndices().size() - 1) m_output << ", ";
        }
        m_output << " }";
        break;
    }
    }
}

void HumanPrinter::print(const Type* type) {
  switch (type->getKind()) {
    case Type::TypeKind::Integer: {
        auto intType = (const IntegerType*)type;
        m_output << "i" << (int)intType->getBits();
        break;
    }
    case Type::TypeKind::Float: {
        auto floatType = (const FloatType*)type;
        m_output << "f" << (int)floatType->getBits();
        break;
    }
    case Type::TypeKind::Pointer: {
        auto pointerType = (const PointerType*)type;
        print(pointerType->getPointee());
        m_output << "*";
        break;
    }
    case Type::TypeKind::Void: {
        m_output << "void";
        break;
    }
    case Type::TypeKind::Struct: {
        auto basicType = (const StructType*)type;
        m_output << basicType->getName();
        m_deferPrintStruct.insert(basicType);
        break;
    }
    case Type::TypeKind::Array: {
        auto arrayType = (const ArrayType*)type;
        print(arrayType->getElement());
        m_output << "[" << arrayType->getScale() << "]";
        break;
    }
    case Type::TypeKind::Function:
        auto fnType = (FunctionType*)type;
        print(fnType->getReturnType());
        m_output << "(";
        for(size_t i = 0; i < fnType->getArguments().size(); i++) {
            print(fnType->getArguments()[i]);
            if(i != fnType->getArguments().size() - 1) {
                m_output << ", ";
            }
            else if(fnType->isVarArg())
                m_output << ", ...";
            }
        m_output << ")";
        break;
  }
}
    
}