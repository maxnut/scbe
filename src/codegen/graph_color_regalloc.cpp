#include "codegen/graph_color_regalloc.hpp"
#include "MIR/function.hpp"
#include "MIR/instruction.hpp"
#include "MIR/register_info.hpp"
#include "target/instruction_info.hpp"
#include <algorithm>
#include <deque>

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
            return a->m_canonicalConnections.size() > b->m_canonicalConnections.size();
        });
    }

    void remove(uint32_t id, uint32_t canonical) {
        for(auto& node : m_nodes) {
            node->m_canonicalConnections.erase(canonical);
        }

        m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(), [&](auto& node) { return node->m_id == id; }), m_nodes.end());
    }

    Ref<GraphColorRegalloc::GraphNode> mostRelevantNode() {
        int max = INT_MIN;
        Ref<GraphColorRegalloc::GraphNode> res = nullptr;

        for(auto node : m_nodes) {
            if((int)node->m_canonicalConnections.size() > max) {
                max = node->m_canonicalConnections.size();
                res = node;
            }
        }
        assert(res);

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
    // compute live ranges, but do not add to registerinfo yet. these live ranges will include virtual registers 
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
                node->m_canonicalConnections.insert(overlaps.begin(), overlaps.end());
                continue;
            }
            visited.insert(range->m_id);

            auto node = std::make_shared<GraphNode>();
            node->m_id = range->m_id;
            node->m_canonicalConnections = getOverlaps(range->m_id, block->m_liveRanges, block->m_mirBlock);
            graph.addNode(node);
        }
    }
    graph.sort();

    std::vector<Ref<GraphNode>> workStack;
    while(!graph.empty()) {
        bool removed = false;
        for(auto node : graph.getNodes()) {
            MIR::VRegInfo info = function->getRegisterInfo().getVirtualRegisterInfo(node->m_id);
            if(node->m_canonicalConnections.size() >= m_registerInfo->getAvailableRegisters(info.m_class).size()) continue;
            workStack.push_back(node);
            graph.remove(node->m_id, m_registerInfo->getCanonicalRegister(node->m_id));
            removed = true;
            break;
        }

        if(!removed) {
            auto node = graph.mostRelevantNode();
            graph.remove(node->m_id, m_registerInfo->getCanonicalRegister(node->m_id));
            function->getRegisterInfo().addSpill(node->m_id);
        }
    }

    UMap<uint32_t, uint32_t> assignedColors;
    while(!workStack.empty()) {
        Ref<GraphNode> popped = workStack.back();
        workStack.pop_back();

        MIR::VRegInfo info = function->getRegisterInfo().getVirtualRegisterInfo(popped->m_id);
        for(auto phys : m_registerInfo->getAvailableRegisters(info.m_class)) {
            bool found = false;
            for(uint32_t conn : popped->m_canonicalConnections) {
                if(m_registerInfo->isPhysicalRegister(conn)) {
                    if(m_registerInfo->isSameRegister(conn, phys)) {
                        found = true;
                        break;
                    }
                    continue;
                }

                auto it = assignedColors.find(conn);
                if (it == assignedColors.end())
                    continue;

                if (m_registerInfo->isSameRegister(it->second, phys)) {
                    found = true;
                    break;
                }
            }

            if(!found) {
                popped->m_physicalRegister = phys;
                assignedColors.insert({popped->m_id, phys});
                break;
            }
        }

        graph.addNode(popped);
    }
    for(auto node : graph.getNodes()) {
        if(node->m_physicalRegister == SPILL - 1) {
            throw std::runtime_error("Failed to allocate register");
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
            blocks[block.get()]->m_successors.push_back(blocks[succ].get());
        }
        for(auto& pred : block->getPredecessors()) {
            blocks[block.get()]->m_predecessors.push_back(blocks[pred].get());
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
    for(uint32_t livein : function->getLiveIns()) {
        rangeForRegister(livein, 0, entryBlock, false);
    }
    for(auto block : result)
        fillRanges(block);

    fillHoles(result);
    propagate(result);

    return result;
}

void GraphColorRegalloc::rangeForRegister(uint32_t regId, size_t pos, Ref<Block> block, bool assigned) {
    Ref<MIR::LiveRange> range = nullptr;
    if(assigned || block->m_liveRanges[regId].empty()) {
        range = std::make_shared<MIR::LiveRange>();
        range->m_id = regId;
        range->m_instructionRange.first = block->m_mirBlock->getInstructions().at(pos).get();
        range->m_assignedFirst = assigned;
        range->m_origin = block->m_mirBlock;
        block->m_liveRanges[regId].push_back(range);
        block->m_rangeVector.push_back(range);
    }

    range = block->m_liveRanges[regId].back();
    range->m_instructionRange.second = block->m_mirBlock->getInstructions().at(pos).get();
}

void GraphColorRegalloc::fillRanges(Ref<GraphColorRegalloc::Block> block) {
    if(block->m_mirBlock->getInstructions().empty()) return;
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
            rangeForRegister(clobber, i, block, true);

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

void GraphColorRegalloc::fillHoles(std::vector<Ref<Block>>& blocks) {
    std::deque<Block*> worklist;
    for (auto& block : blocks)
        worklist.push_front(block.get());

    while(!worklist.empty()) {
        auto& block = worklist.front();
        worklist.pop_front();
        for(auto& succ : block->m_successors) {
            for(auto& pair : succ->m_liveRanges) {
                auto& range = pair.second.front();
                if(block->m_liveRanges.contains(range->m_id) || range->m_assignedFirst) continue;
                auto copy = std::make_shared<MIR::LiveRange>();
                copy->m_id = range->m_id;
                copy->m_assignedFirst = range->m_assignedFirst;
                copy->m_instructionRange = {block->m_mirBlock->getInstructions().front().get(), block->m_mirBlock->getInstructions().back().get()};
                copy->m_origin = range->m_origin;
                block->m_liveRanges[range->m_id].push_back(copy);
                block->m_rangeVector.push_back(copy);

                for(auto& pred : block->m_predecessors) {
                    if(succ == pred || std::find(worklist.begin(), worklist.end(), pred) != worklist.end()) continue;
                    worklist.push_back(pred);
                }
            }
        }
    }
}

void GraphColorRegalloc::propagate(std::vector<Ref<Block>>& blocks) {
    for(auto& block : blocks) {
        UMap<MIR::Block*, MIR::Instruction*> lastJumpFor;

        for(auto it = block->m_mirBlock->getInstructions().rbegin(); it != block->m_mirBlock->getInstructions().rend(); it++) {
            auto& instr = *it;
            if(!m_instructionInfo->isJump(instr->getOpcode())) continue;
            for(MIR::Operand* op : instr->getOperands()) {
                if(!op->isBlock()) continue;
                MIR::Block* blockOp = cast<MIR::Block>(op);
                if(lastJumpFor.contains(blockOp)) continue;

                lastJumpFor.insert({blockOp, instr.get()});
            }
        }

        for(auto& pair : block->m_liveRanges) {
            MIR::LiveRange* range = pair.second.back().get();
            for(auto conn : block->m_successors) {
                MIR::LiveRange* firstRangeFor = !conn->m_liveRanges.contains(range->m_id) ? nullptr : conn->m_liveRanges.at(range->m_id).front().get();
                if(!firstRangeFor || (firstRangeFor->m_assignedFirst && firstRangeFor->m_origin == conn->m_mirBlock)) // TODO could be wrong!!!
                    continue;

                range->m_instructionRange.second = lastJumpFor.at(conn->m_mirBlock);
                firstRangeFor->m_instructionRange.first = conn->m_mirBlock->getInstructions().front().get();
            }
        }
    }
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
                    ret.insert(m_registerInfo->getCanonicalRegister(r.first));
            }
        }
    }

    return ret;
}

void GraphColorRegalloc::end(MIR::Function* function) {
    // now compute live ranges and add them to register info. these will be the proper physical register ranges
    auto result = computeLiveRanges(function);
    for(auto block : result) {
        for(auto pair : block->m_liveRanges) {
            for(auto range : pair.second) {
                function->getRegisterInfo().addLiveRange(pair.first, *range);
            }
        }
    }
}

}