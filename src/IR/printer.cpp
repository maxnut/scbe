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
            m_output << "}\n";
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
    print(instruction->getOpcode());
    m_output << " ";
    for(int i = 0; i < instruction->getNumOperands(); i++) {
        print(instruction->getOperand(i));
        if(i < instruction->getNumOperands() - 1)
            m_output << ", ";
    }
}

void HumanPrinter::print(Instruction::Opcode opcode) {
  switch (opcode) {
    case Instruction::Opcode::Load:
        m_output << "load";
        break;
    case Instruction::Opcode::Store:
        m_output << "store";
        break;
    case Instruction::Opcode::Add:
        m_output << "add";
        break;
    case Instruction::Opcode::Sub:
        m_output << "sub";
        break;
    case Instruction::Opcode::IMul:
        m_output << "imul";
        break;
    case Instruction::Opcode::UMul:
        m_output << "umul";
        break;
    case Instruction::Opcode::FMul:
        m_output << "fmul";
        break;
    case Instruction::Opcode::Allocate:
        m_output << "allocate";
        break;
    case Instruction::Opcode::Ret:
        m_output << "ret";
        break;
    case Instruction::Opcode::ICmpEq:
        m_output << "icmp eq";
        break;
    case Instruction::Opcode::ICmpNe:
        m_output << "icmp ne";
        break;
    case Instruction::Opcode::ICmpGt:
        m_output << "icmp gt";
        break;
    case Instruction::Opcode::ICmpGe:
        m_output << "icmp ge";
        break;
    case Instruction::Opcode::ICmpLt:
        m_output << "icmp lt";
        break;
    case Instruction::Opcode::ICmpLe:
        m_output << "icmp le";
        break;
    case Instruction::Opcode::UCmpGt:
        m_output << "ucmp gt";
        break;
    case Instruction::Opcode::UCmpGe:
        m_output << "ucmp ge";
        break;
    case Instruction::Opcode::UCmpLt:
        m_output << "ucmp lt";
        break;
    case Instruction::Opcode::UCmpLe:
        m_output << "ucmp le";
        break;
    case Instruction::Opcode::FCmpEq:
        m_output << "fcmp eq";
        break;
    case Instruction::Opcode::FCmpNe:
        m_output << "fcmp ne";
        break;
    case Instruction::Opcode::FCmpGt:
        m_output << "fcmp gt";
        break;
    case Instruction::Opcode::FCmpGe:
        m_output << "fcmp ge";
        break;
    case Instruction::Opcode::FCmpLt:
        m_output << "fcmp lt";
        break;
    case Instruction::Opcode::FCmpLe:
        m_output << "fcmp le";
        break;
    case Instruction::Opcode::Jump:
        m_output << "jump";
        break;
    case Instruction::Opcode::Phi:
        m_output << "phi";
        break;
    case Instruction::Opcode::GetElementPtr:
        m_output << "getelementptr";
        break;
    case Instruction::Opcode::Call:
        m_output << "call";
        break;
    case Instruction::Opcode::Zext:
        m_output << "zext";
        break;
    case Instruction::Opcode::Sext:
        m_output << "sext";
        break;
    case Instruction::Opcode::Trunc:
        m_output << "trunc";
        break;
    case Instruction::Opcode::Fptrunc:
        m_output << "fptrunc";
        break;
    case Instruction::Opcode::Fpext:
        m_output << "fpext";
        break;
    case Instruction::Opcode::Fptosi:
        m_output << "fptosi";
        break;
    case Instruction::Opcode::Uitofp:
        m_output << "uitofp";
        break;
    case Instruction::Opcode::Bitcast:
        m_output << "bitcast";
        break;
    case Instruction::Opcode::Ptrtoint:
        m_output << "ptrtoint";
        break;
    case Instruction::Opcode::Inttoptr:
        m_output << "inttoptr";
        break;
    case Instruction::Opcode::Fptoui:
        m_output << "fptoui";
        break;
    case Instruction::Opcode::Sitofp:
        m_output << "sitofp";
        break;
    case Instruction::Opcode::ShiftLeft:
        m_output << "shl";
        break;
    case Instruction::Opcode::LShiftRight:
        m_output << "lshr";
        break;
    case Instruction::Opcode::AShiftRight:
        m_output << "ashr";
        break;
    case Instruction::Opcode::And:
        m_output << "and";
        break;
    case Instruction::Opcode::Or:
        m_output << "or";
        break;
    case Instruction::Opcode::Xor:
        m_output << "xor";
        break;
    case Instruction::Opcode::IDiv:
        m_output << "idiv";
        break;
    case Instruction::Opcode::UDiv:
        m_output << "udiv";
        break;
    case Instruction::Opcode::FDiv:
        m_output << "fdiv";
        break;
    case Instruction::Opcode::IRem:
        m_output << "irem";
        break;
    case Instruction::Opcode::URem:
        m_output << "urem";
        break;
    case Instruction::Opcode::Switch:
        m_output << "switch";
        break;
    case Instruction::Opcode::ExtractValue:
        m_output << "extractvalue";
        break;
    case Instruction::Opcode::Count:
        break;
      break;
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
        }
        m_output << ")";
        break;
  }
}
    
}