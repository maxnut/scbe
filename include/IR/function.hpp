#pragma once

#include "IR/global_value.hpp"
#include "IR/dominator_tree.hpp"
#include "IR/heuristics.hpp"
#include "MIR/function.hpp"
#include "calling_convention.hpp"
#include "type.hpp"
#include <memory>

namespace scbe::IR {

class Block;
class AllocateInstruction;
class Instruction;

class Function : public GlobalValue {
public:
    Function(const std::string& name, FunctionType* type, Linkage linkage);
    ~Function();

    Type* getType() const override;
    FunctionType* getFunctionType() const { return (FunctionType*)m_type; }
    Block* getEntryBlock() const;
    MIR::Function* getMachineFunction() const { return m_machineFunction.get(); }
    DominatorTree* getDominatorTree();
    Unit* getUnit() const { return m_unit; }
    Heuristics& getHeuristics();
    CallingConvention getCallingConvention() const { return m_callConv; }

    const std::vector<std::unique_ptr<Block>>& getBlocks() const { return m_blocks; }
    const std::vector<AllocateInstruction*>& getAllocations() const { return m_allocations; }
    const std::vector<std::unique_ptr<FunctionArgument>>& getArguments() const { return m_args; }    
    bool isIntrinsic() const { return m_isIntrinsic; }
    bool isDominatorTreeDirty() const { return m_dominatorTreeDirty; }
    bool hasBody() const { return !m_blocks.empty(); }
    bool isRecursive() const { return m_isRecursive; }

    void replace(Value* replace, Value* with);

    // Remove instruction from function.
    // Make sure to replace any uses of the instruction, since the instruction will be deleted and removed from any uses.
    // Otherwise you will end up with undefined behavior.
    void removeInstruction(Instruction* instruction);
    void setMachineFunction(std::unique_ptr<MIR::Function> function) { m_machineFunction = std::move(function); }
    void removeBlock(Block* block);
    void setCFGDirty() { m_dominatorTreeDirty = true; m_heuristicsDirty = true; }
    void setCallingConvention(CallingConvention cc) { m_callConv = cc; }

    Block* insertBlock(const std::string name = "");
    Block* insertBlockAfter(Block* after, const std::string name = "");
    Block* insertBlockBefore(Block* before, const std::string name = "");

    void insertBlock(std::unique_ptr<Block> block);
    void insertBlockAfter(Block* after, std::unique_ptr<Block> block);
    void insertBlockBefore(Block* before, std::unique_ptr<Block> block);

    void mergeBlocks(Block* into, Block* from);

    void computeDominatorTree();
    size_t getInstructionIdx(Instruction* instruction) const;
    size_t getInstructionSize() const;

    auto getBlockIdx(Block* block) {
        return std::find_if(m_blocks.begin(), m_blocks.end(),
            [block](auto const& ptr) { return ptr.get() == block; });
    }
    
protected:
    std::vector<std::unique_ptr<Block>> m_blocks;
    std::vector<AllocateInstruction*> m_allocations;
    std::vector<std::unique_ptr<FunctionArgument>> m_args;
    std::unique_ptr<MIR::Function> m_machineFunction = nullptr;
    std::unique_ptr<DominatorTree> m_dominatorTree;
    CallingConvention m_callConv = CallingConvention::Count;
    Unit* m_unit = nullptr;

    size_t m_valueNameCounter = 0;
    
    bool m_isIntrinsic = false;
    bool m_dominatorTreeDirty = true;
    bool m_heuristicsDirty = true;
    bool m_isRecursive = false;

    Heuristics m_heuristics;

friend class Block;
friend class MIR::Function;
friend class scbe::Unit;
friend class CallAnalysis;
friend class AllocateInstruction;
};

}