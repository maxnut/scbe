#include "codegen/graph_color_regalloc.hpp"
#include "MIR/function.hpp"
#include "MIR/instruction.hpp"
#include "MIR/register_info.hpp"
#include "target/instruction_info.hpp"

namespace scbe::Codegen {

class ColorGraph {
public:
    void addNode(Ref<GraphColorRegalloc::GraphNode> node) {
        m_nodes.push_back(node);
    }

    void sort() {
        std::sort(m_nodes.begin(), m_nodes.end(), [](auto& a, auto& b) {
            if(a->m_physicalRegister == SPILL || b->m_physicalRegister == SPILL) {
                if(a->m_physicalRegister == SPILL)
                    return false;
                if(b->m_physicalRegister == SPILL)
                    return true;
            }
            return a->m_connections.size() > b->m_connections.size();
        });
    }

    void remove(uint32_t id) {
        for(uint32_t idx = 0; idx < m_nodes.size(); idx++) {
            auto& node = m_nodes[idx];
            if (node->m_connections.contains(id))
                node->m_connections.erase(id);
        }

        m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(), [&](auto& node) { return node->m_id == id; }), m_nodes.end());
    }

    Ref<GraphColorRegalloc::GraphNode> mostRelevantNode() {
        int min = INT_MAX;
        Ref<GraphColorRegalloc::GraphNode> res;

        for(auto node : m_nodes) {
            if((int)node->m_connections.size() < min) {
                min = node->m_connections.size();
                res = node;
            }
        }

        return res;
    }

    Ref<GraphColorRegalloc::GraphNode> find(uint32_t id) {
        for(auto node : m_nodes) {
            if(node->m_id == id)
                return node;
        }
        return nullptr;
    }

    std::vector<Ref<GraphColorRegalloc::GraphNode>>& getNodes() {
        return m_nodes;
    }

    bool empty() {
        return m_nodes.empty();
    }

private:
    std::vector<Ref<GraphColorRegalloc::GraphNode>> m_nodes;
};

uint32_t GraphColorRegalloc::pickAvailableRegister(MIR::Function* function, uint32_t vregId) {
    if(function->getRegisterInfo().hasVPMapping(vregId))
        return function->getRegisterInfo().getVPMapping(vregId);
    return SPILL;
}

void GraphColorRegalloc::analyze(MIR::Function* function) {
    auto blocks = computeLiveRanges(function);

    ColorGraph graph;
    std::unordered_set<uint32_t> visited;
    for(auto block : blocks) {
        // since we can have more than one live range for a register, but we only want one graph node
        // per register, we process them once, getOverlaps will calculate the overlaps accounting for every live range
        // so there's still advantage to this, maybe in the future i will make it so live ranges for a vreg can have more than
        // one physical register, but not for now
        for(auto range : block->m_rangeVector) {
            if(m_registerInfo->isPhysicalRegister(range->m_id)) continue;
            if(visited.contains(range->m_id)) {
                auto node = graph.find(range->m_id);
                auto overlaps = getOverlaps(range->m_id, block->m_liveRanges, block->m_mirBlock);
                node->m_connections.insert(overlaps.begin(), overlaps.end());
                continue;
            }
            visited.insert(range->m_id);

            auto node = std::make_shared<GraphNode>();
            node->m_id = range->m_id;
            node->m_connections = getOverlaps(range->m_id, block->m_liveRanges, block->m_mirBlock);
            graph.addNode(node);
        }
    }
    graph.sort();

    std::vector<Ref<GraphNode>> workStack;
    while(!graph.empty()) {
        bool removed = false;
        for(auto node : graph.getNodes()) {
            MIR::VRegInfo info = function->getRegisterInfo().getVirtualRegisterInfo(node->m_id);
            if(getVirtualConnectionCount(node->m_connections, m_registerInfo) >= m_registerInfo->getAvailableRegisters(info.m_class).size()) continue;
            workStack.push_back(node);
            graph.remove(node->m_id);
            removed = true;
            break;
        }

        if(!removed) {
            auto node = graph.mostRelevantNode();
            graph.remove(node->m_id);
            function->getRegisterInfo().addSpill(node->m_id);
        }
    }

    while(!workStack.empty()) {
        Ref<GraphNode> popped = workStack.back();
        workStack.pop_back();

        MIR::VRegInfo info = function->getRegisterInfo().getVirtualRegisterInfo(popped->m_id);
        for(auto phys : m_registerInfo->getAvailableRegisters(info.m_class)) {
            bool found = false;
            for(uint32_t conn : popped->m_connections) {
                if(m_registerInfo->isPhysicalRegister(conn)) {
                    if(m_registerInfo->isSameRegister(conn, phys)) {
                        found = true;
                        break;
                    }
                    continue;
                }
                auto node = graph.find(conn);
                if(!m_registerInfo->isSameRegister(node->m_physicalRegister, phys)) continue;
                found = true;
                break;
            }

            if(!found) {
                popped->m_physicalRegister = phys;
                break;
            }
        }

        graph.addNode(popped);
    }
    for(auto node : graph.getNodes()) {
        if(node->m_physicalRegister == SPILL - 1) {
            function->getRegisterInfo().addSpill(node->m_id);
            continue;
        }
        function->getRegisterInfo().setVPMapping(node->m_id, node->m_physicalRegister);
    }
}

std::vector<Ref<GraphColorRegalloc::Block>> GraphColorRegalloc::computeLiveRanges(MIR::Function* function) {
    std::unordered_map<MIR::Block*, Ref<Block>> blocks;
    for(auto& block : function->getBlocks()) {
        auto bb = std::make_shared<Block>();
        bb->m_mirBlock = block.get();
        blocks[block.get()] = bb;
    }

    for(auto& block : function->getBlocks()) {
        for(auto& succ : block->getSuccessors()) {
            blocks[block.get()]->m_successors.push_back(blocks[succ]);
        }
    }

    std::vector<Ref<Block>> result;

    for(auto& block : function->getBlocks()) {
        if(!blocks.contains(block.get())) {
            continue;
        }
        auto current = blocks[block.get()];
        result.push_back(current);
    }

    auto entryBlock = blocks[function->getEntryBlock()];
    for(auto block : result)
        fillRanges(block);
    for(uint32_t livein : function->getLiveIns()) {
        rangeForRegister(livein, 0, entryBlock, false);
    }

    auto root = blocks[function->getEntryBlock()];
    std::unordered_set<Ref<Block>> visited;
    visit(root, visited);
    visited.clear();
    propagate(root, visited);

    for(auto block : result) {
        for(auto pair : block->m_liveRanges) {
            for(auto range : pair.second) {
                function->getRegisterInfo().addLiveRange(pair.first, *range);
            }
        }
    }
    return result;
}

void GraphColorRegalloc::rangeForRegister(uint32_t regId, size_t pos, Ref<Block> block, bool assigned) {
    Ref<MIR::LiveRange> range = nullptr;
    if(assigned || block->m_liveRanges[regId].empty()) {
        range = std::make_shared<MIR::LiveRange>();
        range->m_id = regId;
        range->m_instructionRange.first = block->m_mirBlock->getInstructions().at(pos).get();
        range->m_assignedFirst = assigned;
        block->m_liveRanges[regId].push_back(range);
        block->m_rangeVector.push_back(range);
    }

    range = block->m_liveRanges[regId].back();
    range->m_instructionRange.second = block->m_mirBlock->getInstructions().at(pos).get();
}

void GraphColorRegalloc::fillRanges(Ref<GraphColorRegalloc::Block> block) {
    if(block->m_mirBlock->getInstructions().empty()) return;
    block->m_liveRanges.clear();
    block->m_rangeVector.clear();
    size_t pos = block->m_mirBlock->getParentFunction()->getInstructionIdx(block->m_mirBlock->getInstructions().front().get());
    for(size_t i = 0; i < block->m_mirBlock->getInstructions().size(); i++) {
        auto instr = block->m_mirBlock->getInstructions()[i].get();

        if(auto call = dyn_cast<MIR::CallInstruction>(instr)) {
            for(uint32_t ret : call->getReturnRegisters()) {
                rangeForRegister(ret, i, block, true);
            }
        }
        
        Target::InstructionDescriptor desc = m_instructionInfo->getInstructionDescriptor(instr->getOpcode());

        std::vector<uint32_t> assigned;

        for(size_t j = 0; j < instr->getOperands().size(); j++) {
            auto& op = instr->getOperands()[j];
            if(!op || op->getKind() != MIR::Operand::Kind::Register) continue;
            auto reg = cast<MIR::Register>(op);
            Target::Restriction rest = desc.getRestriction(j);
            if(rest.isAssigned()) {
                assigned.push_back(reg->getId());
                continue;
            }

            rangeForRegister(reg->getId(), i, block, false);
        }
        for(uint32_t clobber : desc.getClobberRegisters())
            rangeForRegister(clobber, i, block, false);

        /*
            process assigned last because
            mov %1028, QWORD PTR [rbp - 32]
            mov %1029, DWORD PTR [rbp - 4]
            mov %1032, %1029
            lea %1028, QWORD PTR [%1028 + %1032 * 8]
            in a situation like this, the assignment would split the range incorrectly
            since it would get processed first
        */
        for(uint32_t assignedReg : assigned) {
            rangeForRegister(assignedReg, i, block, true);
        }

        pos++;
    }
}

void GraphColorRegalloc::visit(Ref<Block> root, std::unordered_set<Ref<Block>>& visited) {
    if(visited.contains(root))
        return;
    visited.insert(root);
    std::vector<Ref<Block>> path;
    std::unordered_set<Ref<Block>> visited2;
    fillHoles(root, root, path, visited2);
    for(auto conn : root->m_successors)
        visit(conn, visited);
}

void GraphColorRegalloc::fillHoles(Ref<Block> from, Ref<Block> current, std::vector<Ref<Block>>& path, std::unordered_set<Ref<Block>>& visited) {
    path.push_back(current);

    if(path.size() > 2) {
        for(auto range : from->m_rangeVector) {
            if(!current->m_liveRanges.contains(range->m_id) || range->m_assignedFirst) // warning could be wrong!!!
                continue;

            for(size_t i = 1; i < path.size() - 1; i++) {
                Ref<Block> block = path[i];
                if(block->m_liveRanges.contains(range->m_id))
                    continue;
                auto copy = std::make_shared<MIR::LiveRange>();
                copy->m_id = range->m_id;
                copy->m_assignedFirst = range->m_assignedFirst;
                copy->m_instructionRange = {block->m_mirBlock->getInstructions().front().get(), block->m_mirBlock->getInstructions().back().get()};
                block->m_liveRanges[range->m_id].push_back(copy);
                block->m_rangeVector.push_back(copy);
            }
        }
    }
    
    if(visited.contains(current)) {
        path.pop_back();
        return;
    }
    visited.insert(current);
    for(auto conn : current->m_successors)
        fillHoles(from, conn, path, visited);
    path.pop_back();
}

void GraphColorRegalloc::propagate(Ref<GraphColorRegalloc::Block> root, std::unordered_set<Ref<GraphColorRegalloc::Block>>& visited) {
    if(visited.contains(root))
        return;
    visited.insert(root);

    for(auto range : root->m_rangeVector) {
        for(auto conn : root->m_successors) {
            if(!conn->m_liveRanges.contains(range->m_id))
                continue;

            range->m_instructionRange.second = root->m_mirBlock->getInstructions().back().get();
            conn->m_liveRanges.at(range->m_id).back()->m_instructionRange.first = conn->m_mirBlock->getInstructions().front().get();
        }
    }
    
    for(auto conn : root->m_successors)
        propagate(conn, visited);
}

USet<uint32_t> GraphColorRegalloc::getOverlaps(uint32_t id, const std::unordered_map<uint32_t, std::vector<Ref<MIR::LiveRange>>>& ranges, MIR::Block* block) {
    USet<uint32_t> ret;
    for(auto& my : ranges.at(id)) {
        std::pair<size_t, size_t> first = {block->getParentFunction()->getInstructionIdx(my->m_instructionRange.first), block->getParentFunction()->getInstructionIdx(my->m_instructionRange.second)};

        for(auto& r : ranges) {
            if(r.first == id)
                continue;

            const auto& vec = r.second;

            for(auto& cmp : vec) {
                std::pair<size_t, size_t> second = {block->getParentFunction()->getInstructionIdx(cmp->m_instructionRange.first), block->getParentFunction()->getInstructionIdx(cmp->m_instructionRange.second)};
                if((first.first <= second.second) && (second.first <= first.second))
                    ret.insert(r.first);
            }
        }
    }

    return ret;
}

uint32_t GraphColorRegalloc::getVirtualConnectionCount(const USet<uint32_t>& connections, Target::RegisterInfo* registerInfo) const {
    uint32_t count = 0;
    for(auto& conn : connections) {
        if(registerInfo->isPhysicalRegister(conn)) continue;
        count++;
    }
    return count;
}

}