#pragma once

#include "ISel/inserter.hpp"
#include "ISel/instruction.hpp"
#include "opt_level.hpp"
#include "pass.hpp"

namespace scbe {
class DataLayout;
}

namespace scbe::ISel {
class Chain;
class Node;
class ConstantInt;
class ConstantFloat;
}

namespace scbe::IR {
class Function;
class Value;
class Block;
class Instruction;
}

namespace scbe::MIR {
class Block;
class Function;
}

namespace scbe::Target {
class InstructionInfo;
class RegisterInfo;
}

namespace scbe::Codegen {

struct MatchResult {
    const ISel::Pattern* m_pattern;
    uint32_t m_cost;
    ISel::Node* m_node;
};

class ISelPass : public FunctionPass {
public:
    ISelPass(Target::InstructionInfo* instrInfo, Target::RegisterInfo* registerInfo, DataLayout* dataLayout, Ref<Context> context, OptimizationLevel level)
        : FunctionPass(), m_instructionInfo(instrInfo), m_registerInfo(registerInfo), m_dataLayout(dataLayout), m_context(context), m_optLevel(level) {}

    bool run(IR::Function* function) override;
    void init(Unit& unit) override;
    void selectPattern(ISel::Node* node);
    MIR::Operand* emitOrGet(ISel::Node* node, MIR::Block* block, bool autoextract = true);
    std::unordered_map<ISel::Root*, MIR::Block*>& getRootToMirNodes() { return m_rootToMirNodes; }

private:
    bool buildTree(IR::Function* function);
    ISel::Node* buildValue(IR::Value* value);
    ISel::Node* buildInstruction(IR::Instruction* value);
    void patchInstruction(IR::Instruction* value);

    ISel::Register* makeOrGetRegister(IR::Value* referenceValue, Type* type);
    ISel::ConstantInt* makeOrGetConstInt(int64_t value, Type* type);
    ISel::ConstantFloat* makeOrGetConstFloat(double value, Type* type);
    ISel::FrameIndex* makeOrGetFrameIndex(uint32_t slot, Type* type);

private:
    ISel::Inserter m_inserter;
    UMap<IR::Value*, ISel::Node*> m_valuesToNodes;
    UMap<ISel::Root*, MIR::Block*> m_rootToMirNodes;
    UMap<ISel::Node*, MIR::Operand*> m_nodesToMIROperands;
    MIR::Function* m_output = nullptr;
    Target::InstructionInfo* m_instructionInfo = nullptr;
    Target::RegisterInfo* m_registerInfo = nullptr;
    DataLayout* m_dataLayout = nullptr;
    size_t m_constantIdx = 0;
    Ref<Context> m_context = nullptr;
    OptimizationLevel m_optLevel = OptimizationLevel::O0;

    UMap<ISel::Node*, MatchResult> m_bestMatch;

    UMap<IR::Value*, ISel::Register*> m_registers;
    UMap<size_t, ISel::ConstantInt*> m_constantInts;
    UMap<size_t, ISel::ConstantFloat*> m_constantFloats;
    UMap<size_t, ISel::FrameIndex*> m_frameIndices;

    std::vector<std::unique_ptr<ISel::Root>> m_roots; // pass is owner of roots
};
    
}