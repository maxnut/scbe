#pragma once

#include "MIR/instruction.hpp"
#include "target/register_info.hpp"
#include "type.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

#define VREG_START 1024

namespace scbe::MIR {

class Function;
class Block;

struct LiveRange {
    uint32_t m_id;
    std::pair<MIR::Instruction*, MIR::Instruction*> m_instructionRange;
    bool m_assignedFirst = false;
};

struct VRegInfo {
    uint32_t m_class;
    Type* m_typeOverride = nullptr;
};

class RegisterInfo {
public:
    RegisterInfo(Function* function) : m_function(function) {}

    size_t getNextVirtualRegister(uint32_t c, Type* typeOverride = nullptr) { m_vRegTypes.push_back({c, typeOverride}); return m_currentVirtualRegister++; }
    VRegInfo& getVirtualRegisterInfo(size_t id) { assert(id >= VREG_START); return m_vRegTypes[id - VREG_START]; }
    std::vector<LiveRange>& getLiveRanges(size_t id) { return m_liveRanges[id]; }
    std::vector<uint32_t>& getPVMappings(size_t id) { return m_physicalToVirtual[id]; }
    uint32_t getVPMapping(size_t id) { return m_virtualToPhysical[id]; }
    std::unordered_set<uint32_t>& getSpills() { return m_spills; }    

    bool isRegisterLive(size_t pos, uint32_t reg, Target::RegisterInfo* info);
    bool isRegisterEverLive(uint32_t reg, Target::RegisterInfo* info);
    bool hasVPMapping(size_t id) { return m_virtualToPhysical.contains(id); }

    void addLiveRange(uint32_t id, LiveRange range) { m_liveRanges[id].push_back(range); }
    void addPVMapping(uint32_t physical, uint32_t virtualRegister) { m_physicalToVirtual[physical].push_back(virtualRegister); }
    void setVPMapping(uint32_t virtualRegister, uint32_t physical) { m_virtualToPhysical[virtualRegister] = physical; }
    void addSpill(uint32_t physical) { m_spills.insert(physical); }
    
private:
    size_t m_currentVirtualRegister = VREG_START;
    std::vector<VRegInfo> m_vRegTypes;
    UMap<uint32_t, std::vector<LiveRange>> m_liveRanges;
    UMap<uint32_t, std::vector<uint32_t>> m_physicalToVirtual;
    USet<uint32_t> m_spills;
    UMap<uint32_t, uint32_t> m_virtualToPhysical;
    Function* m_function;

friend class Function;
friend class Block;
};

}