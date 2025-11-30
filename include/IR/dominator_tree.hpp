#pragma once

namespace scbe::IR {

class Function;
class Block;

struct DTContext {
    UMap<Block*, int> semi;
    UMap<int, Block*> vertex;
    UMap<Block*, Block*> parent;
    UMap<Block*, USet<Block*>> bucket;
    UMap<Block*, Block*> ancestor;
    UMap<Block*, Block*> label;
    UMap<Block*, Block*> idom;

    int N = 0;
};

class DominatorTree {
public:
    DominatorTree(Function* function);

    const std::vector<Block*>& getChildren(Block* block) const { return m_dominatorTree.at(block); }
    const USet<Block*>& getDominanceFrontiers(Block* block) const { return m_dominanceFrontiers.at(block); }
    bool hasChildren(Block* block) const { return m_dominatorTree.contains(block); }
    bool hasDominanceFrontiers(Block* block) const { return m_dominanceFrontiers.contains(block); }
    bool dominates(Block* dominator, Block* dominated) const;

private:
    void DFS(Block* block, DTContext& ctx);
    void compress(Block* block, DTContext& ctx);
    Block* eval(Block* block, DTContext& ctx);

private:
    std::unordered_map<Block*, std::vector<Block*>> m_dominatorTree;
    std::unordered_map<Block*, USet<Block*>> m_dominanceFrontiers;
};

}