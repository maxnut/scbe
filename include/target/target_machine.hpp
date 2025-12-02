#pragma once

#include "data_layout.hpp"
#include "pass_manager.hpp"
#include "target/instruction_info.hpp"
#include "target/register_info.hpp"
#include "target/target_specification.hpp"

#include <initializer_list>

namespace scbe {
class Context;
}

namespace scbe::Target {

    enum class FileType {
        AssemblyFile,
        ObjectFile,
        Count
    };

class TargetMachine {
public:
    TargetMachine(TargetSpecification spec, Ref<Context> context) : m_spec(spec), m_context(context) {}
    virtual ~TargetMachine() = default;

    virtual void addPassesForCodeGeneration(Ref<PassManager> passManager, std::ofstream& output, FileType type, OptimizationLevel level) = 0;
    virtual void addPassesForCodeGeneration(Ref<PassManager> passManager, std::initializer_list<std::reference_wrapper<std::ofstream>> files, std::initializer_list<FileType> type, OptimizationLevel level) = 0;
    virtual DataLayout* getDataLayout() = 0;
    virtual RegisterInfo* getRegisterInfo() = 0;
    virtual InstructionInfo* getInstructionInfo() = 0;

protected:
    TargetSpecification m_spec;
    Ref<Context> m_context = nullptr;
};

}