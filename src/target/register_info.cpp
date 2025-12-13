#include "target/register_info.hpp"
#include "MIR/register_info.hpp"
#include "hash.hpp"
#include <algorithm>

namespace scbe::Target {

std::optional<uint32_t> RegisterInfo::getRegisterWithSize(uint32_t reg, size_t desiredSize) const {
    const auto& regDesc = getRegisterDesc(reg);
    uint32_t regClassId = regDesc.getRegClass();
    const auto& regClass = getRegisterClass(regClassId);
    if(regClass.getSize() == desiredSize) return reg;
    
    const auto& regs = regClass.getRegs();

    // Find the index of this register in its class
    auto it = std::ranges::find(regs.begin(), regs.end(), reg);
    if (it == regs.end()) return std::nullopt;
    size_t indexInClass = std::distance(regs.begin(), it);

    // Now look for another class with same layout but desired size
    for (const auto& targetClass : m_registerClasses) {
        if (!(targetClass.getSize() == desiredSize && targetClass.getRegs().size() > indexInClass))
            continue;

        uint32_t regId = targetClass.getRegs()[indexInClass];

        if(std::find(regDesc.getAliasRegs().begin(), regDesc.getAliasRegs().end(), regId) == regDesc.getAliasRegs().end())
            continue;

        return regId;
    }

    return std::nullopt;
}

uint32_t RegisterInfo::getRegisterIdClass(uint32_t registerId, MIR::RegisterInfo& mirInfo) const {
    if(isPhysicalRegister(registerId))
        return m_registerDescs[registerId].getRegClass();

    return mirInfo.getVirtualRegisterInfo(registerId).m_class;
}

bool RegisterInfo::isSameRegister(uint32_t reg1, uint32_t reg2) const {
    if (reg1 == reg2) return true;
    if (!isPhysicalRegister(reg1) || !isPhysicalRegister(reg2)) return false;

    const auto& aliases1 = getAliasRegs(reg1);
    if (std::find(aliases1.begin(), aliases1.end(), reg2) != aliases1.end()) return true;

    const auto& aliases2 = getAliasRegs(reg2);
    if (std::find(aliases2.begin(), aliases2.end(), reg1) != aliases2.end()) return true;

    return false;
}

MIR::Register* RegisterInfo::getRegister(uint32_t id, int64_t flags) {
    size_t hash = hashValues(id, flags);
    if(m_mirRegisterCache.contains(hash)) return m_mirRegisterCache.at(hash);
    std::unique_ptr ptr = std::unique_ptr<MIR::Register>(new MIR::Register(id));
    ptr->setFlags(flags);
    MIR::Register* ret = ptr.get();
    m_mirRegisterCache.insert({hash, ret});
    m_mirRegisters.push_back(std::move(ptr));
    return ret;
}

uint32_t RegisterInfo::getCanonicalRegister(uint32_t reg) {
    if(!isPhysicalRegister(reg)) return reg;
    auto& desc = getRegisterDesc(reg);
    auto& aliases = desc.getAliasRegs();
    uint32_t min = aliases.size() ? *std::min_element(aliases.begin(), aliases.end()) : reg;
    if(reg < min) return reg;
    return min;
}


}