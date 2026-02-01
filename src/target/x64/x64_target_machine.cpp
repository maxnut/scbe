#include "IR/cfg_semplification.hpp"
#include "IR/constant_folder.hpp"
#include "IR/dce.hpp"
#include "IR/function_inlining.hpp"
#include "IR/mem2reg.hpp"
#include "IR/split_critical_edge.hpp"
#include "codegen/coff_object_emitter.hpp"
#include "codegen/elf_object_emitter.hpp"
#include "codegen/graph_color_regalloc.hpp"
#include "codegen/isel_pass.hpp"
#include "data_layout.hpp"
#include "target/instruction_info.hpp"
#include "target/register_info.hpp"
#include "target/target_specification.hpp"
#include "target/x64/encoding/x64_instruction_encoder.hpp"
#include "target/x64/x64_asm_printer.hpp"
#include "target/x64/x64_instruction_info.hpp"
#include "target/x64/x64_legalizer.hpp"
#include "target/x64/x64_register_info.hpp"
#include "target/x64/x64_save_call_registers.hpp"
#include "target/x64/x64_target_lowering.hpp"
#include "target/x64/x64_target_machine.hpp"

#include <algorithm>

using namespace scbe::ISel;

namespace scbe::Target::x64 {

class x64DataLayout : public DataLayout {
    size_t getPointerSize() const override {
        return 8;
    }

    size_t getAlignment(Type* type) const override {
        switch(type->getKind()) {
            default: break;
            case Type::TypeKind::Integer:
                return std::max(1, ((const IntegerType*)type)->getBits() / 8);
            case Type::TypeKind::Float:
                return std::max(1, ((const FloatType*)type)->getBits() / 8);
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

void x64TargetMachine::addPassesForCodeGeneration(Ref<PassManager> passManager, std::ofstream& output, FileType type, OptimizationLevel level) {
    if(level >= OptimizationLevel::O1) {
        passManager->addRun({
            std::make_shared<IR::FunctionInlining>(),
            std::make_shared<IR::Mem2Reg>(m_context),
            std::make_shared<IR::ConstantFolder>(m_context),
            std::make_shared<IR::DeadCodeElimination>(),
            std::make_shared<IR::CFGSemplification>()
        }, true);
    }
    passManager->addRun({
        std::make_shared<IR::SplitCriticalEdge>(m_context),
        std::make_shared<x64Legalizer>(m_context, m_spec),
        std::make_shared<Codegen::ISelPass>(getInstructionInfo(), getRegisterInfo(), getDataLayout(), m_context, level),
        std::make_shared<x64TargetLowering>(getRegisterInfo(), getInstructionInfo(), getDataLayout(), m_spec, level, m_context),
        std::make_shared<Codegen::GraphColorRegalloc>(getDataLayout(), getInstructionInfo(), getRegisterInfo()),
        std::make_shared<x64SaveCallRegisters>(getRegisterInfo(), getInstructionInfo())
    }, false);

    if(type == FileType::AssemblyFile) {
        passManager->addRun({std::make_shared<x64AsmPrinter>(output, getInstructionInfo(), getRegisterInfo(), getDataLayout(), m_spec)}, false);
    }
    else {
        if(m_spec.getOS() == OS::Linux) {
            passManager->addRun({std::make_shared<Codegen::ELFObjectEmitter>(
                output, std::make_shared<x64InstructionEncoder>(getInstructionInfo(), m_spec), getInstructionInfo()
            )}, false);
        }
        else {
            passManager->addRun({std::make_shared<Codegen::COFFObjectEmitter>(
                output, std::make_shared<x64InstructionEncoder>(getInstructionInfo(), m_spec), getInstructionInfo()
            )}, false);
        }
    }
}

void x64TargetMachine::addPassesForCodeGeneration(Ref<PassManager> passManager, std::initializer_list<std::reference_wrapper<std::ofstream>> files, std::initializer_list<FileType> type, OptimizationLevel level) {
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
        std::make_shared<IR::SplitCriticalEdge>(m_context),
        std::make_shared<x64Legalizer>(m_context, m_spec),
        std::make_shared<Codegen::ISelPass>(getInstructionInfo(), getRegisterInfo(), getDataLayout(), m_context, level),
        std::make_shared<x64TargetLowering>(getRegisterInfo(), getInstructionInfo(), getDataLayout(), m_spec, level, m_context),
        std::make_shared<Codegen::GraphColorRegalloc>(getDataLayout(), getInstructionInfo(), getRegisterInfo()),
        std::make_shared<x64SaveCallRegisters>(getRegisterInfo(), getInstructionInfo())
    }, false);

    for(size_t i = 0; i < files.size(); i++) {
        if(type.begin()[i] == FileType::AssemblyFile) {
            passManager->addRun({std::make_shared<x64AsmPrinter>(files.begin()[i].get(), getInstructionInfo(), getRegisterInfo(), getDataLayout(), m_spec)}, false);
        }
        else {
            if(m_spec.getOS() == OS::Linux) {
                passManager->addRun({std::make_shared<Codegen::ELFObjectEmitter>(
                    files.begin()[i].get(), std::make_shared<x64InstructionEncoder>(getInstructionInfo(), m_spec), getInstructionInfo()
                )}, false);
            }
            else {
                passManager->addRun({std::make_shared<Codegen::COFFObjectEmitter>(
                    files.begin()[i].get(), std::make_shared<x64InstructionEncoder>(getInstructionInfo(), m_spec), getInstructionInfo()
                )}, false);
            }
        }
    }
}

DataLayout* x64TargetMachine::getDataLayout() {
    static x64DataLayout layout;
    return &layout;
}

RegisterInfo* x64TargetMachine::getRegisterInfo() {
    static x64RegisterInfo info;
    return &info;
}

InstructionInfo* x64TargetMachine::getInstructionInfo() {
    static x64InstructionInfo info(getRegisterInfo(), m_context);
    return &info;
}

}