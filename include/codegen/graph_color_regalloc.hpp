#pragma once

#include "MIR/block.hpp"
#include "MIR/register_info.hpp"
#include "codegen/regalloc_base.hpp"

#include <limits>

namespace scbe::Codegen {

class GraphColorRegalloc : public RegallocBase {
public:
    struct Block {
        std::unordered_map<uint32_t, std::vector<Ref<MIR::LiveRange>>> m_liveRanges;
        std::vector<Ref<MIR::LiveRange>> m_rangeVector;
        MIR::Block* m_mirBlock = nullptr;
        std::vector<Ref<Block>> m_successors;
    };

    struct GraphNode {
        uint32_t m_id = 0;
        uint32_t m_physicalRegister = SPILL - 1;
        USet<uint32_t> m_connections;
    };

    GraphColorRegalloc(DataLayout* dataLayout, Target::InstructionInfo* instrInfo, Target::RegisterInfo* registerInfo) : RegallocBase(dataLayout, instrInfo, registerInfo) {}

    uint32_t pickAvailableRegister(MIR::Function* function, uint32_t vregId) override;
    void analyze(MIR::Function* function) override;

    std::vector<Ref<Block>> computeLiveRanges(MIR::Function* function);

    void rangeForRegister(uint32_t regId, size_t pos, Ref<Block> block, bool assigned);
    void fillRanges(Ref<Block> block);
    void visit(Ref<Block> root, std::unordered_set<Ref<Block>>& visited);
    void fillHoles(Ref<Block> from, Ref<Block> current, std::vector<Ref<Block>>& path, std::unordered_set<Ref<Block>>& visited);
    void propagate(Ref<Block> root, std::unordered_set<Ref<Block>>& visited);

    USet<uint32_t> getOverlaps(uint32_t id, const std::unordered_map<uint32_t, std::vector<Ref<MIR::LiveRange>>>& ranges, MIR::Block* block);
    uint32_t getVirtualConnectionCount(const USet<uint32_t>& connections, Target::RegisterInfo* registerInfo) const;
};

}

