#pragma once

#include "MIR/constant_pool.hpp"
#include "MIR/instruction.hpp"
#include "MIR/operand.hpp"
#include "MIR/block.hpp"
#include "MIR/register_info.hpp"
#include "block.hpp"
#include "stack_frame.hpp"
#include "target/register_info.hpp"
#include "type_alias.hpp"

#include <cassert>
#include <string>

namespace scbe::IR {
class Function;
}

namespace scbe::MIR {

class Function {
public:
    Function(const std::string& name, IR::Function* irFunction, Target::RegisterInfo* registerInfo);

    const std::vector<std::unique_ptr<Block>>& getBlocks() const { return m_blocks; }
    const std::vector<Operand*>& getArguments() const { return m_args; }
    const std::vector<uint32_t>& getLiveIns() const { return m_liveIns; }
    const std::string& getName() const { return m_name; }

    Block* getEntryBlock() const { assert(!m_blocks.empty()); return m_blocks.front().get(); }
    IR::Function* getIRFunction() const { return m_irFunction; }
    StackFrame& getStackFrame() { return m_stack; }
    RegisterInfo& getRegisterInfo() { return m_registerInfo; }
    ConstantPool& getConstantPool() { return m_constantPool; }

    size_t getInstructionIdx(Instruction* instruction) const;
    size_t getFunctionPrologueSize() const { return m_functionPrologueSize; }

    void addBlock(std::unique_ptr<Block> block) { block->m_parentFunction = this; m_blocks.push_back(std::move(block)); }
    void addMultiValue(std::unique_ptr<MultiValue> multiValue) { m_multiValues.push_back(std::move(multiValue)); }
    void replace(Operand* replace, Operand* with, bool copyFlags);
    void addLiveIn(uint32_t reg) { m_liveIns.push_back(reg); }
    void setFunctionPrologueSize(size_t size) { m_functionPrologueSize = size; }

    bool hasLiveIn(uint32_t reg) const;

    Operand* cloneOpWithFlags(Operand* operand, int64_t flags);

private:
    std::vector<std::unique_ptr<Block>> m_blocks;
    std::vector<std::unique_ptr<MultiValue>> m_multiValues;
    std::vector<Operand*> m_args;
    std::vector<uint32_t> m_liveIns;
    IR::Function* m_irFunction = nullptr;
    StackFrame m_stack;
    RegisterInfo m_registerInfo{this};
    Target::RegisterInfo* m_targetRegisterInfo = nullptr;
    ConstantPool m_constantPool;
    std::string m_name;

    size_t m_functionPrologueSize = 0;
};
    
}