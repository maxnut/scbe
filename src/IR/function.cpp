#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/call_analysis.hpp"
#include "IR/instruction.hpp"
#include "IR/dominator_tree.hpp"
#include "IR/loop_analysis.hpp"
#include "unit.hpp"

#include <algorithm>
#include <cassert>

namespace scbe::IR {

Function::Function(const std::string& name, FunctionType* type, Linkage linkage): GlobalValue(type, linkage, ValueKind::Function, name) {
    for(size_t i = 0; i < type->getArguments().size(); i++) {
        auto arg = type->getArguments()[i];
        auto ptr = std::make_unique<FunctionArgument>(std::to_string(m_valueNameCounter++), arg, i);
        m_args.push_back(std::move(ptr));
    }
}

Function::~Function() = default;

Block* Function::insertBlock(const std::string name) {
    auto block = std::unique_ptr<Block>(new Block(m_unit->getContext(), name));
    Block* ret = block.get();
    insertBlock(std::move(block));
    return ret;
}

Block* Function::insertBlockAfter(Block* after, const std::string name) {
    auto block = std::unique_ptr<Block>(new Block(m_unit->getContext(), name));
    Block* ret = block.get();
    insertBlockAfter(after, std::move(block));
    return ret;
}

Block* Function::insertBlockBefore(Block* before, const std::string name) {
    auto block = std::unique_ptr<Block>(new Block(m_unit->getContext(), name));
    Block* ret = block.get();
    insertBlockBefore(before, std::move(block));
    return ret;
}

void Function::insertBlock(std::unique_ptr<Block> block) {
    if(block->getName().empty())
        block->setName(".");

    if(!m_unit->m_blockNameStack.contains(block->getName()))
        m_unit->m_blockNameStack[block->getName()] = 0;
    block->setName(block->getName() + std::to_string(m_unit->m_blockNameStack[block->getName()]++));

    block->m_parentFunction = this;
    m_blocks.push_back(std::move(block));
    m_dominatorTreeDirty = true;
}

void Function::insertBlockAfter(Block* after, std::unique_ptr<Block> block) {
    if(block->getName().empty())
        block->setName(".");

    if(!m_unit->m_blockNameStack.contains(block->getName()))
        m_unit->m_blockNameStack[block->getName()] = 0;
    block->setName(block->getName() + std::to_string(m_unit->m_blockNameStack[block->getName()]++));

    block->m_parentFunction = this;

    auto it = getBlockIdx(after);
    m_blocks.insert(it + 1, std::move(block));
    m_dominatorTreeDirty = true;
}

void Function::insertBlockBefore(Block* before, std::unique_ptr<Block> block) {
    if(block->getName().empty())
        block->setName(".");

    if(!m_unit->m_blockNameStack.contains(block->getName()))
        m_unit->m_blockNameStack[block->getName()] = 0;
    block->setName(block->getName() + std::to_string(m_unit->m_blockNameStack[block->getName()]++));

    block->m_parentFunction = this;

    auto it = getBlockIdx(before);
    m_blocks.insert(it, std::move(block));
    m_dominatorTreeDirty = true;
}

Type* Function::getType() const {
    return m_unit->getContext()->makePointerType(m_type);
}

Block* Function::getEntryBlock() const {
    assert(!m_blocks.empty() && "Function has no blocks!");
    return m_blocks.front().get();
}

void Function::replace(Value* replace, Value* with) {
    for(auto& block : m_blocks) {
        block->replace(replace, with);
    }
}

void Function::removeInstruction(Instruction* instruction) {
    for(auto& block : m_blocks) {
        block->removeInstruction(instruction);
    }
}

DominatorTree* Function::getDominatorTree() {
    if(m_dominatorTreeDirty) computeDominatorTree();
    return m_dominatorTree.get();
}

Heuristics& Function::getHeuristics() {
    if(m_heuristicsDirty) {
        m_heuristicsDirty = false;
        LoopAnalysis().run(this);
        CallAnalysis().run(this);
    }
    return m_heuristics;
}

void Function::computeDominatorTree() {
    m_dominatorTree = std::make_unique<DominatorTree>(this);
    m_dominatorTreeDirty = false;
}

size_t Function::getInstructionIdx(Instruction* instruction) const {
    size_t cur = 0;
    for(auto& block : m_blocks) {
        if(!block->hasInstruction(instruction)) {
            cur += block->getInstructions().size();
            continue;
        }
        cur += block->getInstructionIdx(instruction);
        break;
    }
    return cur;
}

size_t Function::getInstructionSize() const {
    size_t size = 0;
    for(auto& block : m_blocks)
        size += block->getInstructions().size();
    return size;
}

void Function::removeBlock(Block* block) {
    for(auto& instr : block->getUses())
        instr->removeOperand(block);

    block->clearInstructions();

    for(auto& predecessor : block->m_predecessors)
        predecessor.first->m_successors.erase(block); // remove all instances of block

    for(auto& successor : block->m_successors)
        successor.first->m_predecessors.erase(block);

    m_blocks.erase(std::remove_if(m_blocks.begin(), m_blocks.end(), [&](auto& blockPtr) { return blockPtr.get() == block; }), m_blocks.end());
    m_dominatorTreeDirty = true;
}

void Function::mergeBlocks(Block* into, Block* from) {
    std::vector<std::pair<Value*, Value*>> replacements;
    for(auto& instruction : from->m_instructions) {
        auto clone = instruction->clone();
        replacements.push_back({instruction.get(), clone.get()});
        into->addInstruction(std::move(clone));
    }

    for(auto& replacement : replacements) {
        replace(replacement.first, replacement.second);
    }

    removeBlock(from);
}

}