#include "IR/instruction.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/dominator_tree.hpp"

namespace scbe::IR {

DominatorTree::DominatorTree(Function* function) {
    DTContext ctx;
    // dfs numbering
    DFS(function->getEntryBlock(), ctx);

    // semi dominators and idoms
    for (int i = ctx.N - 1; i >= 1; --i) {
        Block* w = ctx.vertex[i];

        ctx.semi[w] = i;
        for (auto& pair : w->getPredecessors()) {
            Block* u = eval(pair.first, ctx);
            if (ctx.semi[u] < ctx.semi[w]) ctx.semi[w] = ctx.semi[u];
        }

        Block* semiBlock = ctx.vertex[ctx.semi[w]];
        ctx.bucket[semiBlock].insert(w);

        Block* p = ctx.parent[w];
        ctx.ancestor[w] = p;

        for (Block* v : ctx.bucket[p]) {
            Block* u = eval(v, ctx);
            if (ctx.semi[u] < ctx.semi[v]) ctx.idom[v] = u;
            else ctx.idom[v] = p;
        }
        ctx.bucket[p].clear();
    }

    for (int i = 1; i < ctx.N; ++i) {
        Block* w = ctx.vertex[i];
        if (ctx.idom[w] != ctx.vertex[ctx.semi[w]]) {
            ctx.idom[w] = ctx.idom[ctx.idom[w]];
        }
    }

    ctx.idom[function->getEntryBlock()] = nullptr;

    for(auto& block : function->getBlocks()) {
        if(!ctx.idom.contains(block.get()) || !ctx.idom.at(block.get())) continue;
        m_dominatorTree[ctx.idom.at(block.get())].push_back(block.get());
    }

    // dominance frontiers
    for(auto& block : function->getBlocks()) {
        if(block->getPredecessors().size() < 2)
            continue;
        for(auto& predecessor : block->getPredecessors()) {
            IR::Block* runner = predecessor.first;
            IR::Block* idom = ctx.idom.at(block.get());
            while(runner && runner != idom) {
                m_dominanceFrontiers[runner].insert(block.get());
                runner = ctx.idom[runner];
            }
        }
    }
}

bool DominatorTree::dominates(Block* dominator, Block* dominated) const {
    if(!hasChildren(dominator)) return false;
    auto& children = m_dominatorTree.at(dominator);
    return std::find(children.begin(), children.end(), dominated) != children.end();
}

void DominatorTree::DFS(Block* block, DTContext& ctx) {
    ctx.semi[block] = ctx.N;
    ctx.vertex[ctx.N] = block;
    ctx.label[block] = block;
    ctx.N++;
    for (auto& succ : block->getSuccessors()) {
        if(ctx.semi.contains(succ.first)) continue;
        ctx.parent[succ.first] = block;
        DFS(succ.first, ctx);
    }
}

void DominatorTree::compress(Block* block, DTContext& ctx) {
    if (ctx.ancestor[ctx.ancestor[block]] != nullptr) {
        compress(ctx.ancestor[block], ctx);
        if (ctx.semi[ctx.label[ctx.ancestor[block]]] < ctx.semi[ctx.label[block]]) {
            ctx.label[block] = ctx.label[ctx.ancestor[block]];
        }
        ctx.ancestor[block] = ctx.ancestor[ctx.ancestor[block]];
    }
}

Block* DominatorTree::eval(Block* block, DTContext& ctx) {
    if (ctx.ancestor[block] == nullptr)
        return ctx.label[block];
    compress(block, ctx);
    return ctx.label[block];
}

}