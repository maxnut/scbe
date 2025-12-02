#include "IR/call_analysis.hpp"
#include "IR/cfg_semplification.hpp"
#include "IR/function_inlining.hpp"
#include "IR/loop_analysis.hpp"
#include "data_layout.hpp"
#include "IR/constant_folder.hpp"
#include "IR/dce.hpp"
#include "IR/mem2reg.hpp"
#include "codegen/dag_isel_pass.hpp"
#include "codegen/graph_color_regalloc.hpp"
#include "target/AArch64/AArch64_target_machine.hpp"
#include "target/AArch64/AArch64_asm_printer.hpp"
#include "target/AArch64/AArch64_instruction_info.hpp"
#include "target/AArch64/AArch64_register_info.hpp"
#include "target/AArch64/AArch64_save_call_registers.hpp"
#include "target/AArch64/AArch64_target_lowering.hpp"
#include "target/target_machine.hpp"

namespace scbe::Target::AArch64 {

class AArch64DataLayout : public DataLayout {
    size_t getPointerSize() const override {
        return 8;
    }

    size_t getAlignment(Type* type) const override {
        switch(type->getKind()) {
            default: break;
            case Type::TypeKind::Integer:
                return std::max(1, (cast<const IntegerType>(type))->getBits() / 8);
            case Type::TypeKind::Float:
                return std::max(1, (cast<const FloatType>(type))->getBits() / 8);
            case Type::TypeKind::Void:
                return 0;
            case Type::TypeKind::Struct:
                return 8;
            case Type::TypeKind::Array:
                return 8;
            case Type::TypeKind::Pointer:
            case Type::TypeKind::Function:
                return getPointerSize();
        }
        return 0;
    }

    size_t getSize(Type* type) const override {
        switch(type->getKind()) {
            default: break;
            case Type::TypeKind::Integer:
                return std::max(1, ((const IntegerType*)type)->getBits() / 8);
            case Type::TypeKind::Float:
                return std::max(1, ((const FloatType*)type)->getBits() / 8);
            case Type::TypeKind::Pointer:
                return getPointerSize();
            case Type::TypeKind::Void:
                return 0;
            case Type::TypeKind::Struct: {
                const StructType* structType = (const StructType*)type;
                size_t size = 0;
                for(const auto& element : structType->getContainedTypes()) {
                    size += getSize(element);
                }
                return size;
            }
            case Type::TypeKind::Array: {
                const ArrayType* arrayType = (const ArrayType*)type;
                return arrayType->getScale() * getSize(arrayType->getElement());
            }
            case Type::TypeKind::Function:
                return getPointerSize();
        }
        return 0;
    }
};

void AArch64TargetMachine::addPassesForCodeGeneration(Ref<PassManager> passManager, std::ofstream& output, FileType type, OptimizationLevel level) {
    if(type == FileType::ObjectFile) throw std::runtime_error("TODO");
    if(level >= OptimizationLevel::O1) {
        passManager->addRun({
            std::make_shared<IR::FunctionInlining>(),
            std::make_shared<IR::Mem2Reg>(m_context),
            std::make_shared<IR::DeadCodeElimination>(),
            std::make_shared<IR::CFGSemplification>(),
            std::make_shared<IR::ConstantFolder>(m_context),
        }, true);
    }
    passManager->addRun({
        std::make_shared<Codegen::DagISelPass>(getInstructionInfo(), getRegisterInfo(), getDataLayout(), m_context, level),
        std::make_shared<AArch64TargetLowering>(getRegisterInfo(), getInstructionInfo(), getDataLayout(), m_spec.getOS(), level),
        std::make_shared<Codegen::GraphColorRegalloc>(getDataLayout(), getInstructionInfo(), getRegisterInfo()),
        std::make_shared<AArch64SaveCallRegisters>(getRegisterInfo(), getInstructionInfo()),
        std::make_shared<AArch64AsmPrinter>(output, getInstructionInfo(), getRegisterInfo(), getDataLayout(), m_spec)
    }, false);
}

void AArch64TargetMachine::addPassesForCodeGeneration(Ref<PassManager> passManager, std::initializer_list<std::reference_wrapper<std::ofstream>> files, std::initializer_list<FileType> type, OptimizationLevel level) {
    throw std::runtime_error("TODO");
}

DataLayout* AArch64TargetMachine::getDataLayout() {
    static AArch64DataLayout layout;
    return &layout;
}

RegisterInfo* AArch64TargetMachine::getRegisterInfo() {
    static AArch64RegisterInfo info;
    return &info;
    return  nullptr;
}

InstructionInfo* AArch64TargetMachine::getInstructionInfo() {
    static AArch64InstructionInfo info(getRegisterInfo(), m_context);
    return &info;
}

}