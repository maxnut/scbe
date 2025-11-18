#include "IR/loop_analysis.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include <deque>

namespace scbe::IR {

struct BackEdge {
    Block* m_latch = nullptr;
    Block* m_header = nullptr;
};

bool LoopAnalysis::run(IR::Function* function) {
    function->getHeuristics().m_loops.clear();
    std::vector<BackEdge> backEdges;

    for(auto& block : function->getBlocks()) {
        for(auto& pair : block->getSuccessors()) {
            Block* successor = pair.first;
            if(block.get() == successor) {
                std::unique_ptr<LoopInfo> loop = std::make_unique<LoopInfo>(block.get());
                loop->m_blocks.push_back(block.get());
                function->getHeuristics().addLoop(std::move(loop));
                continue;
            }
            if(!function->getDominatorTree()->dominates(successor, block.get())) continue;
            backEdges.push_back({block.get(), successor});
        }
    }

    for(const BackEdge& backEdge : backEdges) {
        std::unique_ptr<LoopInfo> loop = std::make_unique<LoopInfo>(backEdge.m_header);
        loop->m_blocks.push_back(backEdge.m_header);
        loop->m_blocks.push_back(backEdge.m_latch);
        std::deque<Block*> worklist = {backEdge.m_latch};
        while (!worklist.empty()) {
            Block* b = worklist.back();
            worklist.pop_back();
            for (auto& pair : b->getPredecessors()) {
                Block* pred = pair.first;
                if (loop->contains(pred)) continue;
                loop->m_blocks.push_back(pred);
                if (pred != backEdge.m_header) worklist.push_back(pred);
            }
        }
        function->getHeuristics().addLoop(std::move(loop));
    }

    for(auto& loop1 : function->getHeuristics().getLoops()) {
        for(auto& loop2 : function->getHeuristics().getLoops()) {
            if(loop1.get() == loop2.get() || !loop1->contains(loop2->m_header)) continue;
            loop1->m_children.push_back(loop2.get());
            loop2->m_parent = loop1.get();
        }
    }

    for(auto& loop : function->getHeuristics().getLoops()) {
        if(loop->getParent()) continue;
        propagateDepth(loop.get(), 0);
    }

    return false;
}

void LoopAnalysis::propagateDepth(LoopInfo* loop, uint32_t depth) {
    loop->m_depth = depth;
    for(auto& child : loop->getChildren())
        propagateDepth(child, depth + 1);
}

}