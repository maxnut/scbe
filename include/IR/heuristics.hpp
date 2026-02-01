#pragma once

namespace scbe::IR {

class Block;
class Value;
class CallInstruction;

class LoopInfo {
public:
    LoopInfo(Block* header) : m_header(header) {}

    bool contains(Block* block) const { return std::find(m_blocks.begin(), m_blocks.end(), block) != m_blocks.end(); }
    const std::vector<Block*>& getBlocks() const { return m_blocks; }
    const std::vector<LoopInfo*>& getChildren() const { return m_children; }
    LoopInfo* getParent() const { return m_parent; }
    uint32_t getDepth() const { return m_depth; }

private:
    std::vector<Block*> m_blocks;
    std::vector<LoopInfo*> m_children;
    LoopInfo* m_parent = nullptr;
    Block* m_header = nullptr;
    uint32_t m_depth = 0;

friend class LoopAnalysis;
};

class CallSite {
public:
    CallSite(CallInstruction* call, Block* location, Value* callee) : m_call(call), m_location(location), m_callee(callee) {}

    CallInstruction* getCall() const { return m_call; }
    Block* getLocation() const { return m_location; }
    Value* getCallee() const { return m_callee; }
private:
    CallInstruction* m_call = nullptr;
    Block* m_location = nullptr;
    Value* m_callee = nullptr;
};

class Heuristics {
public:
    void addLoop(std::unique_ptr<LoopInfo> loop) { m_loops.push_back(std::move(loop)); }
    const std::vector<std::unique_ptr<LoopInfo>>& getLoops() const { return m_loops; }

    LoopInfo* getInnermostLoop(Block* forBlock);
    LoopInfo* getOutermostLoop(Block* forBlock);

    bool isLoop(Block* block);
    bool isLoop(Block* first, Block* second);

    void addCallSite(CallSite callSite) { m_callSites.push_back(callSite); }
    const std::vector<CallSite>& getCallSites() const { return m_callSites; }
private:
    std::vector<std::unique_ptr<LoopInfo>> m_loops;
    std::vector<CallSite> m_callSites;

friend class LoopAnalysis;
friend class CallAnalysis;
};

}