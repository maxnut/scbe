#include "MIR/register_info.hpp"
#include "MIR/function.hpp"
#include "target/register_info.hpp"

namespace scbe::MIR {

bool RegisterInfo::isRegisterLive(size_t pos, uint32_t reg, Target::RegisterInfo* info, bool assignedIsLive) {
    // if(m_function->hasLiveIn(reg)) return true;
    
    std::vector<uint32_t> worklist;
    if(info->isPhysicalRegister(reg)) {
        worklist.push_back(reg);
        // auto mappings = m_physicalToVirtual[reg];
        // worklist.insert(worklist.end(), mappings.begin(), mappings.end());
        for(uint32_t alias : info->getAliasRegs(reg)) {
            worklist.push_back(alias);
            // mappings = m_physicalToVirtual[alias];
            // worklist.insert(worklist.end(), mappings.begin(), mappings.end());
        }
    }
    else worklist = {reg};

    for(uint32_t id : worklist) {
        if(!m_liveRanges.contains(id)) continue;
        for(const LiveRange& range : m_liveRanges.at(id)) {
            std::pair<size_t, size_t> pair = {m_function->getInstructionIdx(range.m_instructionRange.first), m_function->getInstructionIdx(range.m_instructionRange.second)};
            if(pair.first <= pos && pos <= pair.second) {
                return !(!assignedIsLive && range.m_assignedFirst && pos == pair.first);
            }
        }
    }

    return false;
}

bool RegisterInfo::isRegisterEverLive(uint32_t reg, Target::RegisterInfo* info) {
    if(m_function->hasLiveIn(reg)) return true;
    for(auto& range : m_liveRanges) {
        if(info->isSameRegister(range.first, reg)) return true;
    }
    return false;
}

}