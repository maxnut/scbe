#pragma once

#include "ISel/DAG/builder.hpp"
#include "ISel/DAG/instruction.hpp"
#include "opt_level.hpp"
#include "pass.hpp"

namespace scbe {
class DataLayout;
}

namespace scbe::ISel::DAG {
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
    const ISel::DAG::Pattern* m_pattern;
    uint32_t m_cost;
    ISel::DAG::Node* m_node;
};

class DagISelPass : public FunctionPass {
public:
    DagISelPass(Target::InstructionInfo* instrInfo, Target::RegisterInfo* registerInfo, DataLayout* dataLayout, Ref<Context> context, OptimizationLevel level)
        : FunctionPass(), m_instructionInfo(instrInfo), m_registerInfo(registerInfo), m_dataLayout(dataLayout), m_context(context), m_optLevel(level) {}

    bool run(IR::Function* function) override;
    void init(Unit& unit) override;
    void selectPattern(ISel::DAG::Node* node);
    MIR::Operand* emitOrGet(ISel::DAG::Node* node, MIR::Block* block);
    std::unordered_map<ISel::DAG::Root*, MIR::Block*>& getDagRootToMirNodes() { return m_dagRootToMirNodes; }

private:
    bool buildDAG(IR::Function* function);
    ISel::DAG::Chain* earlyBuildChain(IR::Instruction* instruction);
    void patchChain(IR::Instruction* instruction, ISel::DAG::Chain* chain);
    ISel::DAG::Root* buildBlock(IR::Block* block);
    ISel::DAG::Node* buildNonChain(IR::Value* value);
    ISel::DAG::Node* buildInstruction(IR::Instruction* value);

    bool isChain(IR::Instruction* instruction);

    ISel::DAG::Register* makeOrGetRegister(IR::Value* referenceValue, Type* type);
    ISel::DAG::ConstantInt* makeOrGetConstInt(int64_t value, Type* type);
    ISel::DAG::ConstantFloat* makeOrGetConstFloat(double value, Type* type);
    ISel::DAG::FrameIndex* makeOrGetFrameIndex(uint32_t slot, Type* type);

private:
    ISel::DAG::Builder m_builder;
    UMap<IR::Value*, ISel::DAG::Node*> m_valuesToNodes;
    UMap<ISel::DAG::Root*, MIR::Block*> m_dagRootToMirNodes;
    UMap<ISel::DAG::Node*, MIR::Operand*> m_nodesToMIROperands;
    MIR::Function* m_output = nullptr;
    Target::InstructionInfo* m_instructionInfo = nullptr;
    Target::RegisterInfo* m_registerInfo = nullptr;
    DataLayout* m_dataLayout = nullptr;
    size_t m_constantIdx = 0;
    Ref<Context> m_context = nullptr;
    OptimizationLevel m_optLevel = OptimizationLevel::O0;

    UMap<ISel::DAG::Node*, MatchResult> m_bestMatch;
    USet<ISel::DAG::Node*> m_inProgress;

    UMap<IR::Value*, ISel::DAG::Register*> m_registers;
    UMap<size_t, ISel::DAG::ConstantInt*> m_constantInts;
    UMap<size_t, ISel::DAG::ConstantFloat*> m_constantFloats;
    UMap<size_t, ISel::DAG::FrameIndex*> m_frameIndices;

    std::vector<std::unique_ptr<ISel::DAG::Root>> m_roots; // pass is owner of roots
};
    
}